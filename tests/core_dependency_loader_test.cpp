// srcdeps: CoreDependencyLoader.cpp
//
// The contract PluginLoader relies on: dependencies load OFF the calling
// thread, in order, stopping at the first failure, and report back ON the
// calling thread.

#include <QtTest/QtTest>

#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QThread>
#include <QTimer>

#include "CoreDependencyLoader.h"

namespace {

// The loader hands names to its worker; a test needs to read them back from
// the main thread once it is done, so every observation is mutex-guarded.
struct Recorder {
    mutable QMutex   mutex;
    QStringList      attempted;
    QList<bool>      requiredFlags;
    QSet<QThread*>   threads;
    QStringList      failing;      // names loadOne should refuse
    int              delayMs = 0;

    bool loadOne(const QString& name, bool required)
    {
        {
            QMutexLocker lock(&mutex);
            attempted << name;
            requiredFlags << required;
            threads.insert(QThread::currentThread());
        }
        if (delayMs > 0) QThread::msleep(delayMs);
        QMutexLocker lock(&mutex);
        return !failing.contains(name);
    }

    QStringList attemptedNames() const { QMutexLocker l(&mutex); return attempted; }
    QList<bool> flags() const { QMutexLocker l(&mutex); return requiredFlags; }
    QSet<QThread*> usedThreads() const { QMutexLocker l(&mutex); return threads; }
};

// Bind a Recorder as a batch's load callable.
CoreDependencyLoader::LoadOne bind(Recorder& rec)
{
    return [&rec](const QString& n, bool required) { return rec.loadOne(n, required); };
}

}  // namespace

class CoreDependencyLoaderTest : public QObject {
    Q_OBJECT

private slots:
    void loadsEveryDependencyInOrder();
    void runsOffTheCallingThread();
    void reportsBackOnTheCallingThread();
    void doesNotBlockTheCallingThread();
    void stopsAtTheFirstFailure();
    void runsBatchesInSubmissionOrder();
    void ownerThreadWorkPostedByALoadRunsBeforeTheCallback();
    void optionalDependenciesLoadAfterTheRequiredOnes();
    void anOptionalFailureDoesNotFailTheBatch();
    void optionalsAreSkippedWhenARequiredOneFails();
    void joinsItsWorkerOnDestruction();
};

// Spin the event loop until `done`, or fail after `timeoutMs`.
#define AWAIT(done, timeoutMs) do { \
        QElapsedTimer _t; _t.start(); \
        while (!(done) && _t.elapsed() < (timeoutMs)) \
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10); \
        QVERIFY2((done), "timed out waiting for the loader"); \
    } while (0)

void CoreDependencyLoaderTest::loadsEveryDependencyInOrder()
{
    Recorder rec;
    CoreDependencyLoader loader;

    bool ok = false;
    loader.enqueue({"alpha", "beta", "gamma"}, {}, bind(rec), [&ok] { ok = true; },
                   [](const QString&) { QFAIL("unexpected failure"); });

    AWAIT(ok, 5000);
    QCOMPARE(rec.attemptedNames(), QStringList({"alpha", "beta", "gamma"}));
}

void CoreDependencyLoaderTest::runsOffTheCallingThread()
{
    Recorder rec;
    CoreDependencyLoader loader;

    bool ok = false;
    loader.enqueue({"alpha", "beta"}, {}, bind(rec), [&ok] { ok = true; }, [](const QString&) {});
    AWAIT(ok, 5000);

    const QSet<QThread*> used = rec.usedThreads();
    QCOMPARE(used.size(), 1);
    QVERIFY2(!used.contains(QThread::currentThread()),
             "dependencies must not load on the caller's thread");
}

void CoreDependencyLoaderTest::reportsBackOnTheCallingThread()
{
    Recorder rec;
    CoreDependencyLoader loader;

    QThread* successThread = nullptr;
    QThread* failureThread = nullptr;

    bool ok = false;
    loader.enqueue({"alpha"}, {}, bind(rec), [&] { successThread = QThread::currentThread(); ok = true; },
                   [](const QString&) {});
    AWAIT(ok, 5000);

    rec.failing = {"beta"};
    bool failed = false;
    loader.enqueue({"beta"}, {}, bind(rec), [] { QFAIL("unexpected success"); },
                   [&](const QString&) { failureThread = QThread::currentThread(); failed = true; });
    AWAIT(failed, 5000);

    QCOMPARE(successThread, QThread::currentThread());
    QCOMPARE(failureThread, QThread::currentThread());
}

void CoreDependencyLoaderTest::doesNotBlockTheCallingThread()
{
    Recorder rec;
    rec.delayMs = 120;                      // stand-in for a subprocess spawn
    CoreDependencyLoader loader;

    bool ok = false;
    QElapsedTimer enqueueTimer;
    enqueueTimer.start();
    loader.enqueue({"alpha", "beta", "gamma"}, {}, bind(rec), [&ok] { ok = true; }, [](const QString&) {});
    const qint64 enqueueMs = enqueueTimer.elapsed();

    // The caller returns immediately...
    QVERIFY2(enqueueMs < 50, qPrintable(QStringLiteral("enqueue() blocked for %1 ms")
                                            .arg(enqueueMs)));

    // ...and its event loop keeps running while the loads are in flight, which
    // is what lets the spinner animate instead of freezing.
    int ticks = 0;
    QTimer ticker;
    connect(&ticker, &QTimer::timeout, [&ticks] { ++ticks; });
    ticker.start(10);

    AWAIT(ok, 5000);
    QVERIFY2(ticks > 0, "the caller's event loop stalled while dependencies loaded");
}

void CoreDependencyLoaderTest::stopsAtTheFirstFailure()
{
    Recorder rec;
    rec.failing = {"beta"};
    CoreDependencyLoader loader;

    QString reported;
    bool failed = false;
    loader.enqueue({"alpha", "beta", "gamma"}, {}, bind(rec), [] { QFAIL("unexpected success"); },
                   [&](const QString& name) { reported = name; failed = true; });

    AWAIT(failed, 5000);
    QCOMPARE(reported, QStringLiteral("beta"));
    QCOMPARE(rec.attemptedNames(), QStringList({"alpha", "beta"}));
}

void CoreDependencyLoaderTest::runsBatchesInSubmissionOrder()
{
    Recorder rec;
    CoreDependencyLoader loader;

    int done = 0;
    loader.enqueue({"first"}, {}, bind(rec), [&done] { ++done; }, [](const QString&) {});
    loader.enqueue({"second"}, {}, bind(rec), [&done] { ++done; }, [](const QString&) {});

    AWAIT(done == 2, 5000);
    QCOMPARE(rec.attemptedNames(), QStringList({"first", "second"}));
}

void CoreDependencyLoaderTest::ownerThreadWorkPostedByALoadRunsBeforeTheCallback()
{
    // The invariant PluginLoader depends on. logos_core_load_module POSTS its
    // capability-module registration to the owner thread (runOnOwner) rather
    // than running it inline when called off that thread. onSuccess is posted
    // after all of them, so it must be delivered last — otherwise a ui-host
    // spawned from onSuccess could call a dependency that is not registered
    // yet and be refused.
    QStringList order;
    CoreDependencyLoader loader;
    auto poster = [this, &order](const QString& name, bool) {
        QMetaObject::invokeMethod(this, [&order, name] {
            order << QStringLiteral("registered:") + name;
        }, Qt::QueuedConnection);
        return true;
    };

    bool ok = false;
    loader.enqueue({"alpha", "beta"}, {}, poster, [&] { order << QStringLiteral("callback"); ok = true; },
                   [](const QString&) { QFAIL("unexpected failure"); });

    AWAIT(ok, 5000);
    QCOMPARE(order, QStringList({QStringLiteral("registered:alpha"),
                                 QStringLiteral("registered:beta"),
                                 QStringLiteral("callback")}));
}

void CoreDependencyLoaderTest::optionalDependenciesLoadAfterTheRequiredOnes()
{
    Recorder rec;
    CoreDependencyLoader loader;

    bool ok = false;
    loader.enqueue({"req1", "req2"}, {"opt1", "opt2"}, bind(rec),
                   [&ok] { ok = true; }, [](const QString&) { QFAIL("unexpected failure"); });

    AWAIT(ok, 5000);
    QCOMPARE(rec.attemptedNames(), QStringList({"req1", "req2", "opt1", "opt2"}));
    QCOMPARE(rec.flags(), QList<bool>({true, true, false, false}));
}

void CoreDependencyLoaderTest::anOptionalFailureDoesNotFailTheBatch()
{
    // A plugin that does not REQUIRE a module must still mount when it is
    // absent -- and the ones after it must still be tried.
    Recorder rec;
    rec.failing = {"opt1"};
    CoreDependencyLoader loader;

    bool ok = false;
    loader.enqueue({"req1"}, {"opt1", "opt2"}, bind(rec),
                   [&ok] { ok = true; },
                   [](const QString&) { QFAIL("an optional failure must not fail the batch"); });

    AWAIT(ok, 5000);
    QCOMPARE(rec.attemptedNames(), QStringList({"req1", "opt1", "opt2"}));
}

void CoreDependencyLoaderTest::optionalsAreSkippedWhenARequiredOneFails()
{
    // The plugin is not going to mount, so its optional collaborators are work
    // nobody asked for.
    Recorder rec;
    rec.failing = {"req1"};
    CoreDependencyLoader loader;

    QString reported;
    bool failed = false;
    loader.enqueue({"req1", "req2"}, {"opt1"}, bind(rec),
                   [] { QFAIL("unexpected success"); },
                   [&](const QString& n) { reported = n; failed = true; });

    AWAIT(failed, 5000);
    QCOMPARE(reported, QStringLiteral("req1"));
    QCOMPARE(rec.attemptedNames(), QStringList({"req1"}));
}

void CoreDependencyLoaderTest::joinsItsWorkerOnDestruction()
{
    // The worker lambda reads state owned outside it, so the destructor must
    // join rather than detach.
    Recorder rec;
    rec.delayMs = 50;
    {
        CoreDependencyLoader loader;
        loader.enqueue({"alpha", "beta"}, {}, bind(rec), [] {}, [](const QString&) {});
        QCoreApplication::processEvents();   // let the batch reach the worker
    }

    // Past the destructor the attempt list is final: nothing is still running.
    const QStringList settled = rec.attemptedNames();
    QThread::msleep(300);                    // well past both 50 ms loads
    QCOMPARE(rec.attemptedNames(), settled);
}

QTEST_MAIN(CoreDependencyLoaderTest)
#include "core_dependency_loader_test.moc"
