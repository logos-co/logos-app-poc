// srcdeps: utils/LogSink.cpp
// libdeps: spdlog::spdlog
//
// Unit tests for the session log sink (app/utils/LogSink.*) — the rotating
// capture of everything this process and its module hosts write to
// stdout/stderr.
//
// The behaviours worth protecting:
//
//   * Capture is pipe-based, so a subprocess that inherited the descriptors
//     lands in the log too. Only the local half is exercised here; spawning a
//     module host is the smoke test's job.
//   * Retention bounds the DIRECTORY, not one launch's rotations. Every launch
//     opens a new stamped name, so spdlog's own max_files would leave one file
//     per launch forever — which is exactly what the line-counting redirector
//     this replaced did.
//   * The configured name survives as a symlink to the current file, so
//     `tail -F logs/basecamp.log` follows across restarts.
//
// Every test redirects this process's own stdout, so each one holds a
// SinkGuard: a QVERIFY that fails mid-capture returns immediately, and without
// the guard the rest of the run's output would vanish into the log file.
//
// Run: nix build .#unit-tests -L

#include "../app/utils/LogSink.h"

#include <QtTest/QtTest>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include <chrono>
#include <cstdio>
#include <thread>

using LogosBasecampLog::LogSink;

namespace {

// Restores stdout however the test leaves the stack.
struct SinkGuard {
    ~SinkGuard() { LogSink::instance().stop(); }
};

QString readAll(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(f.readAll());
}

// The stable name is a symlink into the same directory, so counting it would
// inflate every retention assertion by one.
int countLogFiles(const QString& dir, const QString& stem)
{
    int n = 0;
    const QFileInfoList entries = QDir(dir).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& e : entries) {
        if (e.isSymLink()) continue;
        if (e.fileName().startsWith(stem)) ++n;
    }
    return n;
}

} // namespace

class LogSinkTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

    QString logsDir() const { return QDir(m_tmp.path()).filePath("logs"); }

    LogSink::Options options() const
    {
        LogSink::Options o;
        o.dir = logsDir();
        o.console = false;   // don't spray the test runner's own output
        return o;
    }

private slots:
    void init()
    {
        QVERIFY(m_tmp.isValid());
        QDir(logsDir()).removeRecursively();
    }

    void cleanup() { LogSink::instance().stop(); }

    // Anything written to stdout or stderr has to reach the file. That is the
    // whole reason capture is pipe-based rather than a plain redirect.
    void capturesStdoutAndStderr()
    {
        {
            SinkGuard guard;
            QVERIFY(LogSink::instance().start(options()));
            std::printf("hello-from-stdout\n");
            std::fprintf(stderr, "hello-from-stderr\n");
            std::fflush(stdout);
            std::fflush(stderr);
        }   // stop() drains the reader before anything below reads the file

        const QString body = readAll(QDir(logsDir()).filePath("basecamp.log"));
        QVERIFY(body.contains(QStringLiteral("hello-from-stdout")));
        QVERIFY(body.contains(QStringLiteral("hello-from-stderr")));
    }

    // Lines arriving from the pipe already carry liblogos's own timestamp and
    // level. The sink must not stamp them a second time.
    void doesNotRestampAlreadyFormattedLines()
    {
        {
            SinkGuard guard;
            QVERIFY(LogSink::instance().start(options()));
            std::printf("[2026-09-09 16:38:32.715] [info] [logos] already-formatted\n");
            std::fflush(stdout);
        }

        const QString body = readAll(QDir(logsDir()).filePath("basecamp.log"));
        QCOMPARE(body.split(QLatin1Char('\n')).value(0),
                 QStringLiteral("[2026-09-09 16:38:32.715] [info] [logos] already-formatted"));
    }

    // The real file carries a start-time stamp, and the configured name
    // survives as a symlink to it.
    void stampsTheFileAndLinksTheStableName()
    {
        QString real;
        {
            SinkGuard guard;
            QVERIFY(LogSink::instance().start(options()));
            real = LogSink::instance().currentFile();
            std::printf("marker-line\n");
            std::fflush(stdout);
        }

        const QString base = QFileInfo(real).fileName();
        QVERIFY2(base != QStringLiteral("basecamp.log"), qPrintable(base));
        QVERIFY2(base.startsWith(QStringLiteral("basecamp_")), qPrintable(base));
        QVERIFY2(base.endsWith(QStringLiteral(".log")), qPrintable(base));
        QCOMPARE(base.size(), QStringLiteral("basecamp_20260909_163832.log").size());

        const QString link = LogSink::stablePath(logsDir(), QStringLiteral("basecamp.log"));
        QVERIFY(QFileInfo(link).isSymLink());
        QVERIFY2(readAll(link).contains(QStringLiteral("marker-line")),
                 "the stable name must resolve to this session's file");
    }

    // A long-lived app must not grow its log without bound, nor keep every
    // rotation forever.
    void rotatesAtTheSizeCapAndKeepsOnlyMaxFiles()
    {
        {
            SinkGuard guard;
            LogSink::Options o = options();
            o.maxSizeMb = 1;   // the smallest cap the schema allows
            o.maxFiles = 3;    // live + 2 rotated
            QVERIFY(LogSink::instance().start(o));

            // Comfortably past 3 MB, so rotation happens more than maxFiles
            // times and the oldest files have to be dropped.
            const std::string chunk(512, 'x');
            for (int i = 0; i < 8000; ++i)
                std::printf("%06d %s\n", i, chunk.c_str());
            std::fflush(stdout);
        }

        const int files = countLogFiles(logsDir(), QStringLiteral("basecamp_"));
        QVERIFY2(files > 1, "expected the log to have rotated");
        QVERIFY2(files <= 3, qPrintable(QStringLiteral("retention kept %1 files")
                                            .arg(files)));

        const QFileInfoList entries =
            QDir(logsDir()).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        for (const QFileInfo& e : entries) {
            if (e.isSymLink()) continue;
            // Slack of one line: the cap is checked per write.
            QVERIFY2(e.size() < 2 * 1024 * 1024,
                     qPrintable(e.fileName() + QStringLiteral(" ignored the size cap")));
        }
    }

    // spdlog's own max_files prunes one sink's rotation set, and every launch
    // opens a new stamped name, so retention has to prune across launches too.
    void retentionPrunesAcrossSessions()
    {
        for (int session = 0; session < 3; ++session) {
            {
                SinkGuard guard;
                LogSink::Options o = options();
                o.maxFiles = 2;
                QVERIFY(LogSink::instance().start(o));
                std::printf("session %d\n", session);
                std::fflush(stdout);
            }
            // Stamps have one-second resolution, so without this two sessions
            // collide on one filename and the test proves nothing.
            if (session < 2)
                std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        }

        QCOMPARE(countLogFiles(logsDir(), QStringLiteral("basecamp_")), 2);

        // The stable name must survive pruning and still resolve.
        const QString link = LogSink::stablePath(logsDir(), QStringLiteral("basecamp.log"));
        QVERIFY(QFileInfo(link).isSymLink());
        QVERIFY(QFileInfo(link).exists());
    }

    // Disabled logging is a valid configuration, not an error, and must leave
    // stdout alone.
    void disabledWritesNothingAndSucceeds()
    {
        LogSink::Options o = options();
        o.enabled = false;
        QVERIFY(LogSink::instance().start(o));
        QVERIFY(LogSink::instance().currentFile().isEmpty());
        QVERIFY(!QFileInfo::exists(QDir(logsDir()).filePath("basecamp.log")));
    }

    // start() creates the directory; a caller should not have to.
    void createsTheLogsDirectory()
    {
        QVERIFY(!QDir(logsDir()).exists());
        {
            SinkGuard guard;
            QVERIFY(LogSink::instance().start(options()));
        }
        QVERIFY(QDir(logsDir()).exists());
    }
};

QTEST_MAIN(LogSinkTest)
#include "log_sink_test.moc"
