#pragma once

#include <QString>
#include <QStringList>

#include <cstddef>

namespace LogosBasecampLog {

// The `logging:` block of the session's config document, resolved.
struct LoggingSettings {
    bool enabled = true;
    QString dir;                                     // absolute, always set
    QString file = QStringLiteral("basecamp.log");
    std::size_t maxSizeMb = 10;
    std::size_t maxFiles = 5;
    bool console = true;
};

struct ConfigLoad {
    LoggingSettings logging;
    // What was wrong with the document, phrased for whoever wrote it. Emitted
    // AFTER the sink starts, so the reasons land in the log file as well as on
    // the terminal.
    QStringList diagnostics;
};

// The document: <sessionDir>/config.yaml.
QString configPath(const QString& sessionDir);

// Read it. A missing file is not a problem — the defaults are a working
// configuration. A document that is present but unreadable leaves the defaults
// in place and records why: half-applying it is how someone ends up with a
// running app that does not match the file they are looking at.
ConfigLoad loadConfig(const QString& sessionDir);

// Same reader over a document already in memory, so the schema can be tested
// without touching disk.
ConfigLoad parseConfig(const QString& text, const QString& sessionDir);

} // namespace LogosBasecampLog
