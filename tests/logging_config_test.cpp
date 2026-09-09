// srcdeps: utils/LoggingConfig.cpp
// libdeps: yaml-cpp::yaml-cpp
//
// Unit tests for the session logging configuration (app/utils/LoggingConfig.*)
// — the `logging:` block of <session>/config.yaml, which the app reads before
// it opens its log file.
//
// Two rules here are load-bearing and easy to regress:
//
//   * A document that cannot be read is not applied AT ALL. Half-applying it
//     leaves someone running an app whose logging does not match the file they
//     are looking at, which is worse than plainly ignoring the file and saying
//     so.
//   * A typo is reported, never silently dropped. `max_size` (the key is
//     `max_size_mb`) has to produce a message naming the key, because the
//     alternative is an operator whose intent never took effect and who has
//     nothing to go on.
//
// YAML scalars are untyped text, so the quoting rules get their own cases: an
// unquoted 42 where a name belongs is a mistake worth reporting, and a quoted
// "42" is a name.
//
// Run: nix build .#unit-tests -L

#include "../app/utils/LoggingConfig.h"

#include <QtTest/QtTest>
#include <QDir>
#include <QTemporaryDir>

using LogosBasecampLog::ConfigLoad;
using LogosBasecampLog::parseConfig;

class LoggingConfigTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_session;

    QString sessionDir() const { return m_session.path(); }

    ConfigLoad parse(const QString& text) const
    {
        return parseConfig(text, sessionDir());
    }

    // The whole point of a diagnostic is that it names the key. Asserting on
    // the key rather than the whole sentence keeps the wording free to change.
    static bool mentions(const ConfigLoad& c, const QString& needle)
    {
        for (const QString& d : c.diagnostics)
            if (d.contains(needle)) return true;
        return false;
    }

    static QString allDiagnostics(const ConfigLoad& c)
    {
        return c.diagnostics.join(QStringLiteral(" | "));
    }

private slots:
    void initTestCase() { QVERIFY(m_session.isValid()); }

    // ── Nothing to read is the common case, not a problem ───────────────────

    void absentDocumentYieldsWorkingDefaults()
    {
        const auto c = LogosBasecampLog::loadConfig(sessionDir());
        QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
        QVERIFY(c.logging.enabled);
        QCOMPARE(c.logging.file, QStringLiteral("basecamp.log"));
        QCOMPARE(c.logging.dir, QDir(sessionDir()).filePath("logs"));
        QCOMPARE(c.logging.maxSizeMb, std::size_t(10));
        QCOMPARE(c.logging.maxFiles, std::size_t(5));
        QVERIFY(c.logging.console);
    }

    void emptyDocumentYieldsDefaults()
    {
        for (const QString& text : {QString(), QStringLiteral("\n"),
                                    QStringLiteral("# just a comment\n")}) {
            const auto c = parse(text);
            QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
            QVERIFY(c.logging.enabled);
            QCOMPARE(c.logging.dir, QDir(sessionDir()).filePath("logs"));
        }
    }

    void configPathIsInTheSessionRoot()
    {
        QCOMPARE(LogosBasecampLog::configPath(sessionDir()),
                 QDir(sessionDir()).filePath("config.yaml"));
    }

    // ── A document that reads cleanly is applied whole ──────────────────────

    void everyKeyIsApplied()
    {
        const auto c = parse(
            "version: 1\n"
            "logging:\n"
            "  enabled: true\n"
            "  file: app.log\n"
            "  max_size_mb: 25\n"
            "  max_files: 3\n"
            "  console: false\n");
        QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
        QVERIFY(c.logging.enabled);
        QCOMPARE(c.logging.file, QStringLiteral("app.log"));
        QCOMPARE(c.logging.maxSizeMb, std::size_t(25));
        QCOMPARE(c.logging.maxFiles, std::size_t(3));
        QVERIFY(!c.logging.console);
    }

    void loggingCanBeTurnedOff()
    {
        const auto c = parse("logging:\n  enabled: false\n");
        QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
        QVERIFY(!c.logging.enabled);
    }

    // `key:` with nothing after it is null, which means "not set".
    void emptyValueMeansNotSet()
    {
        const auto c = parse("logging:\n  enabled:\n  file:\n");
        QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
        QVERIFY(c.logging.enabled);
        QCOMPARE(c.logging.file, QStringLiteral("basecamp.log"));
    }

    void versionMayBeOmittedOrOne()
    {
        for (const QString& text : {QStringLiteral("logging:\n  max_files: 2\n"),
                                    QStringLiteral("version: 1\nlogging:\n  max_files: 2\n")}) {
            const auto c = parse(text);
            QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
            QCOMPARE(c.logging.maxFiles, std::size_t(2));
        }
    }

    // ── Where the logs go ──────────────────────────────────────────────────

    void relativeDirStaysInsideTheSession()
    {
        const auto c = parse("logging:\n  dir: logs-custom\n");
        QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
        QCOMPARE(c.logging.dir, QDir(sessionDir()).filePath("logs-custom"));
    }

    void absoluteDirIsUsedVerbatim()
    {
        const QString absolute = QDir::tempPath() + QStringLiteral("/basecamp-logs-test");
        const auto c = parse(QStringLiteral("logging:\n  dir: %1\n").arg(absolute));
        QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
        QCOMPARE(c.logging.dir, absolute);
    }

    void tildeDirResolvesAgainstHome()
    {
        const auto c = parse("logging:\n  dir: ~/basecamp-logs\n");
        QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
        QCOMPARE(c.logging.dir, QDir::homePath() + QStringLiteral("/basecamp-logs"));
    }

    // ── Quoting decides the type ───────────────────────────────────────────

    void quotedNumberIsAName()
    {
        const auto c = parse("logging:\n  file: \"42\"\n");
        QVERIFY2(c.diagnostics.isEmpty(), qPrintable(allDiagnostics(c)));
        QCOMPARE(c.logging.file, QStringLiteral("42"));
    }

    void unquotedNumberIsNotAName()
    {
        const auto c = parse("logging:\n  file: 42\n");
        QVERIFY(mentions(c, QStringLiteral("logging.file")));
        QCOMPARE(c.logging.file, QStringLiteral("basecamp.log"));
    }

    void quotedNumberIsNotACount()
    {
        const auto c = parse("logging:\n  max_files: \"5\"\n");
        QVERIFY(mentions(c, QStringLiteral("logging.max_files")));
        QCOMPARE(c.logging.maxFiles, std::size_t(5));   // the default, not the value
    }

    // ── A document that cannot be read is not applied at all ───────────────

    void aWrongTypeDiscardsTheWholeBlock()
    {
        // `console: false` reads cleanly, but the document as a whole does not,
        // so it must NOT be applied on its own.
        const auto c = parse(
            "logging:\n"
            "  console: false\n"
            "  max_size_mb: big\n");
        QVERIFY(mentions(c, QStringLiteral("logging.max_size_mb")));
        QVERIFY2(c.logging.console,
                 "a document that failed to read must not be half-applied");
        QCOMPARE(c.logging.maxSizeMb, std::size_t(10));
    }

    void outOfRangeNumberIsReportedWithItsRange()
    {
        const auto c = parse("logging:\n  max_size_mb: -1\n");
        QVERIFY(mentions(c, QStringLiteral("logging.max_size_mb")));
        QVERIFY(mentions(c, QStringLiteral("between")));
        QCOMPARE(c.logging.maxSizeMb, std::size_t(10));
    }

    void loggingMustBeAMapping()
    {
        const auto c = parse("logging: true\n");
        QVERIFY(mentions(c, QStringLiteral("logging")));
        QVERIFY(c.logging.enabled);
    }

    void documentMustBeAMapping()
    {
        const auto c = parse("- one\n- two\n");
        QVERIFY(!c.diagnostics.isEmpty());
        QCOMPARE(c.logging.file, QStringLiteral("basecamp.log"));
    }

    void malformedYamlIsReportedNotThrown()
    {
        const auto c = parse("logging:\n  file: [unterminated\n");
        QVERIFY(!c.diagnostics.isEmpty());
        QVERIFY(c.logging.enabled);
        QCOMPARE(c.logging.dir, QDir(sessionDir()).filePath("logs"));
    }

    void anUnreadableVersionIsRefused()
    {
        const auto c = parse("version: 2\nlogging:\n  max_files: 1\n");
        QVERIFY(mentions(c, QStringLiteral("version")));
        QCOMPARE(c.logging.maxFiles, std::size_t(5));
    }

    // `file` names a log inside the logs directory. A path would put it
    // somewhere the retention sweep never looks.
    void fileMustBeAPlainName()
    {
        for (const QString& bad : {QStringLiteral("logs/app.log"),
                                   QStringLiteral("../app.log"),
                                   QStringLiteral("sub\\app.log")}) {
            // Single-quoted: YAML processes escapes inside DOUBLE quotes, so
            // "sub\app.log" would arrive as a bell character and no backslash.
            const auto c = parse(QStringLiteral("logging:\n  file: '%1'\n").arg(bad));
            QVERIFY2(mentions(c, QStringLiteral("logging.file")), qPrintable(bad));
            QCOMPARE(c.logging.file, QStringLiteral("basecamp.log"));
        }
    }

    // ── An unknown key is reported, but costs nothing else ─────────────────

    void unknownKeyIsReportedWithoutDiscardingTheRest()
    {
        const auto c = parse(
            "logging:\n"
            "  max_size: 25\n"
            "  max_files: 2\n");
        QVERIFY(mentions(c, QStringLiteral("max_size")));
        QVERIFY2(c.logging.maxFiles == std::size_t(2),
                 "an unknown key names a mistake, but the keys that DID read "
                 "must still be applied");
    }

    void unknownTopLevelKeyIsReported()
    {
        const auto c = parse("loging:\n  enabled: false\n");
        QVERIFY(mentions(c, QStringLiteral("loging")));
        QVERIFY(c.logging.enabled);
    }
};

QTEST_MAIN(LoggingConfigTest)
#include "logging_config_test.moc"
