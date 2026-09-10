#include "CoreDependencyLoader.h"

CoreDependencyLoader::CoreDependencyLoader(QObject* parent)
    : QObject(parent)
    , m_worker(new QObject)
{
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start();
}

CoreDependencyLoader::~CoreDependencyLoader()
{
    // wait() is what lets the worker lambda capture `this`: the thread cannot
    // outlive us part-way through a load.
    m_thread.quit();
    m_thread.wait();
}

void CoreDependencyLoader::enqueue(const QStringList& required,
                                   const QStringList& optional,
                                   LoadOne loadOne,
                                   std::function<void()> onSuccess,
                                   std::function<void(const QString&)> onFailure)
{
    QMetaObject::invokeMethod(m_worker,
        [this, required, optional, loadOne, onSuccess, onFailure] {
            QString failed;
            for (const QString& name : required) {
                if (!loadOne(name, true)) {
                    failed = name;
                    break;
                }
            }
            // Only once the required set is up: an optional collaborator of a
            // plugin that is not going to mount is work nobody asked for.
            if (failed.isEmpty()) {
                for (const QString& name : optional)
                    loadOne(name, false);
            }
            QMetaObject::invokeMethod(this, [failed, onSuccess, onFailure] {
                if (failed.isEmpty()) onSuccess();
                else                  onFailure(failed);
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
}
