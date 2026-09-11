#include "LogSink.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include <fcntl.h>

#ifdef Q_OS_WIN
#  include <io.h>
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace LogosBasecampLog {
namespace {

// std::filesystem's narrow constructor reads bytes in the native ANSI code page
// on Windows, so a UTF-8 QString would be mangled for any non-ASCII profile
// name. The wide form is the faithful one there.
fs::path toPath(const QString& s)
{
#ifdef Q_OS_WIN
    return fs::path(s.toStdWString());
#else
    return fs::path(s.toStdString());
#endif
}

// pipe(2). Windows' CRT spells it _pipe and wants a size and a mode; _O_BINARY
// because this carries arbitrary bytes and CRLF translation would corrupt them.
int makePipe(int fds[2])
{
#ifdef Q_OS_WIN
    return ::_pipe(fds, 64 * 1024, _O_BINARY);
#else
    return ::pipe(fds);
#endif
}

struct NameParts { QString stem, ext; };

NameParts split(const QString& file)
{
    const QFileInfo info(file);
    return { info.completeBaseName(), info.suffix().isEmpty() ? QString()
                                                              : "." + info.suffix() };
}

// Keep at most `keep` log files in `dir`, deleting the oldest first.
//
// spdlog's own max_files prunes only within one sink's rotation set, and each
// launch opens a new stamped base name -- so without this an app started a
// hundred times leaves a hundred logs behind, which is exactly what the
// line-counting redirector this replaced used to do.
void pruneOldLogs(const QString& dir, const NameParts& n, std::size_t keep)
{
    struct Entry { fs::path path; fs::file_time_type when; };
    std::vector<Entry> found;

    // Compared in fs::path's own encoding: path::string() would narrow through
    // the ANSI code page on Windows, which can throw on an unconvertible name.
    const auto stemPrefix = toPath(n.stem + "_").native();
    const auto ext = toPath(n.ext).native();

    std::error_code ec;
    for (const auto& e : fs::directory_iterator(toPath(dir), ec)) {
        if (ec) break;
        if (!e.is_regular_file(ec)) continue;
        // Only our own stamped files: "<stem>_...". The stable name is
        // "<stem><ext>" with no underscore, so it is never a candidate.
        if (e.path().filename().native().rfind(stemPrefix, 0) != 0) continue;
        if (!ext.empty() && e.path().extension().native() != ext) continue;
        std::error_code tec;
        const auto when = fs::last_write_time(e.path(), tec);
        if (tec) continue;
        found.push_back({ e.path(), when });
    }

    if (found.size() <= keep) return;
    std::sort(found.begin(), found.end(),
              [](const Entry& a, const Entry& b) { return a.when < b.when; });
    for (std::size_t i = 0; i + keep < found.size(); ++i) {
        std::error_code rec;
        fs::remove(found[i].path, rec);
    }
}

} // namespace

LogSink& LogSink::instance()
{
    static LogSink s;
    return s;
}

LogSink::~LogSink()
{
    stop();
}

QString LogSink::stablePath(const QString& dir, const QString& file)
{
    return QDir(dir).filePath(file);
}

QString LogSink::currentFile() const
{
    return m_path;
}

bool LogSink::start(const Options& opts)
{
    if (m_started)
        return true;
    if (!opts.enabled)
        return true;   // disabled is a valid configuration, not a failure

    if (!QDir().mkpath(opts.dir))
        return false;

    const NameParts parts = split(opts.file);

    // Make room before opening the new file, so the count after startup is the
    // configured maximum rather than one over it.
    if (opts.maxFiles > 0)
        pruneOldLogs(opts.dir, parts, opts.maxFiles - 1);

    const QString stamped = parts.stem + "_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + parts.ext;
    m_path = QDir(opts.dir).filePath(stamped);
    m_linkPath = stablePath(opts.dir, opts.file);
    m_console = opts.console;

    try {
        // maxSizeMb == 0 means "never rotate": spdlog has no such mode, so
        // approximate it with a cap large enough to be unreachable rather than
        // silently imposing a limit the operator declined.
        const std::size_t maxBytes = opts.maxSizeMb == 0
            ? std::size_t(1) << 40
            : opts.maxSizeMb * 1024ull * 1024ull;
        // maxFiles is the total kept; spdlog counts rotated files besides the
        // live one, so subtract it.
        const std::size_t rotated = opts.maxFiles > 1 ? opts.maxFiles - 1 : 0;

        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            m_path.toStdString(), maxBytes, rotated);
        // Lines arriving from the pipe already carry liblogos's timestamp and
        // level; stamping them again would double every prefix.
        sink->set_pattern("%v");
        auto logger = std::make_shared<spdlog::logger>("basecamp-session", sink);
        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::info);
        m_logger = logger;
    } catch (const std::exception&) {
        m_path.clear();
        m_linkPath.clear();
        return false;
    }

    // Point the stable name at this session's file, so anyone tailing it gets
    // one path that is always current. Best-effort: a filesystem without
    // symlinks (or Windows without Developer Mode) costs the convenience, not
    // the logging.
    if (!m_linkPath.isEmpty()) {
        std::error_code lec;
        fs::remove(toPath(m_linkPath), lec);
        fs::create_symlink(toPath(stamped), toPath(m_linkPath), lec);
    }

    auto cleanup = [this]() {
        if (m_originalStdout >= 0) { ::close(m_originalStdout); m_originalStdout = -1; }
        if (m_originalStderr >= 0) { ::close(m_originalStderr); m_originalStderr = -1; }
        m_logger.reset();
        m_path.clear();
        m_linkPath.clear();
    };

#ifdef Q_OS_WIN
    // A GUI-subsystem process started without a console has no stdio at all:
    // the CRT leaves stdout/stderr at _NO_CONSOLE_FILENO (-2), so the dup below
    // fails, start() returns false and the app runs with NO file logging --
    // silently, because the only channel that could have said so is the one
    // that is missing. Bind them to NUL so the redirect has a real fd to take
    // over; the mirror at the bottom of the reader then writes to NUL, which is
    // where a console-less process's terminal output was going anyway.
    if (::_fileno(stdout) < 0) ::freopen("NUL", "w", stdout);
    if (::_fileno(stderr) < 0) ::freopen("NUL", "w", stderr);
#endif

    m_originalStdout = ::dup(fileno(stdout));
    m_originalStderr = ::dup(fileno(stderr));
    if (m_originalStdout < 0 || m_originalStderr < 0) {
        cleanup();
        return false;
    }

    int fds[2];
    if (makePipe(fds) != 0) {
        cleanup();
        return false;
    }

    std::fflush(stdout);
    std::fflush(stderr);

    if (::dup2(fds[1], fileno(stdout)) == -1 ||
        ::dup2(fds[1], fileno(stderr)) == -1) {
        ::close(fds[0]);
        ::close(fds[1]);
        cleanup();
        return false;
    }

#ifdef Q_OS_WIN
    // The dup2 above rebinds only the CRT's fd table. A CHILD PROCESS does not
    // inherit that table: CreateProcess passes the STARTUPINFO std handles,
    // which default to the parent's STD_OUTPUT_HANDLE / STD_ERROR_HANDLE --
    // still the console. Capturing logos_host and ui-host output is the entire
    // reason capture is pipe-based, so leaving these alone would silently
    // defeat it.
    //
    // Take the handles from fd 1 / fd 2 AFTER the dup2, not from fds[1]: _dup2
    // duplicates the underlying OS handle into the target slot, so these stay
    // valid after the ::close(fds[1]) below, whereas fds[1]'s handle does not.
    m_originalStdoutHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    m_originalStderrHandle = ::GetStdHandle(STD_ERROR_HANDLE);
    ::SetStdHandle(STD_OUTPUT_HANDLE,
                   reinterpret_cast<HANDLE>(::_get_osfhandle(_fileno(stdout))));
    ::SetStdHandle(STD_ERROR_HANDLE,
                   reinterpret_cast<HANDLE>(::_get_osfhandle(_fileno(stderr))));
#endif

    // Line-buffer so a line reaches the reader as soon as it is complete
    // rather than sitting in a 4K buffer; without this a crash loses the last,
    // most interesting output.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    std::setvbuf(stderr, nullptr, _IOLBF, 0);

    ::close(fds[1]);
    m_readFd = fds[0];

    m_running = true;
    m_readerThread = std::thread(&LogSink::readerLoop, this);
    m_started = true;
    return true;
}

void LogSink::readerLoop()
{
    auto logger = std::static_pointer_cast<spdlog::logger>(m_logger);
    std::string pending;
    std::vector<char> buf(4096);

    for (;;) {
        // `auto`, not ssize_t: the CRT's read() returns int and takes an
        // unsigned count, so the POSIX signature cannot be spelled here.
        const auto n = ::read(m_readFd, buf.data(),
                              static_cast<unsigned int>(buf.size()));
        if (n > 0) {
            // Mirror first: if the process dies mid-write, the terminal has
            // already seen it.
            if (m_console && m_originalStdout >= 0) {
                const auto ignored = ::write(m_originalStdout, buf.data(),
                                             static_cast<unsigned int>(n));
                (void)ignored;
            }
            pending.append(buf.data(), static_cast<std::size_t>(n));

            // The sink appends its own newline, so hand it bare lines and hold
            // any partial tail until the rest arrives.
            std::size_t start = 0;
            for (std::size_t i = 0; i < pending.size(); ++i) {
                if (pending[i] != '\n') continue;
                if (logger)
                    logger->info(pending.substr(start, i - start));
                start = i + 1;
            }
            pending.erase(0, start);
            // A producer that never emits a newline would otherwise grow this
            // without bound; flush it as its own line well before that.
            if (pending.size() > 64 * 1024) {
                if (logger) logger->info(pending);
                pending.clear();
            }
        } else if (n == 0) {
            break;                        // write ends closed
        } else if (errno != EINTR) {
            break;
        }
    }

    if (!pending.empty() && logger)
        logger->info(pending);
    if (logger)
        logger->flush();
}

void LogSink::stop()
{
    if (!m_started)
        return;
    m_running = false;

    std::fflush(stdout);
    std::fflush(stderr);

    // dup2 back closes the pipe's write end held by stdout/stderr; once both
    // are restored the reader sees EOF and returns. The duplicated originals
    // stay open until it has finished, because readerLoop may still be
    // mirroring to m_originalStdout and closing it here would race fd reuse.
    if (m_originalStdout >= 0) ::dup2(m_originalStdout, fileno(stdout));
    if (m_originalStderr >= 0) ::dup2(m_originalStderr, fileno(stderr));

#ifdef Q_OS_WIN
    // Put the Win32 std handles back in the same breath as the fds: the ones
    // installed in start() are those fd 1 / fd 2 own, so the dup2 above has
    // just closed them, leaving STD_OUTPUT_HANDLE dangling for anything (a
    // child spawned during shutdown) that reads it.
    if (m_originalStdoutHandle) {
        ::SetStdHandle(STD_OUTPUT_HANDLE, m_originalStdoutHandle);
        m_originalStdoutHandle = nullptr;
    }
    if (m_originalStderrHandle) {
        ::SetStdHandle(STD_ERROR_HANDLE, m_originalStderrHandle);
        m_originalStderrHandle = nullptr;
    }
#endif

    if (m_readerThread.joinable())
        m_readerThread.join();

    if (m_originalStdout >= 0) { ::close(m_originalStdout); m_originalStdout = -1; }
    if (m_originalStderr >= 0) { ::close(m_originalStderr); m_originalStderr = -1; }
    if (m_readFd >= 0)         { ::close(m_readFd);         m_readFd = -1; }

    m_logger.reset();
    m_path.clear();
    m_linkPath.clear();
    m_started = false;
}

} // namespace LogosBasecampLog
