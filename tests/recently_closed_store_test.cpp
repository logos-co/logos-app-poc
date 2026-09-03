// srcdeps: RecentlyClosedStore.cpp
//
// Unit tests for the welcome page's Recently Closed list (app/
// RecentlyClosedStore.{h,cpp}) — the persisted record of which apps the user
// closed, most recent first.
//
// Persistence is the whole point, so most of these construct a SECOND store
// over the same file rather than reading back from the first: that is what an
// app restart actually does, and an in-memory-only regression would pass any
// test that trusted the original instance.
//
// Two behaviours here are load-bearing and easy to regress:
//   * a shrinking list must not leave stale entries behind — QSettings'
//     beginWriteArray keeps higher indices unless the array is removed first,
//     which would resurrect a forgotten app on the next load;
//   * re-closing an app must move it, not duplicate it.
//
// Run: nix build .#unit-tests -L

#include "../app/RecentlyClosedStore.h"

#include <QtTest/QtTest>
#include <QTemporaryDir>

class RecentlyClosedStoreTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString path() const { return m_dir.filePath("recently-closed.ini"); }

    // What a fresh process would see: a new store over the same file.
    QStringList afterRestart() const { return RecentlyClosedStore(path()).names(); }

private slots:
    void init()
    {
        QVERIFY(m_dir.isValid());
        QFile::remove(path());
    }

    void emptyWhenNothingClosed()
    {
        QVERIFY(RecentlyClosedStore(path()).names().isEmpty());
    }

    void recordsAndPersists()
    {
        { RecentlyClosedStore s(path()); s.record("blockchain"); }
        QCOMPARE(afterRestart(), QStringList{"blockchain"});
    }

    void mostRecentFirst()
    {
        RecentlyClosedStore s(path());
        s.record("storage_ui");
        s.record("blockchain");
        QCOMPARE(s.names(), (QStringList{"blockchain", "storage_ui"}));
    }

    // Re-closing moves the entry rather than adding a second copy.
    void reclosingDeduplicates()
    {
        RecentlyClosedStore s(path());
        s.record("blockchain");
        s.record("storage_ui");
        s.record("blockchain");
        QCOMPARE(s.names(), (QStringList{"blockchain", "storage_ui"}));
    }

    void orderSurvivesRestart()
    {
        {
            RecentlyClosedStore s(path());
            s.record("storage_ui");
            s.record("blockchain");
        }
        QCOMPARE(afterRestart(), (QStringList{"blockchain", "storage_ui"}));
    }

    // Uninstall drops the entry so a later reinstall does not resurrect it.
    void forgetRemovesAndPersists()
    {
        {
            RecentlyClosedStore s(path());
            s.record("storage_ui");
            s.record("blockchain");
            s.forget("storage_ui");
            QCOMPARE(s.names(), QStringList{"blockchain"});
        }
        QCOMPARE(afterRestart(), QStringList{"blockchain"});
    }

    // The shrink case: without remove() before beginWriteArray, the dropped
    // entry survives at its old index and reappears on load.
    void forgettingEveryEntryLeavesNothingBehind()
    {
        {
            RecentlyClosedStore s(path());
            s.record("a");
            s.record("b");
            s.forget("a");
            s.forget("b");
        }
        QVERIFY(afterRestart().isEmpty());
    }

    void forgetIsQuietWhenAbsent()
    {
        RecentlyClosedStore s(path());
        s.record("blockchain");
        s.forget("never_closed");
        QCOMPARE(s.names(), QStringList{"blockchain"});
    }

    void emptyNameIsIgnored()
    {
        RecentlyClosedStore s(path());
        s.record(QString());
        QVERIFY(s.names().isEmpty());
    }

    void capsAtMaxEntriesEvictingOldest()
    {
        RecentlyClosedStore s(path());
        for (int i = 0; i < RecentlyClosedStore::kMaxEntries + 5; ++i)
            s.record(QStringLiteral("app%1").arg(i));

        const QStringList names = s.names();
        QCOMPARE(names.size(), RecentlyClosedStore::kMaxEntries);
        // Newest kept, oldest evicted.
        QCOMPARE(names.first(),
                 QStringLiteral("app%1").arg(RecentlyClosedStore::kMaxEntries + 4));
        QVERIFY(!names.contains("app0"));
    }

    void closedAtIsRecordedPerEntry()
    {
        RecentlyClosedStore s(path());
        s.record("blockchain");
        QCOMPARE(s.entries().size(), 1);
        QVERIFY(s.entries().first().closedAt.isValid());
    }

    // A hand-edited file must not produce a nameless tile or the same app twice.
    void ignoresBlankAndDuplicateRowsOnDisk()
    {
        {
            QSettings out(path(), QSettings::IniFormat);
            out.beginWriteArray("recentlyClosed", 3);
            out.setArrayIndex(0); out.setValue("name", "blockchain");
            out.setArrayIndex(1); out.setValue("name", "");
            out.setArrayIndex(2); out.setValue("name", "blockchain");
            out.endArray();
        }
        QCOMPARE(afterRestart(), QStringList{"blockchain"});
    }

    // The store is handed a path under a directory that may not exist yet on a
    // first run; QSettings will not create it and fails silently if absent.
    void createsItsParentDirectory()
    {
        const QString nested = m_dir.filePath("does/not/exist/recently-closed.ini");
        { RecentlyClosedStore s(nested); s.record("blockchain"); }
        QVERIFY(QFile::exists(nested));
        QCOMPARE(RecentlyClosedStore(nested).names(), QStringList{"blockchain"});
    }
};

QTEST_MAIN(RecentlyClosedStoreTest)
#include "recently_closed_store_test.moc"
