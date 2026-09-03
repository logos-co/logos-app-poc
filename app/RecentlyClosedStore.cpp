#include "RecentlyClosedStore.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>

namespace {
constexpr auto kArrayKey = "recentlyClosed";
constexpr auto kNameKey  = "name";
constexpr auto kClosedAt = "closedAt";

bool hasName(const QList<RecentlyClosedStore::Entry>& entries, const QString& name)
{
    for (const auto& e : entries) {
        if (e.name == name) return true;
    }
    return false;
}
}

RecentlyClosedStore::RecentlyClosedStore(QString filePath)
    : m_path(std::move(filePath))
{
    load();
}

QStringList RecentlyClosedStore::names() const
{
    QStringList out;
    out.reserve(m_entries.size());
    for (const auto& e : m_entries) {
        out.append(e.name);
    }
    return out;
}

void RecentlyClosedStore::load()
{
    QSettings settings(m_path, QSettings::IniFormat);
    const int count = settings.beginReadArray(kArrayKey);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        const QString name = settings.value(kNameKey).toString();
        // Skip blanks and duplicates rather than letting a hand-edited file
        // produce a nameless tile or the same app twice.
        if (name.isEmpty() || hasName(m_entries, name)) {
            continue;
        }
        m_entries.append({name,
                          QDateTime::fromString(settings.value(kClosedAt).toString(),
                                                Qt::ISODate)});
    }
    settings.endArray();

    while (m_entries.size() > kMaxEntries) {
        m_entries.removeLast();
    }
}

void RecentlyClosedStore::save() const
{
    // QSettings will not create missing parent directories, and fails silently
    // if they are absent — on a first run nothing else may have made the dir yet.
    QDir().mkpath(QFileInfo(m_path).absolutePath());

    QSettings settings(m_path, QSettings::IniFormat);
    // remove() first: beginWriteArray leaves stale higher indices behind when
    // the list shrinks, which would resurrect forgotten apps on the next load.
    settings.remove(kArrayKey);
    settings.beginWriteArray(kArrayKey, m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(kNameKey, m_entries.at(i).name);
        settings.setValue(kClosedAt, m_entries.at(i).closedAt.toString(Qt::ISODate));
    }
    settings.endArray();

    // QSettings reports write failures only through status() — otherwise a
    // read-only or full data dir looks identical to "nothing was ever closed",
    // which is expensive to diagnose from the UI end.
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        qWarning() << "RecentlyClosedStore: failed to write" << m_path
                   << "status:" << settings.status();
    }
}

void RecentlyClosedStore::record(const QString& name)
{
    if (name.isEmpty()) {
        return;
    }
    m_entries.removeIf([&name](const Entry& e) { return e.name == name; });
    m_entries.prepend({name, QDateTime::currentDateTimeUtc()});
    while (m_entries.size() > kMaxEntries) {
        m_entries.removeLast();
    }
    save();
}

void RecentlyClosedStore::forget(const QString& name)
{
    if (m_entries.removeIf([&name](const Entry& e) { return e.name == name; }) > 0) {
        save();
    }
}
