#pragma once

#include <QString>

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>

namespace LogosBasecampLog {

// Captures everything this process and its child processes (logos_host,
// ui-host) write to stdout/stderr into a rotating log file under the session
// directory.
//
// Capture is pipe-based rather than a file redirect. The module hosts are
// separate processes holding inherited descriptors, and renaming a file out
// from under a child that still has it open just keeps filling the old inode --
// so a redirect would catch their output but make rotation impossible. A pipe
// puts one reader in charge of the file, which makes rotation safe while
// subprocess output still lands in it.
//
// The size cap and retention come from spdlog's rotating sink, which liblogos
// already logs through. Lines arriving from the pipe carry their own timestamp
// and level, so they are written verbatim rather than stamped twice.
//
// Ported from logosctl's daemon log sink (logos-logoscore-cli,
// src/daemon/log_sink.h) so the two frontends log the same way.
class LogSink {
public:
    struct Options {
        bool enabled = true;
        QString dir;                                    // resolved logs directory
        // Names the log. The real file gets a start-time stamp inserted --
        // "basecamp.log" becomes "basecamp_20260909_163832.log" -- and this
        // exact name survives as a symlink to whichever file is current, so
        // `tail -F logs/basecamp.log` keeps working across restarts.
        QString file = QStringLiteral("basecamp.log");
        // Rotate once the file passes this size, keeping maxFiles in total
        // (the live one plus maxFiles-1 older). 0 disables rotation.
        std::size_t maxSizeMb = 10;
        std::size_t maxFiles = 5;
        // Mirror to the terminal that launched the app. A no-op for a bundle
        // started from Finder, which has nowhere to mirror to.
        bool console = true;
    };

    static LogSink& instance();

    // False when the log file could not be opened; the caller should warn and
    // carry on, because an app that cannot write a log is still a working app.
    bool start(const Options& opts);

    // Restore the original stdout/stderr and drain the reader. Safe to call
    // when never started.
    void stop();

    // This session's timestamped file, or empty when not started.
    QString currentFile() const;

    // The stable name symlinked to it (<dir>/<file>). Static so callers can
    // name it before the sink has opened anything.
    static QString stablePath(const QString& dir, const QString& file);

    LogSink(const LogSink&) = delete;
    LogSink& operator=(const LogSink&) = delete;

private:
    LogSink() = default;
    ~LogSink();

    void readerLoop();

    std::shared_ptr<void> m_logger;   // spdlog::logger, type-erased to keep
                                      // spdlog out of this header
    QString m_path;      // timestamped file actually written
    QString m_linkPath;  // stable name pointing at it

    int m_readFd = -1;
    int m_originalStdout = -1;
    int m_originalStderr = -1;

    // Windows only: the STD_OUTPUT_HANDLE / STD_ERROR_HANDLE in place before
    // start() repointed them at the pipe. Held as void* to keep windows.h out
    // of this header; they are HANDLEs. Unused on POSIX, where dup2 is the
    // whole story.
    void* m_originalStdoutHandle = nullptr;
    void* m_originalStderrHandle = nullptr;

    std::thread m_readerThread;
    std::atomic<bool> m_running{false};
    bool m_started = false;
    bool m_console = true;
};

} // namespace LogosBasecampLog
