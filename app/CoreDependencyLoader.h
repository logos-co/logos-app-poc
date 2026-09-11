#pragma once

#include <QObject>
#include <QStringList>
#include <QThread>

#include <functional>

// Loads a UI plugin's core dependencies off the GUI thread.
//
// Each logos_core_load_module spawns a subprocess and waits for its verdict
// (~35 ms warm, ~150 ms cold), so a plugin with a handful of dependencies
// froze the GUI thread — including the spinner raised to cover the wait.
// Off-thread is safe by contract: logos_core.h documents core's outbound calls
// as marshalled to the thread that called logos_core_start().
class CoreDependencyLoader : public QObject {
    Q_OBJECT

public:
    // Called on the worker thread, once per dependency, in order. `required`
    // is false for the best-effort ones, so a caller can say which kind it is
    // reporting. Returns "ended up loaded" — already-loaded counts, matching
    // ICoreRuntime::loadModule.
    using LoadOne = std::function<bool(const QString& name, bool required)>;

    explicit CoreDependencyLoader(QObject* parent = nullptr);
    ~CoreDependencyLoader() override;

    // Loads `required` in order and stops at the first failure, then every
    // `optional` one best-effort — a failure there does not fail the batch,
    // because a plugin that does not REQUIRE a module must still mount when
    // that module is absent. Exactly one callback runs, on THIS object's
    // thread. Batches run one at a time, in submission order.
    //
    // The ordering that matters to callers: an off-thread load posts its
    // capability-module registration to the owner thread as it goes, and
    // onSuccess is posted after all of them — so a caller that spawns a
    // process from onSuccess still finds every dependency registered.
    void enqueue(const QStringList& required,
                 const QStringList& optional,
                 LoadOne loadOne,
                 std::function<void()> onSuccess,
                 std::function<void(const QString& failedDependency)> onFailure);

private:
    QThread  m_thread;
    QObject* m_worker;   // lives on m_thread; deleted when it finishes
};
