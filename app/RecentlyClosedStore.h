#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

// Most-recently-closed UI apps, persisted across restarts.
//
// Only the app's name and the time it was closed are stored. Display name and
// icon are deliberately NOT persisted: they are re-resolved from live plugin
// metadata at read time, so a renamed or upgraded app never renders from a
// stale cached label or a dead icon URL.
//
// Order encodes recency (most recent first); closedAt is carried so a future
// age-based cap or "closed 2 days ago" label needs no format migration.
//
// The file lives under LogosBasecampPaths::baseDirectory(), which already
// applies the portable/dev split and honours --user-dir — so dev and portable
// builds on one machine never share a list, and tests get an isolated one.
class RecentlyClosedStore {
public:
    // Entries beyond this are dropped on write. The welcome page shows fewer.
    static constexpr int kMaxEntries = 10;

    struct Entry {
        QString   name;
        QDateTime closedAt;
    };

    explicit RecentlyClosedStore(QString filePath);

    // Moves name to the front, de-duplicating an earlier close of the same app.
    void record(const QString& name);

    // Drops name entirely — used on uninstall, so a later reinstall does not
    // resurrect it as "recently closed".
    void forget(const QString& name);

    // Most recent first. Callers must still filter against what is installed.
    QStringList names() const;

    const QList<Entry>& entries() const { return m_entries; }

private:
    void load();
    void save() const;

    QString      m_path;
    QList<Entry> m_entries;
};
