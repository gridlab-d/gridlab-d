/* $Id: http_client.c 4738 2014-07-03 00:55:39Z dchassin $
 */

#include <cctype>
#include <cstdio>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <limits>

#include "http_client.h"
#include "output.h"
#include "server.h"

#include <cctype>

#ifdef _WIN32
// Windows-specific: Use strtok_s (secure version of strtok)
#define strtok_r strtok_s
#else
// Unix-like systems: Use strtok_r
#include <string.h>
#endif

HTTP *hopen(const char *url, int maxlen) {
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char protocol[64];
  char hostname[1024];
  char filespec[1024];
  HTTP *http;
  char request[1024];
  int len;

#ifdef _WIN32
  /* init sockets */
  WORD wsaVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  if (WSAStartup(wsaVersion, &wsaData) != 0) {
    output_error("hopen(char *url='%s', int maxlen=%d): unable to initialize "
                 "Windows sockets",
                 url, maxlen);
    return nullptr;
  }
#endif

  /* extract URL parts */
  if (sscanf(url, "%63[^:]://%1023[^/]%1023s", protocol, hostname, filespec) !=
      3) {
    output_error("hopen(char *url='%s', int maxlen=%d): badly formed URL", url,
                 maxlen);
    return nullptr;
  }

  /* setup handle */
  http = (HTTP *)malloc(sizeof(HTTP));
  if (http == nullptr) {
    output_error(
        "hopen(char *url='%s', int maxlen=%d): memory allocation failure", url,
        maxlen);
    return nullptr;
  }

  /* initialize http */
  http->buf = nullptr;
  http->len = 0;
  http->pos = 0;

  /* setup socket */
  http->sd = socket(AF_INET, SOCK_STREAM, 0);
  if (http->sd < 0) {
    output_error(
        "hopen(char *url='%s', int maxlen=%d): unable to create socket", url,
        maxlen);
    goto Error;
  }

  /* find server */
  server = gethostbyname(hostname);
  if (server == nullptr) {
    output_error("hopen(char *url='%s', int maxlen=%d): server not found", url,
                 maxlen);
    goto Error;
  }

  /* extract server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr,
         server->h_length);
  serv_addr.sin_port = htons(80);

  /* open connection */
  if (connect(http->sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    output_error(
        "hopen(char *url='%s', maxlen=%d): unable to connect to server", url,
        maxlen);
    goto Error;
  }

  /* format/send request */
  len = snprintf(request, sizeof(request),
                "GET %s HTTP/1.1\r\nHost: %s:80\r\nUser-Agent: "
                "GridLAB-D/%d.%d\r\nConnection: close\r\n\r\n",
                filespec, hostname, REV_MAJOR, REV_MINOR);
  output_debug("sending HTTP message \n%s", request);
  if (send(http->sd, request, len, 0) < len) {
    output_error("hopen(char *url='%s', maxlen=%d): unable to send request",
                 url, maxlen);
    goto Error;
  }

  /* read response header */
  http->buf = (char *)malloc(maxlen + 1);
  http->len = 0;
  do {
    len =
        recv(http->sd, http->buf + http->len, (int)(maxlen - http->len), 0x00);
    if (len == 0) {
      output_debug("hopen(char *url='%s', maxlen=%d): connection closed", url,
                   maxlen);
    } else if (len < 0) {
      output_error("hopen(char *url='%s', maxlen=%d): unable to read response",
                   url, maxlen);
      goto Error;
    } else {
      http->len += len;
      http->buf[http->len] = '\0';
    }
  } while (len > 0);

  if (http->len > 0)
    output_debug("received HTTP message \n%s", http->buf);
  else
    output_debug("no HTTP response received");
  return http;
Error:
#ifdef _WIN32
  output_error("hopen(char *url='%s', int maxlen=%d): WSA error code %d", url,
               maxlen, WSAGetLastError());
#endif
  return nullptr;
}
int hclose(HTTP *http) {
  if (http) {
    if (http->sd)
#ifdef _WIN32
      closesocket(http->sd);
#else
      close(http->sd);
#endif
    if (http->buf != nullptr)
      free(http->buf);
    free(http);
  }
  return 1;
}
size_t hfilelength(HTTP *http) { return http->len; }
int heof(HTTP *http) { return http->pos >= http->len; }
size_t hread(char *buffer, size_t size, HTTP *http) {
  size_t len = http->len - http->pos;
  if (http->pos >= http->len)
    return 0;
  if (len > size)
    len = size;
  memcpy(buffer, http->buf + http->pos, len);
  http->pos += len;
  return len;
}

/* URL access */
HTTPRESULT *http_new_result(void) {
  HTTPRESULT *result = (HTTPRESULT *)malloc(sizeof(HTTPRESULT));
  if (result == nullptr)
    return nullptr;
  result->body.data = nullptr;
  result->body.size = 0;
  result->header.data = nullptr;
  result->header.size = 0;
  result->status = 0;
  return result;
}

void http_delete_result(void *result_in) {
  HTTPRESULT *result = reinterpret_cast<HTTPRESULT *>(result_in);
  if (result == nullptr)
    return;
  // Free allocations even when their logical content length is zero.
  free(result->body.data);
  free(result->header.data);
  free(result);
}

// Return the first body byte, or 0 when no complete HTTP header is present.
// Scan by length so binary response bodies can contain NUL and whitespace.
static size_t http_body_offset(const char *buffer, size_t len) {
  if (buffer == nullptr || len < 4)
    return 0;
  for (size_t i = 0; i <= len - 4; ++i) {
    if (buffer[i] == '\r' && buffer[i + 1] == '\n' &&
        buffer[i + 2] == '\r' && buffer[i + 3] == '\n')
      return i + 4;
  }
  return 0;
}

void *http_read(const char *url, int maxlen) {
  HTTPRESULT *result = http_new_result();
  if (result == nullptr)
    return nullptr;
  if (url == nullptr) {
    result->status = EINVAL;
    return result;
  }

  if (strncmp(url, "http://", 7) == 0) {
    HTTP *fp = hopen(url, maxlen);
    if (fp == nullptr) {
      output_error("http_read('%s'): unable to access url", url);
      result->status = EIO;
      return result;
    }
    const size_t len = hfilelength(fp);
    // A capped response may be truncated even without Content-Length.
    const size_t body_offset = http_body_offset(fp->buf, len);
    if (maxlen <= 0 || len >= static_cast<size_t>(maxlen) ||
        body_offset == 0 || len > static_cast<size_t>(INT_MAX)) {
      output_error("http_read('%s'): truncated or malformed HTTP response", url);
      result->status = EINVAL;
      hclose(fp);
      return result;
    }
    const size_t header_len = body_offset - 4;
    const size_t body_len = len - body_offset;
    result->header.data = (char *)malloc(header_len + 1);
    result->body.data = (char *)malloc(body_len + 1);
    if (result->header.data == nullptr || result->body.data == nullptr) {
      result->status = ENOMEM;
      hclose(fp);
      return result;
    }
    memcpy(result->header.data, fp->buf, header_len);
    result->header.data[header_len] = '\0';
    memcpy(result->body.data, fp->buf + body_offset, body_len);
    result->body.data[body_len] = '\0';
    result->header.size = static_cast<int>(header_len);
    result->body.size = static_cast<int>(body_len);
    result->status = 0;
    hclose(fp);
  } else if (strncmp(url, "file://", 7) == 0) {
    FILE *fp = fopen(url + 7, "rb");
    if (fp == nullptr) {
      result->status = errno;
      return result;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
      result->status = EIO;
    } else {
      const long n = ftell(fp);
      if (n < 0 || n > INT_MAX || fseek(fp, 0, SEEK_SET) != 0) {
        result->status = EINVAL;
      } else {
        const size_t size = static_cast<size_t>(n);
        result->body.data = (char *)malloc(size + 1);
        if (result->body.data == nullptr) {
          result->status = ENOMEM;
        } else if (fread(result->body.data, 1, size, fp) != size) {
          result->status = EIO;
        } else {
          result->body.data[size] = '\0';
          result->body.size = static_cast<int>(size);
          result->status = 0;
        }
      }
    }
    fclose(fp);
  } else {
    result->status = EINVAL;
  }
  return result;
}
const char *http_get_header_data(HTTPRESULT *result, const char *param) {
  if (result == nullptr || result->header.data == nullptr || result->header.size < 0)
    return nullptr;
  if (param == nullptr)
    return result->header.data;
  // Per-thread scratch storage; do not share response values across threads.
  static thread_local char buffer[1024];
  const size_t wanted = strlen(param);
  const char *line = result->header.data;
  const char *limit = line + result->header.size;
  while (line < limit) {
    const char *end = line;
    while (end < limit && *end != '\r' && *end != '\n') ++end;
    const char *colon = line;
    while (colon < end && *colon != ':') ++colon;
    bool matches = colon < end && static_cast<size_t>(colon - line) == wanted;
    for (size_t i = 0; matches && i < wanted; ++i)
      matches = std::tolower(static_cast<unsigned char>(line[i])) ==
                std::tolower(static_cast<unsigned char>(param[i]));
    if (matches) {
      const char *value = colon + 1;
      while (value < end && (*value == ' ' || *value == '\t')) ++value;
      while (end > value && (end[-1] == ' ' || end[-1] == '\t')) --end;
      const size_t size = static_cast<size_t>(end - value);
      if (size >= sizeof(buffer)) return nullptr;
      memcpy(buffer, value, size);
      buffer[size] = '\0';
      return buffer;
    }
    line = end;
    if (line < limit && *line == '\r') ++line;
    if (line < limit && *line == '\n') ++line;
  }
  return nullptr;
}

unsigned int http_get_status(HTTPRESULT *result)
{
	if ( result==nullptr || result->header.data==nullptr || result->header.size<12 )
		return 0;
	output_debug("http_get_status(): '%s'", result->header.data+9);
	return atoi(result->header.data+9);
}

static unsigned int64 wget_maxsize = 100000000;
static enum { WU_NEVER, WU_NEWER, WU_ALWAYS } wget_update = WU_NEWER;
static char wget_cachedir[1024] = "-";

void http_get_options(void) {
  char *option = nullptr, *last = nullptr;
  while (
      (option = strtok_r(
           ((option == nullptr) ? global_wget_options.get_string() : nullptr),
           ";", &last)) != nullptr) {
    char name[1024], value[1024];
    const char *colon = strchr(option, ':');
    if (colon == nullptr) {
      output_error("wget_options: expected name:value option");
      continue;
    }
    const size_t name_len = static_cast<size_t>(colon - option);
    const size_t value_len = strlen(colon + 1);
    if (name_len == 0 || name_len >= sizeof(name) ||
        value_len == 0 || value_len >= sizeof(value) ||
        strpbrk(colon + 1, "\r\n") != nullptr) {
      output_error("wget_options: invalid or oversized option");
      continue;
    }
    memcpy(name, option, name_len);
    name[name_len] = '\0';
    memcpy(value, colon + 1, value_len + 1);
    const int n = 2;
    if (n > 0) {
      if (strcmp(name, "maxsize") == 0) {
        if (n > 1) {
          errno = 0;
          char *end = nullptr;
          const unsigned long long amount = strtoull(value, &end, 10);
          unsigned long long factor = 1;
          if (strcmp(end, "kB") == 0) factor = 1000ULL;
          else if (strcmp(end, "MB") == 0) factor = 1000000ULL;
          else if (strcmp(end, "GB") == 0) factor = 1000000000ULL;
          else if (strcmp(end, "TB") == 0) factor = 1000000000000ULL;
          else if (*end != '\0') factor = 0;
          // strtoull accepts leading spaces and signs; maxsize must begin with a digit.
          if (end == value || value[0] < '0' || value[0] > '9' ||
              errno == ERANGE || factor == 0 ||
              amount > std::numeric_limits<unsigned int64>::max() / factor) {
            output_error("wget_options: invalid or overflowing maxsize '%s'", value);
          } else {
            wget_maxsize = static_cast<unsigned int64>(amount * factor);
          }
        } else
          output_error("wget_options: option '%s' requires a value", name);
      } else if (strcmp(name, "update") == 0) {
        if (n > 1) {
          if (strcmp(value, "never") == 0)
            wget_update = WU_NEVER;
          else if (strcmp(value, "newer") == 0)
            wget_update = WU_NEWER;
          else if (strcmp(value, "always") == 0)
            wget_update = WU_ALWAYS;
          else
            output_error(
                "wget_options: option value '%s' is not a valid update option",
                value);
        } else
          output_error("wget_options: option '%s' requires a value", name);
      } else if (strcmp(name, "cachedir") == 0) {
        if (n > 1)
          strncpy(wget_cachedir, value, sizeof(wget_cachedir) - 1);
        else
          output_error("wget_options: option '%s' requires a value", name);
      } else {
        output_error("wget_options: invalid option '%s' is ignored", name);
      }
    }
  }
  if (strcmp(wget_cachedir, "-") == 0)
    strcpy(wget_cachedir, global_workdir);
  output_debug("wget_option maxsize = %llu",
               static_cast<unsigned long long>(wget_maxsize));
  output_debug("wget_option update = %d", wget_update);
  output_debug("wget_option cachedir = '%s'", wget_cachedir);
  /** @todo read options from global_wget_options */
}

time_t http_read_datetime(const char *timestamp) {
  /* WWW, dd mmm yyyy HH:MM:SS GMT */
  struct tm dt;
  char month[4], tzone[16];
  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  if (timestamp == nullptr)
    return 0;
  if (sscanf(timestamp, "%*[A-Za-z], %d %3s %d %d:%d:%d %15s", &dt.tm_mday,
             month, &dt.tm_year, &dt.tm_hour, &dt.tm_min, &dt.tm_sec,
             tzone) != 7) {
    output_debug("http_read_datetime(const char *timestamp='%s'): unable to "
                 "parse string",
                 timestamp);
    return 0;
  }
  dt.tm_year -= 1900;
  for (dt.tm_mon = 0; dt.tm_mon < 12; dt.tm_mon++) {
    if (strcmp(months[dt.tm_mon], month) == 0)
      break;
  }
  if (dt.tm_mon == 12) {
    output_debug(
        "http_read_datetime(const char *timestamp='%s'): month not recognized",
        timestamp);
    return 0;
  } else
    return mktime(&dt);
}

static bool http_body_is_complete(HTTPRESULT *result)
{
	if ( result==nullptr || result->header.data==nullptr ||
		result->header.size<0 || result->body.size<0 ) return false;
	const char *line = result->header.data;
	const char *header_end = line + result->header.size;
	bool found = false;
	unsigned long long expected = 0;
	while ( line<header_end )
	{
		const char *end = line;
		while ( end<header_end && *end!='\r' && *end!='\n' ) ++end;
		if ( end==line ) break;
		const char *colon = line;
		while ( colon<end && *colon!=':' ) ++colon;
		// The downloader does not implement chunked transfer decoding.
		const char transfer[] = "transfer-encoding";
		bool chunked = colon<end && colon-line==sizeof(transfer)-1;
		for ( size_t i=0; chunked && i<sizeof(transfer)-1; ++i )
			chunked = tolower((unsigned char)line[i])==transfer[i];
		if ( chunked ) return false;
		const char field[] = "content-length";
		bool matches = colon<end && colon-line==sizeof(field)-1;
		for ( size_t i=0; matches && i<sizeof(field)-1; ++i )
			matches = tolower((unsigned char)line[i])==field[i];
		if ( matches )
		{
			// HTTP field names are case-insensitive; only SP/HTAB are OWS.
			const char *value = colon+1;
			while ( value<end && (*value==' ' || *value=='\t') ) ++value;
			if ( value==end || *value<'0' || *value>'9' ) return false;
			unsigned long long length = 0;
			while ( value<end && *value>='0' && *value<='9' )
			{
				unsigned int digit = *value++ - '0';
				if ( length>(std::numeric_limits<unsigned long long>::max()-digit)/10 )
					return false;
				length = length*10 + digit;
			}
			while ( value<end && (*value==' ' || *value=='\t') ) ++value;
			if ( value!=end || (found && length!=expected) ) return false;
			expected = length;
			found = true;
		}
		line = end;
		if ( line<header_end && *line=='\r' ) ++line;
		if ( line<header_end && *line=='\n' ) ++line;
	}
	return !found || expected == (unsigned long long)result->body.size;
}

int http_saveas(char *url, char *file)
{
  if (url == nullptr || file == nullptr)
    return 0;
  http_get_options();
  // http_read accepts int maxlen; never narrow the configured 64-bit limit.
  const int maxlen = wget_maxsize > static_cast<unsigned int64>(INT_MAX)
                         ? INT_MAX : static_cast<int>(wget_maxsize);
  HTTPRESULT *result = static_cast<HTTPRESULT *>(http_read(url, maxlen));
  if (result == nullptr || result->status != 0 ||
      result->header.data == nullptr || http_get_status(result) != 200 ||
      result->body.data == nullptr || !http_body_is_complete(result)) {
    output_error("http_saveas: invalid or incomplete response for '%s'", url);
    http_delete_result(result);
    return 0;
  }
  FILE *fp = fopen(file, "wb");
  if (fp == nullptr) {
    output_error("http_saveas: cannot open '%s': %s", file, strerror(errno));
    http_delete_result(result);
    return 0;
  }
  const size_t body_len = static_cast<size_t>(result->body.size);
  bool ok = fwrite(result->body.data, 1, body_len, fp) == body_len;
  if (fclose(fp) != 0) ok = false;
  http_delete_result(result);
  if (!ok) {
    // Never leave a partially written download looking successful.
    remove(file);
    output_error("http_saveas: failed to write complete file '%s'", file);
    return 0;
  }
  output_verbose("URL '%s' saved to '%s' ok", url, file);
  return 1;
}
