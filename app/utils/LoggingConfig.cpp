#include "LoggingConfig.h"

#include "LogosBasecampPaths.h"

#include <QDir>
#include <QFile>

#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <iterator>

namespace LogosBasecampLog {
namespace {

// Bumped only for a change a v1 document could not survive. Absent means 1, so
// a hand-written file need not carry it.
constexpr int kSchemaVersion = 1;

const char* const kTopLevelKeys[] = { "version", "logging" };
const char* const kLoggingKeys[] = {
    "enabled", "file", "dir", "max_size_mb", "max_files", "console"
};

QString join(const char* const* keys, std::size_t n)
{
    QStringList out;
    for (std::size_t i = 0; i < n; ++i) out << QString::fromLatin1(keys[i]);
    return out.join(QStringLiteral(", "));
}

// True when a scalar would read back as something other than a string. YAML
// scalars are untyped text and the tag records whether the author quoted them,
// so `file: 42` is a number the author has to fix rather than the string "42".
bool readsAsString(const YAML::Node& n)
{
    if (n.Tag() == "!") return true;   // explicitly quoted
    bool b = false;
    if (YAML::convert<bool>::decode(n, b)) return false;
    std::int64_t i = 0;
    if (YAML::convert<std::int64_t>::decode(n, i)) return false;
    double d = 0;
    if (YAML::convert<double>::decode(n, d)) return false;
    return true;
}

// What a value actually is, phrased for the "but got ..." half of a message.
QString describe(const YAML::Node& n)
{
    switch (n.Type()) {
    case YAML::NodeType::Null:
    case YAML::NodeType::Undefined: return QStringLiteral("nothing");
    case YAML::NodeType::Sequence:  return QStringLiteral("a list");
    case YAML::NodeType::Map:       return QStringLiteral("a mapping");
    case YAML::NodeType::Scalar:    break;
    }
    if (readsAsString(n)) return QStringLiteral("a string");
    bool b = false;
    if (YAML::convert<bool>::decode(n, b)) return QStringLiteral("true or false");
    std::int64_t i = 0;
    if (YAML::convert<std::int64_t>::decode(n, i)) return QStringLiteral("a whole number");
    return QStringLiteral("a decimal number");
}

// Type-checked reads over one mapping. A mismatch is recorded as a message
// naming the key by its dotted path, the read yields the caller's default, and
// `failed()` tells the caller the document as a whole is not to be applied.
class Reader {
public:
    Reader(const YAML::Node& node, QStringList& diagnostics, QString prefix)
        : m_node(node), m_diagnostics(diagnostics), m_prefix(std::move(prefix)) {}

    bool failed() const { return m_failed; }

    QString str(const char* key, const QString& dflt) const
    {
        const YAML::Node v = at(key);
        if (notSet(v)) return dflt;
        if (!v.IsScalar() || !readsAsString(v)) {
            mismatch(key, QStringLiteral("a string"), v);
            return dflt;
        }
        return QString::fromStdString(v.Scalar());
    }

    bool boolean(const char* key, bool dflt) const
    {
        const YAML::Node v = at(key);
        if (notSet(v)) return dflt;
        bool b = false;
        if (!v.IsScalar() || v.Tag() == "!" || !YAML::convert<bool>::decode(v, b)) {
            mismatch(key, QStringLiteral("true or false"), v);
            return dflt;
        }
        return b;
    }

    // One accessor for every integral field: YAML has a single integer type, so
    // callers differ only in the range they accept. Out of range is reported the
    // same way as a wrong type — it is the same class of mistake.
    std::size_t integer(const char* key, std::size_t dflt,
                        std::int64_t min, std::int64_t max) const
    {
        const YAML::Node v = at(key);
        if (notSet(v)) return dflt;
        std::int64_t n = 0;
        if (!v.IsScalar() || v.Tag() == "!"
            || !YAML::convert<std::int64_t>::decode(v, n)) {
            mismatch(key, QStringLiteral("a whole number"), v);
            return dflt;
        }
        if (n < min || n > max) {
            note(QStringLiteral("%1: expected a whole number between %2 and %3, "
                                "but got %4.")
                     .arg(path(key)).arg(min).arg(max).arg(n));
            return dflt;
        }
        return static_cast<std::size_t>(n);
    }

    // Advisory, not fatal: the value was understood, the key was not. Silence
    // here is how `max_size` (the key is `max_size_mb`) leaves someone reading a
    // file whose intent the app never applied.
    void reportUnknownKeys(const char* const* known, std::size_t n) const
    {
        if (!m_node.IsMap()) return;
        for (const auto& kv : m_node) {
            if (!kv.first.IsScalar()) continue;
            const QString key = QString::fromStdString(kv.first.Scalar());
            bool found = false;
            for (std::size_t i = 0; i < n && !found; ++i)
                found = (key == QString::fromLatin1(known[i]));
            if (!found)
                m_diagnostics << QStringLiteral("%1: unknown key, ignored. "
                                                "Known keys are: %2.")
                                     .arg(m_prefix + key, join(known, n));
        }
    }

    QString path(const char* key) const { return m_prefix + QString::fromLatin1(key); }

    void note(const QString& message) const
    {
        m_failed = true;
        m_diagnostics << message;
    }

private:
    // "Not set", and so the caller's default wins, covers two shapes: a key
    // that is absent (yaml-cpp hands back a zombie, IsDefined() false) and one
    // that is explicitly empty -- `key:` with nothing after it, which YAML
    // reads as null. Note that a DEFAULT-CONSTRUCTED Node is neither: it
    // reports IsDefined() as true, so it cannot stand in for a missing key.
    static bool notSet(const YAML::Node& v) { return !v.IsDefined() || v.IsNull(); }

    YAML::Node at(const char* key) const
    {
        return m_node.IsMap() ? YAML::Node(m_node[key]) : YAML::Node();
    }

    void mismatch(const char* key, const QString& expected,
                  const YAML::Node& got) const
    {
        note(QStringLiteral("%1: expected %2, but got %3.")
                 .arg(path(key), expected, describe(got)));
    }

    const YAML::Node& m_node;
    QStringList&      m_diagnostics;
    QString           m_prefix;
    mutable bool      m_failed = false;
};

// How a redirect is written decides whether the session stays portable:
//   logs-custom  -> <session>/logs-custom   (still portable)
//   ~/x          -> $HOME/x                 (outside the session)
//   /var/log/x   -> as given                (outside the session)
QString resolveDir(const QString& raw, const QString& sessionDir)
{
    if (raw.isEmpty())
        return LogosBasecampPaths::logsDirectoryIn(sessionDir);
    if (raw == QLatin1String("~") || raw.startsWith(QLatin1String("~/")))
        return QDir::homePath() + raw.mid(1);
    if (QDir::isAbsolutePath(raw))
        return raw;
    return QDir(sessionDir).filePath(raw);
}

ConfigLoad defaults(const QString& sessionDir)
{
    ConfigLoad out;
    out.logging.dir = resolveDir(QString(), sessionDir);
    return out;
}

} // namespace

QString configPath(const QString& sessionDir)
{
    return QDir(sessionDir).filePath(QStringLiteral("config.yaml"));
}

ConfigLoad parseConfig(const QString& text, const QString& sessionDir)
{
    ConfigLoad out = defaults(sessionDir);

    YAML::Node root;
    try {
        root = YAML::Load(text.toStdString());
    } catch (const std::exception& e) {
        out.diagnostics << QStringLiteral("%1 is not valid YAML: %2. Using the "
                                          "built-in logging defaults.")
                               .arg(configPath(sessionDir),
                                    QString::fromUtf8(e.what()));
        return out;
    }

    // An empty document is a valid file that says nothing.
    if (!root.IsDefined() || root.IsNull())
        return out;
    if (!root.IsMap()) {
        out.diagnostics << QStringLiteral("%1 must be a mapping at the top "
                                          "level, but it is %2. Using the "
                                          "built-in logging defaults.")
                               .arg(configPath(sessionDir), describe(root));
        return out;
    }

    QStringList problems;
    Reader top(root, problems, QString());
    top.reportUnknownKeys(kTopLevelKeys, std::size(kTopLevelKeys));

    const auto version = top.integer("version", kSchemaVersion, 0, 1000000);
    if (!top.failed() && static_cast<int>(version) != kSchemaVersion)
        top.note(QStringLiteral("version: this build reads version %1 documents, "
                                "but the file says %2.")
                     .arg(kSchemaVersion).arg(version));

    LoggingSettings parsed;
    parsed.dir = out.logging.dir;   // no `logging:` block still needs a directory
    bool loggingFailed = false;
    const YAML::Node logging = root["logging"];
    if (logging.IsDefined() && !logging.IsNull()) {
        if (!logging.IsMap()) {
            problems << QStringLiteral("logging: expected a mapping, but got %1.")
                            .arg(describe(logging));
            loggingFailed = true;
        } else {
            Reader r(logging, problems, QStringLiteral("logging."));
            r.reportUnknownKeys(kLoggingKeys, std::size(kLoggingKeys));
            parsed.enabled   = r.boolean("enabled", true);
            parsed.file      = r.str("file", parsed.file);
            parsed.maxSizeMb = r.integer("max_size_mb", 10, 0, 1024 * 1024);
            parsed.maxFiles  = r.integer("max_files", 5, 0, 1000000);
            parsed.console   = r.boolean("console", true);
            parsed.dir       = resolveDir(r.str("dir", QString()), sessionDir);
            loggingFailed = r.failed();
        }
    }

    out.diagnostics += problems;
    if (top.failed() || loggingFailed) {
        out.diagnostics << QStringLiteral("Using the built-in logging defaults; "
                                          "%1 was not applied.")
                               .arg(configPath(sessionDir));
        return out;
    }

    // `file` names a log inside the logs directory. A separator would put it
    // somewhere else entirely and break the retention sweep, which only ever
    // looks in one directory.
    if (parsed.file.isEmpty() || parsed.file.contains(QLatin1Char('/'))
        || parsed.file.contains(QLatin1Char('\\'))
        || parsed.file.contains(QLatin1String(".."))) {
        out.diagnostics << QStringLiteral("logging.file: expected a plain file "
                                          "name, but got \"%1\". Using the "
                                          "built-in logging defaults.")
                               .arg(parsed.file);
        return out;
    }

    out.logging = parsed;
    return out;
}

ConfigLoad loadConfig(const QString& sessionDir)
{
    const QString path = configPath(sessionDir);
    QFile f(path);
    if (!f.exists())
        return defaults(sessionDir);   // no document is the common case
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ConfigLoad out = defaults(sessionDir);
        out.diagnostics << QStringLiteral("%1 could not be opened: %2. Using the "
                                          "built-in logging defaults.")
                               .arg(path, f.errorString());
        return out;
    }
    return parseConfig(QString::fromUtf8(f.readAll()), sessionDir);
}

} // namespace LogosBasecampLog
