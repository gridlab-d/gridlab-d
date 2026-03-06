// $Id: validate.cpp 4738 2014-07-03 00:55:39Z dchassin $
// Copyright (C) 2012 Battelle Memorial Institute
//

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Exclude rarely used Windows headers
#include <direct.h>
#include <io.h>
#include <windows.h>
#include <winsock2.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#endif

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <sys/stat.h>

#include <chrono>
#include <thread>

#ifndef _WIN32
#include <signal.h>
#include <sys/wait.h>
#endif

#include <atomic>
#include <mutex>
#include <vector>

#include <algorithm>
#include <cctype> // for std::isspace
#include <deque>

#include <string>

#include <deque>
#include <sstream>

#include "exec.h"
#include "globals.h"
#include "lock.h"
#include "object.h"
#include "output.h"
#include "threadpool.h"
#include "validate.h"

static std::mutex subprocess_launch_mutex;
static std::shared_mutex report_data_lock;

static FILE *error_log_fp = nullptr;
static unsigned int error_log_lock = 0;

/** validating result counter */
class counters
{
public:
    counters(void)
    {
        _lock = 0;
        n_scanned = n_tested = n_passed = n_files = n_success = n_failed =
            n_exceptions = n_access = 0;
    };
    counters operator+(counters a)
    {
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_scanned += a.get_scanned();
        n_tested += a.get_tested();
        n_passed += a.get_passed();
        n_files += a.get_nfiles();
        n_success += a.get_nsuccess();
        n_failed += a.get_nfailed();
        n_exceptions += a.get_nexceptions();
        // wunlock();
        return *this;
    };
    counters operator+=(counters a)
    {
        *this = *this + a;
        return *this;
    };

private:
    unsigned int _lock;
    // directories
    unsigned int n_scanned; // number of directories scanned
    unsigned int n_tested;  // number of autotest directories tested
    unsigned int n_passed;  // number of autotest directories that passed
public:
    unsigned int get_scanned() { return n_scanned; };
    unsigned int get_tested() { return n_tested; };
    unsigned int get_passed() { return n_passed; };
    void inc_scanned()
    {
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_scanned++;
        // wunlock();
    };
    void inc_tested()
    {
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_tested++;
        // wunlock();
    };
    void inc_passed()
    {
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_passed++;
        // wunlock();
    };

private:
    // files
    unsigned int n_files;      // number of tests completed
    unsigned int n_success;    // unexpected successes
    unsigned int n_failed;     // unexpected failures
    unsigned int n_exceptions; // unexpected exceptions
    unsigned int n_access;     // folder access failure
private:
    // void wlock(void) { ::wlock(&_lock); };
    // void wunlock(void) { ::wunlock(&_lock); };
    // std::shared_lock<std::shared_mutex> rlock(void) { return ::rlock(&_lock);
    // }; void runlock(void) { ::runlock(); };
public:
public:
    unsigned int get_nfiles(void) { return n_files; };
    unsigned int get_nsuccess(void) { return n_success; };
    unsigned int get_nfailed(void) { return n_failed; };
    unsigned int get_nexceptions(void) { return n_exceptions; };
    unsigned int get_naccess(void) { return n_access; };
    void inc_files(const char *name)
    {
        if (global_debug_mode || global_verbose_mode)
            output_debug("processing %s", name);
        else
        {
            static size_t len = 0;
            char blank[1024];
            len = (len < 1024 ? len : 1023);
            memset(blank, 32, len);
            blank[len] = '\0';
            len = static_cast<size_t>(
                output_raw("\r%s\rProcessing %s...", blank, name) - len);
        }
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_files++;
        // wunlock();
    };
    void inc_access(const char *name)
    {
        output_debug("%s folder access failure", name);
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_access++;
        // wunlock();
    };
    void inc_success(const char *name, int code, double t)
    {
        output_error("%s success unexpected, code %d in %.1f seconds", name, code,
                     t);
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_success++;
        // wunlock();
    };
    void inc_failed(const char *name, int code, double t)
    {
        output_error("%s error unexpected, code %d (%s) in %.1f seconds", name,
                     code, exec_getexitcodestr(code), t);
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_failed++;
        // wunlock();
    };
    void inc_exceptions(const char *name, int code, double t)
    {
        output_error("%s exception unexpected, code %d (%s) in %.1f seconds", name,
                     code, exec_getexitcodestr(code), t);
        // wlock();
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        n_exceptions++;
        // wunlock();
    };
    void print(void)
    {
        // rlock();
        std::shared_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&_lock));
        unsigned int n_ok = n_files - n_success - n_failed - n_exceptions;
        output_message("\nValidation report:");
        if (n_access)
            output_message("%d directory access failures", n_access);
        output_message("%d models tested", n_files);
        if (n_files != 0)
        {
            if (n_success)
                output_message("%d unexpected successes", n_success);
            if (n_failed)
                output_message("%d unexpected errors", n_failed);
            if (n_exceptions)
                output_message("%d unexpected exceptions", n_exceptions);
            output_message("%d tests succeeded", n_ok);
            output_message("%.0f%% success rate", 100.0 * n_ok / n_files);
        }
        // runlock();
    };
    unsigned int get_nerrors(void)
    {
        return n_success + n_failed + n_exceptions + n_access;
    };
};

static counters final;     // global counter
static bool clean = false; // set to true to force purge of test directories

/* report generation functions */
static FILE *report_fp = nullptr;

#ifdef _WIN32
static char report_file[1024] = "validate_win32.txt";
#else
static char report_file[1024] = "validate.txt";
#endif

static bool line_has_error_token(const char *buf)
{
    static const char *TOKENS[] = {"ERROR", "EXCEPTION", "FATAL", "CRITICAL"};
    for (auto t : TOKENS)
    {
        if (std::strstr(buf, t))
            return true;
    }
    return false;
}

// Keep last N matching lines from one file (append into a shared ring)
static void collect_last_error_lines_from_file(const std::string &filename,
                                               size_t max_matches,
                                               std::deque<std::string> &last)
{
    FILE *fp = std::fopen(filename.c_str(), "r");
    if (!fp)
        return;
    char buf[8192];
    while (std::fgets(buf, sizeof(buf), fp))
    {
        size_t len = std::strlen(buf);
        while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = '\0';
        if (line_has_error_token(buf))
        {
            if (last.size() == max_matches)
                last.pop_front();
            last.emplace_back(buf);
        }
    }
    std::fclose(fp);
}

// Tail helper (unchanged, but increase window to be safe)
static std::string tail_file(const std::string &path, size_t max_lines = 500)
{
    std::deque<std::string> q;
    FILE *fp = std::fopen(path.c_str(), "r");
    if (!fp)
        return {};
    char buf[8192];
    while (std::fgets(buf, sizeof(buf), fp))
    {
        size_t len = std::strlen(buf);
        while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = '\0';
        q.emplace_back(buf);
        if (q.size() > max_lines)
            q.pop_front();
    }
    std::fclose(fp);
    std::string out;
    for (size_t i = 0; i < q.size(); ++i)
    {
        if (i)
            out.push_back('\n');
        out += q[i];
    }
    return out;
}

static std::string get_all_error_lines(const char *dir)
{
    struct FileInfo
    {
        std::string path;
        time_t mtime;
        off_t size;
    };

    static const char *kPaths[] = {
        "/gridlabd.err.pipe", "/gridlabd.out.pipe", "/gridlabd.err",
        "/gridlabd.out", "/gridlabd.wrn", "/gridlabd.dbg",
        "/gridlabd.inf", "/gridlabd.prg", "/gridlabd.pro"};

    std::vector<FileInfo> files;
    files.reserve(sizeof(kPaths) / sizeof(kPaths[0]));

    for (const char *f : kPaths)
    {
        std::string p = std::string(dir) + f;
        struct stat st{};
        time_t mt = 0;
        off_t sz = 0;
        if (stat(p.c_str(), &st) == 0)
        {
            mt = st.st_mtime;
            sz = st.st_size;
        }
        files.push_back({p, mt, sz});
    }

    // Sort newest-first (tie-break larger size)
    std::sort(files.begin(), files.end(), [](const auto &a, const auto &b)
              {
    if (a.mtime != b.mtime)
      return a.mtime > b.mtime;
    return a.size > b.size; });

    // 1) Last-N token lines across ALL files
    std::deque<std::string> last_tokens;
    constexpr size_t MAX_TOKENS = 50; // keep last 50 token lines overall
    for (const auto &fi : files)
    {
        collect_last_error_lines_from_file(fi.path, MAX_TOKENS, last_tokens);
    }
    if (!last_tokens.empty())
    {
        std::string result;
        for (size_t i = 0; i < last_tokens.size(); ++i)
        {
            if (i)
                result.push_back('\n');
            result += last_tokens[i];
        }
        return result;
    }

    // 2) No token lines at all: tail newest file for context
    for (const auto &fi : files)
    {
        auto t = tail_file(fi.path, 500);
        if (!t.empty())
            return t;
    }

    // 3) Nothing available
    return std::string();
}

static void log_test_error(const char *file, char result_type,
                           const std::string &error_msg)
{
    std::unique_lock<std::shared_mutex> lock(
        SharedMutexManager::get_mutex(&error_log_lock));
    if (error_log_fp)
    {
        fprintf(error_log_fp, "\n%c %s\n", result_type, file);
        if (!error_msg.empty())
        {
            fprintf(error_log_fp, "— %s\n", error_msg.c_str());
        }
        else
        {
            fprintf(error_log_fp, "— (no ERROR line found in output)\n");
        }
        // fprintf(error_log_fp, "\n");\

        fflush(error_log_fp);
    }
}

static const char *report_ext = nullptr;
static const char *report_col = "    ";
static const char *report_eol = "\n";
static const char *report_eot = "\f";
static unsigned int report_cols = 0;
static unsigned int report_rows = 0;
static unsigned int report_lock = 0;

static bool report_open(void)
{
    // wlock(&report_lock);
    //  replace the above with SharedMutexManager
    std::unique_lock<std::shared_mutex> lock(
        SharedMutexManager::get_mutex(&report_lock));

    // global_getvar("validate_report", report_file, sizeof(report_file));

    // Decide column/eol based on extension safely
    const char *dot = strrchr(report_file, '.');
    if (dot && strcmp(dot, ".csv") == 0)
    {
        report_col = ",";
        report_eot = "\n";
    }
    else
    {
        report_col = "\t";
        report_eot = "\n";
    } // default to txt-like

    // Open main report
    report_fp = fopen(report_file, "w");
    if (!report_fp)
    {
        output_error("report_open(): unable to open '%s' for writing - %s",
                     report_file, strerror(errno));
        return false;
    }

    if (report_fp != nullptr)
    {
        report_ext = strrchr(report_file, '.');
        if (strcmp(report_ext, ".csv") == 0)
        {
            report_col = ",";
            report_eot = "\n";
        }
        else if (strcmp(report_ext, ".txt") == 0)
        {
            report_col = "\t";
            report_eot = "\n";
        }
        report_fp = fopen(report_file, "w");

        // Create validate_errors.txt in the SAME directory as validate.txt
        char error_log_file[1024];
        strcpy(error_log_file, report_file);

        // In the validation initialization code (where report_fp is opened)
        // char error_log_file[1024];
        snprintf(error_log_file, sizeof(error_log_file), "%s/validate_errors.txt",
                 global_workdir);
        error_log_fp = fopen(error_log_file, "w");
        if (error_log_fp)
        {
            fprintf(error_log_fp, "Validation Error Details\n");
            fprintf(error_log_fp, "========================\n\n");
        }

        // Find the last '/' or '\' to get directory
        char *last_sep = strrchr(error_log_file, '/');
#ifdef _WIN32
        char *last_backslash = strrchr(error_log_file, '\\');
        if (last_backslash > last_sep)
            last_sep = last_backslash;
#endif

        if (last_sep != nullptr)
        {
            // Has directory path - replace filename
            strcpy(last_sep + 1, "validate_errors.txt");
        }
        else
        {
            // No directory path - just use filename in current directory
            strcpy(error_log_file, "validate_errors.txt");
        }

        error_log_fp = fopen(error_log_file, "w");
        if (error_log_fp)
        {
            fprintf(error_log_fp, "Validation Error Details\n");
            fprintf(error_log_fp, "========================\n\n");
        }
    }
    // wunlock(&report_lock);
    return report_fp != nullptr;
}
static int report_title(const char *fmt, ...)
{
    int len = 0;
    if (report_fp)
    {
        // wlock(&report_lock);
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&report_lock));

        if (report_cols++ > 0)
            len = fprintf(report_fp, "%s", report_eol);
        va_list ptr;
        va_start(ptr, fmt);
        len = +vfprintf(report_fp, fmt, ptr);
        va_end(ptr);
        fflush(report_fp);
        // wunlock(&report_lock);
    }
    return len;
}
static int report_data(const char *fmt = "", ...)
{
    int len = 0;
    if (report_fp)
    {
        // wlock(&report_lock);
        std::unique_lock<std::shared_mutex> lock(report_data_lock);

        if (report_cols++ > 0)
            len = fprintf(report_fp, "%s", report_col);
        va_list ptr;
        va_start(ptr, fmt);
        len = +vfprintf(report_fp, fmt, ptr);
        va_end(ptr);
        // wunlock(&report_lock);
    }
    return len;
}
static int report_newrow(void)
{
    int len = 0;
    if (report_fp)
    {
        // wlock(&report_lock);
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&report_lock));

        report_cols = 0;
        report_rows++;
        len = fprintf(report_fp, "%s", report_eol);
        fflush(report_fp);
        // wunlock(&report_lock);
    }
    return len;
}
static int report_newtable(const char *table)
{
    int len = 0;
    if (report_fp)
    {
        // wlock(&report_lock);
        std::unique_lock<std::shared_mutex> lock(
            SharedMutexManager::get_mutex(&report_lock));

        report_rows++;
        if (report_cols > 0)
            len += fprintf(report_fp, "%s", report_eol);
        report_cols = 0;
        len = fprintf(report_fp, "%s%s%s", report_eot, table, report_eol);
        fflush(report_fp);
        // wunlock(&report_lock);
    }
    return len;
}
static int report_close(void)
{
    // wlock(&report_lock);
    //  replace the above with SharedMutexManager
    std::unique_lock<std::shared_mutex> lock(
        SharedMutexManager::get_mutex(&report_lock));
    if (report_fp)
    {
        fclose(report_fp);
        // At the end of validation (where report_fp is closed)
        if (error_log_fp)
        {
            fclose(error_log_fp);
            error_log_fp = nullptr;
            output_message("See '%s/validate_errors.txt' for error details",
                           global_workdir);
        }
    }
    report_fp = nullptr;
    // wunlock(&report_lock);
    return report_rows;
}

/* Windows implementation of opendir/readdir/closedir */
#ifdef _WIN32
struct dirent
{
    unsigned char d_type; /* file type, see below */
    char *d_name;         /* name must be no longer than this */
    struct dirent *next;  /* next entry */
};
typedef struct
{
    struct dirent *first;
    struct dirent *next;
} DIR;
#define DT_DIR 0x01
const char *GetLastErrorMsg(void)
{
    static unsigned int lock = 0;
    // wlock(&lock);
    //  replace the above with SharedMutexManager
    std::unique_lock<std::shared_mutex> wlock(
        SharedMutexManager::get_mutex(&lock));
    static TCHAR szBuf[256];
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf, 0, nullptr);

    char *p;
    while ((p = strchr((char *)lpMsgBuf, '\n')) != nullptr)
        *p = ' ';
    while ((p = strchr((char *)lpMsgBuf, '\r')) != nullptr)
        *p = ' ';
    sprintf(szBuf, "%s (error code %d)", lpMsgBuf, dw);

    LocalFree(lpMsgBuf);
    // wunlock(&lock);
    return szBuf;
}
DIR *opendir(const char *dirname)
{
    WIN32_FIND_DATA fd;
    char search[MAX_PATH];
    sprintf(search, "%s/*", dirname);
    HANDLE dh = FindFirstFile(search, &fd);
    if (dh == INVALID_HANDLE_VALUE)
    {
        output_error("opendir(const char *dirname='%s'): %s", dirname,
                     GetLastErrorMsg());
        final.inc_access(dirname);
        return nullptr;
    }
    DIR *dirp = new DIR;
    dirp->first = dirp->next = new struct dirent;
    dirp->first->d_type =
        (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : 0;
    dirp->first->d_name = new char[strlen(fd.cFileName) + 1];
    strcpy(dirp->first->d_name, fd.cFileName);
    dirp->first->next = nullptr;
    struct dirent *last = dirp->first;
    while (FindNextFile(dh, &fd) != 0)
    {
        struct dirent *dp = (struct dirent *)malloc(sizeof(struct dirent));
        if (dp == nullptr)
        {
            output_error("opendir(const char *dirname='%s'): %s", dirname,
                         GetLastErrorMsg());
            final.inc_access(dirname);
            return nullptr;
        }
        dp->d_type = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : 0;
        dp->d_name = (char *)malloc(strlen(fd.cFileName) + 10);
        if (dp->d_name == nullptr)
        {
            output_error("opendir(const char *dirname='%s'): %s", dirname,
                         GetLastErrorMsg());
            final.inc_access(dirname);
            return nullptr;
        }
        strcpy(dp->d_name, fd.cFileName);
        dp->next = nullptr;
        last->next = dp;
        last = dp;
    }
    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        output_error("opendir(const char *dirname='%s'): %s", dirname,
                     GetLastErrorMsg());
        final.inc_access(dirname);
    }
    FindClose(dh);
    return dirp;
}
struct dirent *readdir(DIR *dirp)
{
    struct dirent *dp = dirp->next;
    if (dp)
        dirp->next = dp->next;
    return dp;
}
int closedir(DIR *dirp)
{
    struct dirent *dp = dirp->first;
    while (dp != nullptr)
    {
        struct dirent *del = dp;
        dp = dp->next;
        free(del->d_name);
        free(del);
    }
    return 0;
}
#define WIFEXITED(X) (X >= 0 && X < 128)
#define WEXITSTATUS(X) (X & 127)
#define WTERMSIG(X) (X & 127)
#define WIFSIGNALED(X) ((X) >= 128)
#endif

/** command line arguments that are passed to test runs */
static char validate_cmdargs[1024];
static char validate_child_cmdargs[1024];

/** variable arg system call */
static int vsystem(const char *fmt, ...)
{
    char command[1024];
    va_list ptr;
    va_start(ptr, fmt);
    vsprintf(command, fmt, ptr);
    va_end(ptr);
    output_debug("calling system('%s')", command);
    int rc = system(command);
    output_debug("system('%s') returns code %x", command, rc);
    return rc;
}

static constexpr int DEFAULT_TEST_TIMEOUT_SECONDS = 2000;

#ifdef _WIN32
#include <windows.h>

int vsystem_posix_exec_argv_capture(
    const std::vector<std::string> &argv, const std::string &out_path,
    const std::string &err_path,
    int timeout_seconds = DEFAULT_TEST_TIMEOUT_SECONDS)
{
    // Build command line string
    std::string cmd;
    for (const auto &arg : argv)
    {
        if (!cmd.empty())
            cmd += " ";
        if (arg.find(' ') != std::string::npos)
            cmd += "\"" + arg + "\"";
        else
            cmd += arg;
    }

    // Create inheritable file handles for stdout/stderr redirection
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hOut = CreateFileA(out_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                              &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hErr = CreateFileA(err_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                              &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hOut == INVALID_HANDLE_VALUE || hErr == INVALID_HANDLE_VALUE)
    {
        if (hOut != INVALID_HANDLE_VALUE)
            CloseHandle(hOut);
        if (hErr != INVALID_HANDLE_VALUE)
            CloseHandle(hErr);
        return -1;
    }

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hOut;
    si.hStdError = hErr;

    PROCESS_INFORMATION pi = {};

    // CreateProcessA needs a mutable command line buffer
    std::vector<char> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back('\0');

    BOOL ok = CreateProcessA(NULL,           // lpApplicationName
                             cmd_buf.data(), // lpCommandLine (mutable)
                             NULL, NULL,     // process/thread security
                             TRUE,           // bInheritHandles
                             0,              // dwCreationFlags
                             NULL, NULL,     // environment, current directory
                             &si, &pi);

    if (!ok)
    {
        CloseHandle(hOut);
        CloseHandle(hErr);
        return -1;
    }

    // Wait with timeout
    DWORD timeout_ms = (timeout_seconds > 0)
                           ? static_cast<DWORD>(timeout_seconds) * 1000
                           : INFINITE;
    DWORD wait = WaitForSingleObject(pi.hProcess, timeout_ms);

    int result;
    if (wait == WAIT_TIMEOUT)
    {
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 5000); // let it clean up
        result = -2;                            // timeout sentinel — same as POSIX version
    }
    else if (wait == WAIT_OBJECT_0)
    {
        DWORD exit_code;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        result = static_cast<int>(exit_code);
    }
    else
    {
        result = -1; // WaitForSingleObject error
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOut);
    CloseHandle(hErr);

    return result;
}
#endif

// replace vsystem_posix() with an argv-based exec
#ifndef _WIN32
int vsystem_posix_exec_argv(const std::vector<std::string> &argv)
{
    pid_t pid = fork();
    if (pid == -1)
        return -1;

    if (pid == 0)
    {
        std::vector<char *> cargs;
        cargs.reserve(argv.size() + 1);
        for (auto &s : argv)
            cargs.push_back(const_cast<char *>(s.c_str()));
        cargs.push_back(nullptr);
        execvp(cargs[0], cargs.data()); // exec the test directly
        _exit(127);                     // exec failed
    }
    else
    {
        int status;
        if (waitpid(pid, &status, 0) == -1)
            return -1;
        return status;
    }
}

// Same forwarder you already have
static void forward_fd_to_file(int fd, const std::string &path)
{
    FILE *fp = std::fopen(path.c_str(), "w");
    if (!fp)
    {
        // Drain pipe to avoid blocking if file can't be opened
        char sink[4096];
        while (read(fd, sink, sizeof(sink)) > 0)
        {
        }
        return;
    }
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        std::fwrite(buf, 1, static_cast<size_t>(n), fp);
        std::fflush(fp);
    }
    std::fclose(fp);
}

int vsystem_posix_exec_argv_capture(
    const std::vector<std::string> &argv, const std::string &out_path,
    const std::string &err_path,
    int timeout_seconds = DEFAULT_TEST_TIMEOUT_SECONDS)
{
    int out_pipe[2], err_pipe[2];
    if (pipe(out_pipe) == -1 || pipe(err_pipe) == -1)
    {
        return -1;
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        close(out_pipe[0]);
        close(out_pipe[1]);
        close(err_pipe[0]);
        close(err_pipe[1]);
        return -1;
    }
    if (pid == 0)
    {
        // Child — unchanged
        (void)dup2(out_pipe[1], STDOUT_FILENO);
        (void)dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[0]);
        close(out_pipe[1]);
        close(err_pipe[0]);
        close(err_pipe[1]);
        std::vector<char *> cargs;
        cargs.reserve(argv.size() + 1);
        for (auto &s : argv)
            cargs.push_back(const_cast<char *>(s.c_str()));
        cargs.push_back(nullptr);
        execvp(cargs[0], cargs.data());
        _exit(127);
    }

    // Parent
    close(out_pipe[1]);
    close(err_pipe[1]);
    std::thread t_out(forward_fd_to_file, out_pipe[0], out_path);
    std::thread t_err(forward_fd_to_file, err_pipe[0], err_path);

    // ──────────────────────────────────────────────
    // THIS IS WHAT CHANGES: polling waitpid + timeout
    // ──────────────────────────────────────────────
    int status = 0;
    bool timed_out = false;
    auto start = std::chrono::steady_clock::now();

    while (true)
    {
        int ret = waitpid(pid, &status, WNOHANG); // non-blocking
        if (ret > 0)
            break; // child exited
        if (ret == -1 && errno != EINTR)
        {
            status = -1;
            break;
        }

        auto elapsed = std::chrono::steady_clock::now() - start;
        if (timeout_seconds > 0 &&
            std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >=
                timeout_seconds)
        {
            // Graceful kill first
            kill(pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Force-kill if still alive
            if (waitpid(pid, &status, WNOHANG) == 0)
            {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0); // reap
            }
            timed_out = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    // ──────────────────────────────────────────────

    // Once the child is dead its pipe fds are closed,
    // so the forwarding threads will see EOF and finish.
    t_out.join();
    t_err.join();

    if (timed_out)
        return -2; // sentinel: caller can distinguish timeout from crash

    return status;
}

#endif

// A simple signal handler for SIGCHLD.
// Its only purpose is to catch the signal so we can control its behavior.
void sigchld_handler(int sig)
{
    // The OS will still reap the terminated child process.
    // By catching the signal, we prevent it from interrupting blocking calls
    // when SA_RESTART is used.
}

#ifndef _WIN32

// A robust vsystem implementation for POSIX systems (macOS, Linux)
// that correctly returns the child process's wait status.
int vsystem_posix(const char *command)
{
    pid_t pid = fork();

    if (pid == -1)
    {
        // Fork failed
        return -1;
    }
    else if (pid == 0)
    {
        // This is the child process.
        // Execute the command using the shell.
        execl("/bin/sh", "sh", "-c", command, (char *)nullptr);

        // If execl returns, it means an error occurred.
        _exit(127); // Standard exit code for exec failure
    }
    else
    {
        // This is the parent process.
        int status;
        // Wait for the child process to terminate and get its status.
        if (waitpid(pid, &status, 0) == -1)
        {
            return -1; // waitpid failed
        }
        return status; // Return the raw wait status
    }
}
#endif

/** routine to destroy the contents of a directory */
static bool destroy_dir(char *name)
{
    DIR *dirp = opendir(name);
    if (dirp == nullptr)
        return true; // directory does not exist
    struct dirent *dp;
    output_debug("destroying contents of '%s'", name);
    while (dirp != nullptr && (dp = readdir(dirp)) != nullptr)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            char file[1024];
            sprintf(file, "%s/%s", name, dp->d_name);
            if (unlink(file) != 0)
            {
                output_error("destroy_dir(char *name='%s'): unlink('%s') returned '%s'",
                             name, dp->d_name, strerror(errno));
                closedir(dirp);
                return false;
            }
            else
                output_debug("deleted %s", dp->d_name);
        }
    }
    closedir(dirp);
    return true;
}

/** copyfile routine */
static bool copyfile(char *from, char *to)
{
    output_debug("copying '%s' to '%s'", from, to);
    FILE *in = fopen(from, "r");
    if (in == nullptr)
    {
        output_error("copyfile(char *from='%s', char *to='%s'): unable to open "
                     "'%s' for reading - %s",
                     from, to, from, strerror(errno));
        return false;
    }
    FILE *out = fopen(to, "w");
    if (out == nullptr)
    {
        output_error("copyfile(char *from='%s', char *to='%s'): unable to open "
                     "'%s' for writing - %s",
                     from, to, to, strerror(errno));
        fclose(in);
        return false;
    }
    char buffer[65536];
    size_t len;
    while ((len = fread(buffer, 1, sizeof(buffer), in)) > 0)
    {
        if (fwrite(buffer, 1, len, out) != len)
        {
            output_error("copyfile(char *from='%s', char *to='%s'): unable to write "
                         "to '%s' - %s",
                         from, to, to, strerror(errno));
            fclose(in);
            fclose(out);
            return false;
        }
    }
    fclose(in);
    fclose(out);
    return true;
}

/** routine to run a validation test */
static counters run_test(char *file, double *elapsed_time = nullptr)
{
    output_debug("run_test(char *file='%s') starting", file);
    counters result;

    bool is_err =
        strstr(file, "_err.") != nullptr || strstr(file, "_err_") != nullptr;

    if (is_err && (global_validateoptions & VO_TSTERR) == 0)
        return result;

    bool is_exc =
        strstr(file, "_exc.") != nullptr || strstr(file, "_exc_") != nullptr;
    if (is_exc && (global_validateoptions & VO_TSTEXC) == 0)
        return result;

    bool is_opt =
        strstr(file, "_opt.") != nullptr || strstr(file, "_opt_") != nullptr;
    if (is_opt && (global_validateoptions & VO_TSTOPT) == 0)
        return result;

    if (!is_err && !is_opt && !is_exc &&
        (global_validateoptions & VO_TSTRUN) == 0)
        return result;

    char dir[1024];
    strcpy(dir, file);
    char *ext = strrchr(dir, '.');
    // char *name = strrchr(dir, '/') + 1;

    char *slash = strrchr(dir, '/');
    char *name = (slash != nullptr) ? slash + 1 : dir;

    char *char_result;

    if (ext == nullptr || (strcmp(ext, ".glm") != 0 && strcmp(ext, ".json") != 0))
    {
        output_error("run_test(char *file='%s'): file is not a GLM or JSON", file);
        return result;
    }
    std::string fileExtension = std::string(ext); // Store the extension for later use
    *ext = '\0';                                  // remove extension from dir

    // Define the output capture file path - GridLAB-D writes errors to
    // gridlabd.err when --redirect all is used
    char output_capture_file[1024];
    sprintf(output_capture_file, "%s/gridlabd.out", dir);

    char cwd[1024];
    char_result = getcwd(cwd, sizeof(cwd));
    if (clean && !destroy_dir(dir))
    {
        output_error("run_test(char *file='%s'): unable to destroy test folder",
                     dir);
        result.inc_access(file);
        return result;
    }
    else
    {
        // output_verbose("run_test(): deleted '%s'", dir);
        rmdir(dir);
    }
#ifdef _WIN32
    if ((0 != mkdir(dir)) && clean)

#else
    if ((0 != mkdir(dir, 0750)) && clean)
#endif
    {
        output_error("run_test(char *file='%s'): unable to create test folder", dir);
        result.inc_access(file);
        return result;
    }
    else
    {
        output_debug("created test folder '%s'", dir);
        // std::cerr << "Thread " << std::hash<std::thread::id>{}(std::this_thread::get_id()) << "using directory " << dir << std::endl;
    }
    char out[1024];
    sprintf(out, "%s/%s%s", dir, name, fileExtension.c_str());
    if (!copyfile(file, out))
    {
        output_error("run_test(char *file='%s'): unable to copy to test folder %s", file, dir);
        result.inc_access(file);
        return result;
    }
    int64 dt = exec_clock();
    result.inc_files(file);
    // 	unsigned int code = vsystem("\"%s\" -W %s %s %s.glm ",
    // #ifdef _WIN32
    // 								_pgmptr,
    // #else
    // 								global_gl_executable.c_str(),
    // #endif
    // 								dir, validate_cmdargs, name);

    // Assuming 'vsystem' is a custom function that executes an external command.
    // If you have control over 'vsystem', consider updating it to take
    // std::string_view or std::string directly. For this example, we'll assume it
    // still takes const char*.

    // 1. Determine the path to the executable being run.
    std::filesystem::path executable_to_run_path;
#ifdef _WIN32
    char *pgm_path_c_str = nullptr;
    if (_get_pgmptr(&pgm_path_c_str) == 0 && pgm_path_c_str != nullptr)
    {
        executable_to_run_path = pgm_path_c_str;
    }
    else
    {
        // Fallback or error handling if _pgmptr/ _get_pgmptr fails
        // For robustness, you might want a more specific error or default here.
        // For now, if _get_pgmptr fails, executable_to_run_path will be empty,
        // which might lead to further errors but avoids an uninitialized value.
    }
#else
    // Assuming global_gl_executable is a std::string or can be implicitly
    // converted
    executable_to_run_path = global_gl_executable;
#endif

    // 2. Prepare arguments.
    // Ensure 'dir', 'validate_cmdargs', and 'name' are compatible with
    // std::format. If they are char*, std::format will handle them as
    // std::string_view. If they are std::string, that's fine too.

    // 3. Construct the full command string using std::format.
    // This is type-safe and handles memory automatically.
    // // We explicitly quote the executable path to handle spaces in paths
    // correctly.

    std::string command_line = std::format(
        "\"{}\" -W {} {} {}{}",
        executable_to_run_path.string(), // Get string representation for formatting
        dir,
        validate_child_cmdargs,
        name,
        fileExtension);

    // 4. Execute the command using your custom vsystem wrapper.
    // Assuming vsystem expects a C-style string (const char*).
    // unsigned int code = vsystem(command_line.c_str());

    output_debug("Thread %zu acquiring subprocess launch lock",
                 std::hash<std::thread::id>{}(std::this_thread::get_id()));

    unsigned int code;
    int raw = 255;

    {
        std::lock_guard<std::mutex> lock(subprocess_launch_mutex);

        output_debug("Thread %zu launching subprocess: %s",
                     std::hash<std::thread::id>{}(std::this_thread::get_id()),
                     command_line.c_str());

        // code = vsystem(command_line.c_str());

#ifdef _WIN32
        // Windows uses the existing vsystem which wraps std::system
        // code = vsystem(command_line.c_str());

        std::vector<std::string> test_args;
        test_args.push_back(executable_to_run_path.string());
        test_args.push_back("-W");
        test_args.push_back(dir);

        std::string args_str = validate_child_cmdargs;
        std::stringstream ss(args_str);
        std::string token;
        while (ss >> token)
            test_args.push_back(token);

        test_args.push_back(std::format("{}{}", name, fileExtension));

        std::string out_path = std::string(dir) + "/gridlabd.out.pipe";
        std::string err_path = std::string(dir) + "/gridlabd.err.pipe";
        raw = vsystem_posix_exec_argv_capture(test_args, out_path, err_path);
        code = static_cast<unsigned int>(raw);
#else

        // For macOS/Linux: Build a proper argument vector, PREPENDING stdbuf
        std::vector<std::string> test_args;

        // Use stdbuf to force the child process to be unbuffered.
        // This ensures all output is written to the file immediately, even on a
        // crash. on macos, needs brew install coreutils
        // test_args.push_back("gstdbuf");
        // test_args.push_back("-o0");
        // test_args.push_back("-e0"); // stderr unbuffered

        // Now add the original command and its arguments
        test_args.push_back(executable_to_run_path.string());
        test_args.push_back("-W");
        test_args.push_back(dir);

        // Tokenize the child command arguments string and add each part
        std::string args_str = validate_child_cmdargs;
        std::stringstream ss(args_str);
        std::string token;
        while (ss >> token)
        {
            test_args.push_back(token);
        }

        // Finally, add the model file itself
        test_args.push_back(std::format("{}{}", name, fileExtension));

        std::string full_cmd_for_debug;
        for (const auto &arg : test_args)
        {
            full_cmd_for_debug += arg + " ";
        }
        output_debug("Thread %zu launching subprocess: %s",
                     std::hash<std::thread::id>{}(std::this_thread::get_id()),
                     full_cmd_for_debug.c_str());

        std::string out_path = std::string(dir) + "/gridlabd.out.pipe";
        std::string err_path = std::string(dir) + "/gridlabd.err.pipe";
        raw = vsystem_posix_exec_argv_capture(test_args, out_path, err_path);
        code = static_cast<unsigned int>(raw);
#endif
    }

    output_debug("Thread %zu released subprocess launch lock",
                 std::hash<std::thread::id>{}(std::this_thread::get_id()));

    dt = exec_clock() - dt;
    double t = (double)dt / (double)global_ms_per_second;
    if (elapsed_time != nullptr)
        *elapsed_time = t;

    // Handle timeout — treat as a failed test, go through normal cleanup
    if (raw == -2)
    {
        output_error("run_test(char *file='%s'): TIMED OUT after %d seconds", file,
                     DEFAULT_TEST_TIMEOUT_SECONDS);
        result.inc_failed(file, 127, t);
        log_test_error(file, 'T', "Process killed after timeout");

        if (clean && !destroy_dir(dir))
            rmdir(dir);
        return result;
    }

    // Add handling for segfault (signal 11)
    if (WIFSIGNALED(raw) && WTERMSIG(raw) == SIGSEGV)
    {
        output_error("Test crashed with segmentation fault: %s", name);
        result.inc_failed(file, 139,
                          t); // 139 is the typical exit code for SIGSEGV (128 + 11)
        log_test_error(file, 'X', "Segmentation fault (SIGSEGV)");
        return result;
    }

    bool exited = WIFEXITED(code);
    bool signaled = WIFSIGNALED(code);
    // int exit_code = WEXITSTATUS(code);
    int exit_code = exited ? WEXITSTATUS(code) : -1;
    bool problem = false;
    int term_sig = signaled ? WTERMSIG(code) : 0;

    // Short-circuit for _err tests: any non-success exit OR any signal is
    // "expected fail"
    if (is_err && ((exited && exit_code != XC_SUCCESS) || signaled))
    {

        // std::string errs = get_all_error_lines(dir);
        // log_test_error(file, 'E', errs); // use a different flag e.g., 'e' for
        // expected error if you prefer
        output_verbose("%s error was expected (exit=%d, sig=%d) in %.1f seconds",
                       name, exit_code, term_sig, t);

        return result;
    }

    if (exited)
    {
        code = WEXITSTATUS(code);
        output_debug("exit code %d received from %s", code, name);
        if (code == XC_SIGINT) // ctrl-c caught
            return result;
        else if (is_opt) // no expected outcome
        {
            if (code == XC_SUCCESS)
                output_verbose("optional test %s succeeded, code %d in %.1f seconds",
                               name, code, t);
            else if (code == XC_EXCEPTION)
                output_warning("optional test %s exception, code %d in %.1f seconds",
                               name, code, t);
            else
                output_warning("optional test %s error, code %d in %.1f seconds", name,
                               code, t);
        }
        else if (is_exc && code == XC_EXCEPTION) // expected exception and got one
        {
            output_verbose("%s exception was expected, code %d in %.1f seconds", name,
                           code, t);
        }
        else if (is_err && code != XC_SUCCESS) // expected error and got one
        {
            output_verbose("%s error was expected, code %d in %.1f seconds", name,
                           code, t);
            // This is EXPECTED behavior for _err tests, so we don't mark it as a
            // problem
        }
        else if (code == XC_SUCCESS &&
                 !(is_exc || is_err)) // expected success and got it
        {
            output_verbose("%s success was expected, code %d in %.1f seconds", name,
                           code, t);
        }
        else if (code == XC_EXCEPTION) // unexpected exception
        {
            result.inc_exceptions(file, code, t);
            problem = true;
            // std::string last_error = get_all_error_lines(dir) + '\n';
            // log_test_error(file, 'X', last_error);
        }
        else if (code == XC_SUCCESS && is_err) // unexpected success
        {
            output_error("%s succeeded unexpectedly (should have failed), code %d in "
                         "%.1f seconds",
                         name, code, t);
            result.inc_success(file, code, t);
            problem = true;
            // std::string last_error = get_all_error_lines(dir) + '\n';
            // log_test_error(file, 'S', last_error);
        }
        else if (code != XC_SUCCESS && !is_err) // unexpected error
        {
            // A test that should pass actually failed
            result.inc_failed(file, code, t);
            problem = true;
            // std::string last_error = get_all_error_lines(dir) + '\n';
            // log_test_error(file, 'E', last_error);
        }
    }
    else if (signaled && is_err)
    {
        // For MacOS: If process terminated by signal but was expected to fail
        int sig = WTERMSIG(code);
        output_verbose(
            "%s terminated by signal %d, this was an expected error test", name,
            sig);
        // Don't mark as problem since this was an expected error
    }
    else // signaled
    {
        // code = WTERMSIG(code);

        const int sig = WTERMSIG(code);
        const int gld_code = 128 + sig; // GridLAB-D convention for signals

        output_debug("signal %d received from %s", sig, name);
        if (is_opt) // no expected outcome
            output_warning("optional test %s exception, code %d in %.1f seconds",
                           name, sig, t);
        else if (is_exc) // expected exception
            output_warning("%s exception expected, code %d in %.1f seconds", name,
                           sig, t);
        else if (is_err) // expected error - add this condition for macOS
            output_verbose(
                "%s error was expected (terminated by signal %d) in %.1f seconds",
                name, sig, t);
        else
        {
            result.inc_exceptions(file, gld_code, t);
            problem = true;
            // std::string last_error = get_all_error_lines(dir);
            // log_test_error(file, 'X', last_error);
        }
    }

    if (problem)
    {
        std::string last_error = get_all_error_lines(dir);
        log_test_error(file, 'Z', last_error);
    }

    output_debug("run_test(char *file='%s') done", file);
    if (!problem && clean && !destroy_dir(dir))
    {
        output_error("run_test(char *file='%s'): unable to destroy test folder "
                     "after the test",
                     dir);
        result.inc_access(file);
        return result;
    }
    else
    {
        // output_verbose("run_test(): deleted '%s'", dir);
        rmdir(dir);
    }
    return result;
}

/* simple stack to handle directories that need to be processed */
typedef struct s_dirstack
{
    char name[1024];
    unsigned short id;
    struct s_dirstack *next;
} DIRLIST;
static DIRLIST *dirstack = nullptr;
static unsigned short next_id = 0;

// static char *result_code = nullptr;
static std::unique_ptr<std::atomic<char>[]> result_code;

static unsigned int dirlock = 0;
static void pushdir(char *dir)
{
    output_debug("adding %s to process stack", dir);
    DIRLIST *item = (DIRLIST *)malloc(sizeof(DIRLIST));
    strncpy(item->name, dir, sizeof(item->name) - 1);
    // wlock(&dirlock);
    std::unique_lock<std::shared_mutex> lock(
        SharedMutexManager::get_mutex(&dirlock));
    item->next = dirstack;
    item->id = next_id++;
    dirstack = item;
    // wunlock(&dirlock);
}
static void sortlist(void)
{
    bool done = false;
    while (!done)
    {
        DIRLIST *item, *prev = nullptr;
        done = true;
        for (item = dirstack; item != nullptr && item->next != nullptr;
             prev = item, item = item->next)
        {
            DIRLIST *first = item;
            DIRLIST *second = item->next;
            if (strcmp(first->name, second->name) > 0)
            {
                if (prev != nullptr)
                    prev->next = second;
                else
                    dirstack = second;
                first->next = second->next;
                second->next = first;
                done = false;
            }
        }
    }
}

/* popped item must be freed after no longer needed */
static DIRLIST *popdir(void)
{
    // auto v = rlock(&dirlock);
    //  replace the above with SharedMutexManager
    std::unique_lock<std::shared_mutex> lock(
        SharedMutexManager::get_mutex(&dirlock));
    DIRLIST *item = dirstack;
    if (dirstack)
        dirstack = dirstack->next;
    // runlock();
    lock.unlock();
    output_debug("pulling %s from process stack", item->name);
    return item;
}

void *(run_test_proc)(int arg) // *arg)
{
    size_t id = (size_t)arg;
    output_debug("starting run_test_proc id %d", id);
    DIRLIST *item;
    bool passed = true;
    while ((item = popdir()) != nullptr)
    {
        output_debug("process %d picked up '%s'", id, item->name);
        double dt;
        counters result = run_test(item->name, &dt);
        if (result.get_nerrors() > 0)
            passed = false;
        if (global_validateoptions & VO_RPTGLM)
        {
            const char *flags[] = {"", "E", "S", "X"};
            char code = 0;
            if (result.get_nerrors())
                code = 1;
            if (result.get_nsuccess())
                code = 2;
            if (result.get_nexceptions())
                code = 3;
            // result_code[item->id] = code;
            result_code[item->id].store(code);

            char buffer[2048];
            sprintf(buffer, "%s%s%6.1f%s%s", flags[code], report_col, dt, report_col,
                    item->name);
            report_data("%s", buffer);
            report_newrow();
        }
        final += result;
    }
    if (passed)
        final.inc_passed();
    return nullptr;
}

/** routine to process a directory for autotests */
static size_t process_dir(const char *path, bool runglms = false)
{
    // check for block file
    char blockfile[1024];
    sprintf(blockfile, "%s/validate.no", path);
    if (access(blockfile, 00) == 0 && !global_isdefined("force_validate"))
    {
        output_debug(
            "processing directory '%s' blocked by presence of 'validate.no' file",
            path);
        return 0;
    }

    size_t count = 0;
    output_debug("processing directory '%s' with run of GLMs %s", path,
                 runglms ? "enabled" : "disabled");
    counters result;
    final.inc_scanned();
    if (runglms)
        final.inc_tested();
    struct dirent *dp;
    DIR *dirp = opendir(path);
    if (dirp == nullptr)
        return 0; // nothing to do

#ifdef __linux__
    struct stat s;
#endif

    while ((dp = readdir(dirp)) != nullptr)
    {
        char item[1024];
        size_t len = sprintf(item, "%s/%s", path, dp->d_name);
        char *ext = strrchr(item, '.');
        if (dp->d_name[0] == '.')
            continue; // ignore anything that starts with a dot
#ifdef __linux__
        if ((dp->d_type == DT_DIR || (dp->d_type == DT_UNKNOWN &&
                                      !lstat(item, &s) && S_ISDIR(s.st_mode))) &&
            strcmp(dp->d_name, "autotest") == 0)
#else
        if (dp->d_type == DT_DIR && strcmp(dp->d_name, "autotest") == 0)
#endif
        {
            count += process_dir(item, true);
            if (global_validateoptions & VO_RPTDIR)
            {
                report_data();
                report_data("%d", count);
                report_data("%s", item);
                report_newrow();
            }
        }
#ifdef __linux__
        else if (dp->d_type == DT_DIR || (dp->d_type == DT_UNKNOWN &&
                                          !lstat(item, &s) && S_ISDIR(s.st_mode)))
#else
        else if (dp->d_type == DT_DIR)
#endif
            count += process_dir(item);
        else if (runglms == true && strstr(item, "/test_") != 0 && ext != NULL && (strcmp(ext, ".glm") == 0 || strcmp(ext, ".json") == 0))
        {
            pushdir(item);
            count++;
        }
    }
    closedir(dirp);
    // result_code = (char *)malloc(next_id);
    result_code = std::make_unique<std::atomic<char>[]>(next_id);
    for (size_t i = 0; i < next_id; ++i)
        result_code[i].store(0);

    return count;
}

// char *encode_result(char *data, size_t sz)
char *encode_result(std::atomic<char> *data, size_t sz)
{
    char *result = (char *)malloc(sz * 2 + 1); // Correct size
    if (result == NULL)
        return NULL;

    for (size_t i = 0; i < sz; i++)
    {
        static const char hex[] = "0123456789ABCDEF";
        unsigned char val = (unsigned char)data[i].load();
        result[i * 2] = hex[val >> 4];
        result[i * 2 + 1] = hex[val & 0x0F];
    }
    result[sz * 2] = '\0';
    return result;
}

/** main validation routine */
int validate(int argc, char *argv[])
{

    // POSIX-only child signal handling. Windows/MSVC does not provide
    // sigaction/SIGCHLD.
#ifndef _WIN32
#include <signal.h>
    {
        struct sigaction sa{};
        sa.sa_handler = &sigchld_handler; // Set the handler
        sigemptyset(&sa.sa_mask);
        sa.sa_flags =
            SA_RESTART |
            SA_NOCLDSTOP; // restart interrupted calls, ignore stopped children
        if (sigaction(SIGCHLD, &sa, nullptr) == -1)
        {
            perror("sigaction"); // Or use output_error
            return FAILED;
        }
    }
#else
    // Windows: no SIGCHLD/sigaction. Child process handling is done explicitly
    // via CreateProcess/WaitForSingleObject in Windows code paths, and result
    // decoding uses WIFEXITED/WEXITSTATUS/WTERMSIG macros defined above.
#endif

    size_t i;
    int redirect_found = 0;
    strcpy(validate_cmdargs, "");
    strcpy(validate_child_cmdargs, ""); // for each validate test

    // STEP 1: Populate validate_cmdargs with ALL original arguments (for logging
    // purposes)
    for (size_t k = 1; k < argc;
         k++) // Use 'k' to avoid conflict with 'i' in the next loop
    {
        strcat(validate_cmdargs, argv[k]);
        strcat(validate_cmdargs, " ");
    }

    for (i = 1; i < argc; i++)
    {
        // 2. For the 'validate_child_cmdargs' (passed to individual GLM runs)
        // Filter out --threadcount and its value
        if (strcmp(argv[i], "--threadcount") == 0)
        {
            // Skip the current argument (--threadcount) and the next one (its value)
            i++;      // Increment 'i' to skip the value, so next loop iteration starts
                      // after it
            continue; // Do not add --threadcount or its value to child_cmd_args
        }
        // Filter out --validate, as it's for the harness, not individual GLM runs
        if (strcmp(argv[i], "--validate") == 0)
        {
            continue;
        }
        if (strcmp(argv[i], "--redirect") == 0)
            redirect_found = 1;

        strcat(validate_child_cmdargs, argv[i]);
        strcat(validate_child_cmdargs, " ");
    }
    if (!redirect_found)
    {
        strcat(validate_child_cmdargs, " --redirect all");
    }
    strcat(validate_child_cmdargs,
           " --threadcount 1"); // Force single internal thread for each test run

    // In validate(int argc, char *argv[]) after all
    // strcat(validate_child_cmdargs, ...) calls:
    output_message("Final validate_child_cmdargs for child processes: '%s'",
                   validate_child_cmdargs);
    // output_message("Length of validate_child_cmdargs: %zu",
    // strlen(validate_child_cmdargs));

    global_suppress_repeat_messages = 0;
    output_message("Starting validation test in directory '%s'", global_workdir);
    char var[64];
    if (global_getvar("clean", var, sizeof(var)) != nullptr && atoi(var) != 0)
        clean = true;

    report_open();
    if (report_fp == nullptr)
        output_warning("unable to open '%s' for writing", report_file);
    report_title("VALIDATION TEST REPORT");
    report_title("GridLAB-D %d.%d.%d-%d (%s)", global_version_major,
                 global_version_minor, global_version_patch, global_version_build,
                 global_version_branch);

    report_newrow();
    report_newtable("TEST CONFIGURATION");

    char tbuf[64];
    time_t now = time(nullptr);
    struct tm *ts = localtime(&now);
    report_data();
    report_data("Date");
    report_data("%s", strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S %z", ts)
                          ? tbuf
                          : "???");
    report_newrow();

#ifdef _WIN32
    char *user = getenv("USERNAME");
#else
    char *user = getenv("USER");
#endif
    report_data();
    report_data("User");
    report_data("%s", user ? user : "(NA)");
    report_newrow();

#ifdef _WIN32
    char *host = getenv("COMPUTERNAME");
#else
    char *host = getenv("HOSTNAME");
#endif
    report_data();
    report_data("Host");
    report_data("%s", host ? host : "(NA)");
    report_newrow();

    report_data();
    report_data("Platform");
    report_data("%d-bit %s %s", sizeof(void *) * 8, global_platform,
#ifdef _DEBUG
                "DEBUG"
#else
                "RELEASE"
#endif
    );
    report_newrow();

    report_data();
    report_data("Workdir");
    report_data("%s", global_workdir);
    report_newrow();

    report_data();
    report_data("Arguments");
    report_data("%s", validate_cmdargs);
    report_newrow();

    report_data();
    report_data("Clean");
    report_data("%s", clean ? "TRUE" : "FALSE");

    report_newrow();
    report_data();
    report_data("Threads");
    report_data("%d", global_threadcount);
    report_newrow();

    char options[1024] = "";
    report_data();
    report_data("Options");
    report_data("%s", global_getvar("validate", options, sizeof(options))
                          ? options
                          : "(NA)");
    report_newrow();

    char mailto[1024] = "";
    report_data();
    report_data("Mailto");
    report_data("%s", global_getvar("mailto", mailto, sizeof(mailto)) != nullptr
                          ? mailto
                          : "(NA)");
    report_newrow();

    if (global_validateoptions & VO_RPTDIR)
        report_newtable("DIRECTORY SCAN RESULTS");
    process_dir(global_workdir);
    sortlist();

    if (global_validateoptions & VO_RPTGLM)
        report_newtable("FILE TEST RESULTS");
    int n_procs = global_threadcount;
    if (n_procs == 0)
        n_procs = processor_count();
    // n_procs = fmin(final.get_tested(), (unsigned)n_procs);

    // pthread_t *pid = new pthread_t[n_procs];
    // output_debug("starting validation with cmdargs '%s' using %d threads",
    // validate_cmdargs, n_procs); for ( i=0 ; i<n_procs ; i++ )
    //	pthread_create(&pid[i],nullptr,run_test_proc,(void*)i);
    // void *rc;
    // output_debug("begin waiting process");
    // for ( i=0 ; i<n_procs ; i++ )
    //{
    //	pthread_join(pid[i],&rc);
    //	output_debug("process %d done", i);
    // }
    // delete [] pid;

    // Use a vector to store threads
    std::vector<std::thread> threads;
    // n_procs = global_threadcount;
    //  Debug message: starting validation
    // std::cout << "Starting validation with cmdargs '" << validate_cmdargs << "'
    // using "
    //   << n_procs << " threads." << std::endl;

    // Start threads
    for (unsigned int i = 0; i < n_procs; ++i)
    {
        threads.emplace_back([i]()
                             {
      // Cast run_job_proc to the appropriate type and convert i to int
      using FuncType = void *(*)(int);
      FuncType func = run_test_proc;
      try {
        func(static_cast<int>(i)); // Convert size_t to int
      } catch (const std::exception &e) {
        output_error("Thread %d exception: %s", i, e.what());
      } catch (...) {
        output_error("Thread %d unknown exception", i);
      } });
    }

    // Debug message: waiting for threads
    std::cout << "Begin waiting for threads to complete." << std::endl;

    // Wait for all threads to complete
    for (unsigned int i = 0; i < n_procs; ++i)
    {
        if (threads[i].joinable())
        {
            threads[i]
                .join(); // Join the thread
                         // std::cout << "Thread " << i << " is done." << std::endl;
        }
    }

    std::cout << "Validation complete.\n";

    final.print();
    double dt = (double)exec_clock() / global_ms_per_second;
    output_message("Total validation elapsed time: %.1f seconds", dt);

    if (report_fp)
    {
        // Recompute the same values printed by final.print()
        const unsigned int n_files = final.get_nfiles();
        const unsigned int n_success = final.get_nsuccess();
        const unsigned int n_failed = final.get_nfailed();
        const unsigned int n_exceptions = final.get_nexceptions();
        const unsigned int n_access = final.get_naccess();
        const unsigned int n_ok =
            (n_files >= (n_success + n_failed + n_exceptions))
                ? (n_files - n_success - n_failed - n_exceptions)
                : 0;
        const double rate = (n_files != 0) ? (100.0 * n_ok / n_files) : 0.0;

        // Mirror the terminal summary into validate.txt
        fprintf(report_fp, "\nValidation report:\n");
        if (n_access)
            fprintf(report_fp, "%u directory access failures\n", n_access);
        fprintf(report_fp, "%u models tested\n", n_files);
        if (n_files != 0)
        {
            if (n_success)
                fprintf(report_fp, "%u unexpected successes\n", n_success);
            if (n_failed)
                fprintf(report_fp, "%u unexpected errors\n", n_failed);
            if (n_exceptions)
                fprintf(report_fp, "%u unexpected exceptions\n", n_exceptions);
            fprintf(report_fp, "%u tests succeeded\n", n_ok);
            fprintf(report_fp, "%.0f%% success rate\n", rate);
        }
        // Also include elapsed time like stdout
        fprintf(report_fp, "Total validation elapsed time: %.1f seconds\n", dt);
        fflush(report_fp);
    }

    if (report_fp)
        output_message("See '%s/%s' for details", global_workdir, report_file);
    if (final.get_nerrors() == 0)
        exec_setexitcode(XC_SUCCESS);
    else
        exec_setexitcode(XC_TSTERR);

    report_newtable("OVERALL RESULTS");

    const char *flag = "!!!";
    report_data();
    report_data("Directory results");
    report_newrow();

    report_data();
    report_data();
    report_data("Scanned");
    report_data("%d", final.get_scanned());
    report_newrow();

    report_data("%s", final.get_naccess() ? flag : "");
    report_data();
    report_data("Denied");
    report_data("%d", final.get_naccess());
    report_newrow();

    report_data();
    report_data();
    report_data("Tested");
    report_data("%d", final.get_tested());
    report_newrow();

    int n_failed = final.get_tested() - final.get_passed();
    report_data("%s", n_failed ? flag : "");
    report_data();
    report_data("Failed");
    report_data("%d", n_failed);
    report_newrow();

    report_data();
    report_data("File results");
    report_newrow();

    report_data();
    report_data();
    report_data("Tested");
    report_data("%d", final.get_nfiles());
    report_newrow();

    report_data();
    report_data();
    report_data("Passed");
    report_data("%d", final.get_nfiles() - final.get_nerrors());
    report_newrow();

    report_data("%s", final.get_nerrors() ? flag : "");
    report_data();
    report_data("Failed");
    report_data("%d", final.get_nerrors());
    report_newrow();

    report_data();
    report_data("Unexpected results");
    report_newrow();

    report_data("%s", final.get_nsuccess() ? flag : "");
    report_data();
    report_data("Successes");
    report_data("%d", final.get_nsuccess());
    report_newrow();

    report_data("%s", final.get_nfailed() ? flag : "");
    report_data();
    report_data("Errors");
    report_data("%d", final.get_nfailed());
    report_newrow();

    report_data("%s", final.get_nexceptions() ? flag : "");
    report_data();
    report_data("Exceptions");
    report_data("%d", final.get_nexceptions());
    report_newrow();

    report_data();
    report_data("Runtime");
    report_data("%.1f s", dt);
    report_newrow();

    report_data();
    report_data("Result code");
    report_data("%s", encode_result(result_code.get(), next_id));
    report_newrow();

    report_newrow();
    report_title("END TEST REPORT");
    report_newrow();

    fclose(report_fp);

    // At the end of validation (where report_fp is closed)
    if (error_log_fp)
    {
        fclose(error_log_fp);
        error_log_fp = nullptr;
        output_message("See '%s/validate_errors.txt' for error details",
                       global_workdir);
    }

#ifndef WIN32
#ifdef __APPLE__
#define MAILER "/usr/bin/mail"
#else
#define MAILER "/bin/mail"
#endif
    if (strcmp(mailto, "") != 0)
    {
        if (vsystem(MAILER " -s 'GridLAB-D Validation Report (%d errors)' %s <%s",
                    final.get_nerrors(), mailto, report_file) == 0)
            output_verbose("Mail message send to %s", mailto);
        else
            output_error("Error sending notification to %s", mailto);
    }
#endif

    exit(final.get_nerrors() == 0 ? XC_SUCCESS : XC_TSTERR);
}
