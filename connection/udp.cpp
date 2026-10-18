// $Id$
//
// Implements the datagram transport mechanism
//
#include "udp.h"
#include <sstream>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <limits>

udp::udp()
    : // default defaults for non-strings
      header_version(0), header_size(32), message_version(0.0),
      portnum(6267 + 32768 + 1), // see wiki docs
      debug_level(0) {
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  // default defaults for string
  set_message_format("NONE");
  set_hostname("127.0.0.1");
  set_uri("");

#ifdef _WIN32
  // initialize socket subsystem
  WORD wsaVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  if (WSAStartup(wsaVersion, &wsaData) != 0) {
    set_errormsg("unable to initialize Windows sockets");
    exception("udp::udp() failed");
  }
#endif
  // default socket is non-existent
  sd = INVALID_SOCKET;

  // allocate needed buffers
  sockdata = new struct sockaddr_in;

  // reset the error message to anticipate create coming next
  set_errormsg("udp::create() not called");
}

udp::~udp() { flush(); }

// Parse an unsigned decimal option, rejecting signs, junk and overflow.
static bool udp_parse_uint(const char *text, unsigned int &number) {
  if (text == nullptr || *text < '0' || *text > '9') return false;
  errno = 0;
  char *end = nullptr;
  const unsigned long long value = strtoull(text, &end, 10);
  if (errno == ERANGE || end == text || *end != '\0' ||
      value > std::numeric_limits<unsigned int>::max()) return false;
  number = static_cast<unsigned int>(value);
  return true;
}

/// udp pseudo-property handler
int udp::option(char *command) {
  while (command != nullptr && *command != '\0') {
    while (*command == ',' || *command == ';' ||
           isspace(static_cast<unsigned char>(*command))) ++command;
    if (*command == '\0') break;
    const char *end = command + strcspn(command, ",;");
    const char *key_end = command;
    while (key_end < end && *key_end != '=' &&
           !isspace(static_cast<unsigned char>(*key_end))) ++key_end;
    const size_t key_len = static_cast<size_t>(key_end - command);
    const char *start = key_end;
    while (start < end && (*start == '=' ||
           isspace(static_cast<unsigned char>(*start)))) ++start;
    while (end > start && isspace(static_cast<unsigned char>(end[-1]))) --end;
    const size_t value_len = static_cast<size_t>(end - start);
    if (key_len == 0 || key_len >= 256 || value_len == 0 ||
        value_len >= 1024) {
      error("transport option has empty or oversized name/value");
      return 0;
    }
    char param[256], value[1024];
    memcpy(param, command, key_len); param[key_len] = '\0';
    memcpy(value, start, value_len); value[value_len] = '\0';
    unsigned int number = 0;
    if (strcmp(param, "port") == 0) {
      if (!udp_parse_uint(value, number) || number == 0 || number > 65535) return 0;
      set_portnum(number);
    } else if (strcmp(param, "header_version") == 0) {
      if (!udp_parse_uint(value, number)) return 0;
      set_header_version(number);
    } else if (strcmp(param, "timeout") == 0) {
      if (!udp_parse_uint(value, number)) return 0;
      set_timeout(number);
    } else if (strcmp(param, "hostname") == 0) {
      set_hostname(value);
    } else if (strcmp(param, "uri") == 0) {
      set_uri("%s", value);
    } else if (strcmp(param, "debug_level") == 0) {
      if (!udp_parse_uint(value, number)) return 0;
      set_debug_level(number);
    } else if (strcmp(param, "on_error") == 0) {
      if (strcmp(value, "abort") == 0) onerror_abort();
      else if (strcmp(value, "retry") == 0) onerror_retry();
      else if (strcmp(value, "ignore") == 0) onerror_ignore();
      else return 0;
    } else if (strcmp(param, "maxretry") == 0) {
      if (strcmp(value, "none") == 0) set_maxretry();
      else {
        if (!udp_parse_uint(value, number) || number == 0 || number > INT_MAX)
          return 0;
        set_maxretry(static_cast<int>(number));
      }
    } else {
      error("unknown transport option '%s'", param);
      return 0;
    }
    command = const_cast<char *>(command + strcspn(command, ",;"));
    if (*command != '\0') ++command;
  }
  return 1;
}

int udp::create(void) {
  // create socket
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd == INVALID_SOCKET) {
    set_errormsg("udp::create() is unable to create a socket: %s",
                 Socket::strerror());
    return 0;
  }

  // erase the sockdata structure
  set_sockdata(nullptr);

  // reset the error message to anticipate init coming next
  set_errormsg("udp::init() not called");
  return 1; /* return 1 on success, 0 on failure */
}

int udp::init(void) {
  // set up server address info
  struct sockaddr_in &serv_addr = *get_sockdata();
  serv_addr.sin_family = AF_INET;

  // lookup server
  debug(0, "looking up '%s:%d'", hostname, portnum);
  struct hostent *server = gethostbyname(hostname);
  if (server) {
    debug(1, "IP address for '%s' ok", hostname);
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr,
           server->h_length);
    serv_addr.sin_port = htons(portnum);
  } else
    exception("IP address for '%s' cannot be resolved: ", hostname,
              Socket::strerror());

  // connect to server
  unsigned int retries = 3;
Retry:
  if (::connect(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    if (retries-- > 0)
      goto Retry;
    error("unable to connect to server '%s' on port %d: %s", hostname, portnum,
          Socket::strerror());
    return 0;
  }
  return 1;
}

size_t udp::send(const char *msg, size_t len) {
  if (msg == nullptr) {
    msg = output;
    if (position < 0 || static_cast<size_t>(position) >= sizeof(output)) {
      error("udp::send: invalid output buffer position");
      return 0;
    }
    len = static_cast<size_t>(position);
  }
  // format outbound message header
  char temp[256];
  // Clamp before narrowing the (possibly 64-bit) timeval seconds field.
  int tlim = 1;
  if (timeout.tv_sec >= 9) {
    tlim = 9;
  } else if (timeout.tv_sec >= 0) {
    const int seconds = static_cast<int>(timeout.tv_sec);
    tlim = seconds + (timeout.tv_usec > 0 ? 1 : 0);
    if (tlim < 1) tlim = 1;
  }
  const int header_chars = snprintf(temp, sizeof(temp),
          "%-1u %-3u %-7zu %-5.5s %-3.1f %-1d %-3d   ", header_version,
          header_size, len, message_format, message_version, tlim, 0);
  if (header_chars < 0 || static_cast<size_t>(header_chars) >= sizeof(temp)) {
    error("udp::send: outbound header does not fit buffer");
    return 0;
  }
  const size_t header_len = static_cast<size_t>(header_chars);
  // Field widths above are minimum widths. Reject expanded fields rather than
  // advertising an offset that points into the header instead of the payload.
  if (header_len != static_cast<size_t>(header_size)) {
    error("udp::send: formatted header does not match advertised size");
    return 0;
  }
  char sendbuf[2048];
  if (msg == nullptr || header_len >= sizeof(sendbuf) ||
      len > 1500 - header_len || len >= sizeof(sendbuf) - header_len) {
    error("udp::send: message exceeds UDP packet limit (len=%zu)", len);
    return 0;
  }
  // Explicit len is authoritative; %s would read past nonterminated payloads.
  memcpy(sendbuf, temp, header_len);
  memcpy(sendbuf + header_len, msg, len);
  const size_t total = header_len + len;
  sendbuf[total] = '\0';  // Only for diagnostics, not part of the datagram.
  struct sockaddr_in &serv_addr = *(struct sockaddr_in *)sockdata;
  size_t sndlen = sendto(sd, sendbuf, static_cast<int>(total), 0,
                         (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  debug(9, "%zu <= sendto(addr='%s',port=%d,msg='%s')", sndlen,
        inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port), sendbuf);
  if (sndlen == SOCKET_ERROR)
    exception("UDP sendto failed: %s", Socket::strerror());
  return sndlen;
}
size_t udp::recv(char *buf, size_t len) {
  if (buf == nullptr) {
    memset(input, 0, sizeof(input));
    buf = input;
    len = sizeof(input);
  }
  char msg[2048]; // datagrams cannot exceed metric anyway
  struct sockaddr_in &serv_addr = *(struct sockaddr_in *)sockdata;
  struct sockaddr_in remote;
  socklen_t remote_len = sizeof(remote);

  if (call_setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &this->timeout,
                      sizeof(this->timeout)) != 0)
    exception("unable to set timeout for recv_udp"); // new line of code from
                                                     // Dave to fix timeout
  memset(msg, 0, sizeof(msg));
  int retry = maxretry;
Retry:
  size_t rcvlen =
      recvfrom(sd, msg, sizeof(msg), 0, (struct sockaddr *)&remote, &remote_len);
  debug(9, "%zu <= recvfrom(addr='%s',port=%d)", rcvlen,
        inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
  if (rcvlen == SOCKET_ERROR) {
    switch (on_error) {
    case TE_RETRY:
      if (maxretry == -1 || retry-- > 0) {
        if (retry < 0)
          debug(9, "udp::recv() socket error, retrying (no maxretry)");
        else
          debug(9, "udp::recv() socket error, %d retries left", retry);
        goto Retry;
      }
      // fall through to abort
    case TE_ABORT:
      debug(9, "udp::recv() socket error, aborting");
      return -1;
    case TE_IGNORE:
      debug(9, "udp::recv() socket error, ignoring");
      return 0;
      break;
    default:
      exception("invalid on_error type");
    }
  }
  // Read the whole array so an overlong datagram fills it and is rejected.
  // Reserving the last byte in recvfrom could accept a truncated packet.
  if (rcvlen == 0 || rcvlen >= sizeof(msg))
    exception("unexpected recvfrom return value: %lld", (int64)rcvlen);
  msg[rcvlen] = '\0';
  debug(9, "incoming UDP message from %s:%d = '%s'\n\n",
        inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), msg);
  // check response header
  int version;
  int offset;
  int size;
  std::string type;
  int major, minor;
  int time_out;
  int status;
  char decimal_point;

  // Parse only the advertised header, never payload bytes. Stream integer
  // extraction rejects out-of-range values instead of overflowing scanf's %d.
  const size_t header_length = get_header_size();
  if (header_length == 0 || header_length > rcvlen)
    exception("invalid UDP header length");
  std::istringstream header(std::string(msg, header_length));
  if (!(header >> version >> offset >> size >> type >> major >> decimal_point
               >> minor >> time_out >> status) || decimal_point != '.' ||
      major < 0 || minor < 0 || minor > 9)
    exception("incomplete or invalid message header {msg='%s'}", msg);
  header >> std::ws;
  if (!header.eof())
    exception("unexpected data after UDP header fields");

  // check version
  if (version != get_header_version())
    exception("incorrect header version {msg='%s'}, expected version=%d", msg,
              get_header_version());

  // check offset
  if (offset != get_header_size())
    exception("unexpected header size {msg='%s'}, expected size=%d", msg,
              get_header_size());

  // check type
  if (type != get_message_format())
    exception("unexpected message format {msg='%s'}, expected format='%s'", msg,
              get_message_format());

  // check type version (allow diffs up to 0.1)
  if (fabs((double)major + ((double)minor) / 10 - get_message_version()) >
      0.099)
    exception("unexpected message version {msg='%s'}, expected version=%.1f",
              msg, get_message_version());

  // Verify both the datagram's declared bounds and the caller buffer.
  // Reserve one extra byte for the trailing NUL terminator.
  if (offset<0 || size<0 || (size_t)offset>rcvlen ||
      (size_t)size>rcvlen-(size_t)offset || (size_t)size>=len)
    exception("invalid UDP response length (offset=%d, content-length=%d, datagram=%lu, buffer=%lu)",
              offset, size, (unsigned long)rcvlen, (unsigned long)len);

  // check code
  if (status != 200)
    exception("unexpected response code {msg='%s'}, expected code=200", msg);

  // copy result
  memcpy(buf, msg + offset, size);
  buf[size] = '\0';
  debug(2, "udp::recv() => [%s]", buf);

  // destroy the existing translation (if any)
  if (translator != nullptr)
    translation = (*translator)(buf, translation);

  return size;
}
int udp::call_setsockopt(SOCKET s, int level, int optname, timeval *optval,
                         int optlen) {
#ifdef _WIN32
  if (optval == nullptr || optval->tv_sec < 0 || optval->tv_usec < 0 ||
      optval->tv_usec >= 1000000) {
    WSASetLastError(WSAEINVAL);
    return SOCKET_ERROR;
  }
  const unsigned long long seconds =
      static_cast<unsigned long long>(optval->tv_sec);
  const unsigned long long millis =
      static_cast<unsigned long long>(optval->tv_usec) / 1000ULL;
  const unsigned long long limit = std::numeric_limits<DWORD>::max();
  // Validate before multiplying: an arbitrary timeval may exceed uint64.
  if (seconds > limit / 1000ULL ||
      (seconds == limit / 1000ULL && millis > limit % 1000ULL)) {
    WSASetLastError(WSAEINVAL);
    return SOCKET_ERROR;
  }
  const unsigned long long timeout_ms = seconds * 1000ULL + millis;
  const DWORD time_out = static_cast<DWORD>(timeout_ms);
  return setsockopt(s, level, optname, (const char *)&time_out,
                    sizeof(time_out));
#else
  return setsockopt(s, level, optname, optval, optlen);
#endif
}
void udp::flush(void) {}

// read accessors
unsigned int udp::get_header_version(void) const { return header_version; }
unsigned int udp::get_header_size(void) const { return header_size; }
const char *udp::get_message_format(void) const { return message_format; }
double udp::get_message_version(void) const { return message_version; }
timeval udp::get_timeout(void) const { return timeout; }
const char *udp::get_hostname(void) const { return hostname; }
unsigned int udp::get_portnum(void) const { return portnum; }
const char *udp::get_uri(void) const { return uri; }
const char *udp::get_errormsg(void) const { return errormsg; }
unsigned int udp::get_debug_level(void) const { return debug_level; }
struct sockaddr_in *udp::get_sockdata(size_t n) const {
  return (struct sockaddr_in *)sockdata;
}

// write accessors
void udp::set_header_version(unsigned int n) { header_version = n; }
void udp::set_header_size(unsigned int n) { header_size = n; }
void udp::set_message_format(const char *s) { strcpy(message_format, s); }
void udp::set_message_version(double x) { message_version = x; }
void udp::set_timeout(unsigned int n) {
  // Configured milliseconds; timeval takes seconds and microseconds.
  timeout.tv_sec = n / 1000;
  timeout.tv_usec = (n % 1000) * 1000;
}
void udp::set_hostname(const char *s) {
  snprintf(hostname, sizeof(hostname), "%s", s != nullptr ? s : "");
}
void udp::set_portnum(unsigned int n) { portnum = n; }
void udp::set_uri(const char *fmt, ...) {
  va_list ptr;
  va_start(ptr, fmt);
  vsnprintf(uri, sizeof(uri), fmt, ptr);
  va_end(ptr);
}
void udp::set_errormsg(const char *fmt, ...) {
  va_list ptr;
  va_start(ptr, fmt);
  vsnprintf(errormsg, sizeof(errormsg), fmt, ptr);
  va_end(ptr);
}
void udp::set_debug_level(unsigned int n) { debug_level = n; }
void udp::set_sockdata(struct sockaddr_in *p, size_t n) {
  // TODO
}
void udp::set_output(const char *fmt, ...) {
  va_list ptr;
  va_start(ptr, fmt);
  const int n = vsnprintf(output, sizeof(output), fmt, ptr);
  va_end(ptr);
  if (n < 0) {
    position = 0;
    output[0] = '\0';
  } else {
    position = static_cast<int>(n < static_cast<int>(sizeof(output))
                                ? n : sizeof(output) - 1);
  }
}
void udp::add_output(const char *fmt, ...) {
  if (position < 0) position = 0;
  if (static_cast<size_t>(position) >= sizeof(output))
    position = static_cast<int>(sizeof(output) - 1);
  const size_t remaining = sizeof(output) - static_cast<size_t>(position);
  va_list ptr;
  va_start(ptr, fmt);
  const int n = vsnprintf(output + position, remaining, fmt, ptr);
  va_end(ptr);
  if (n < 0) return;
  const size_t written = static_cast<size_t>(n) < remaining
                             ? static_cast<size_t>(n) : remaining - 1;
  position += static_cast<int>(written);
}
