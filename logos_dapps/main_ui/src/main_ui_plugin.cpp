#include "main_ui_plugin.h"
#include "mainwindow.h"
#include "coremoduleview.h"
#include <QDebug>
#include "../../../../logos-cpp-sdk/cpp/logos_api.h"
#include "../../../../logos-cpp-sdk/cpp/token_manager.h"

MainUIPlugin::MainUIPlugin(QObject* parent)
    : QObject(parent)
    , m_mainWindow(nullptr)
    , m_coreModuleView(nullptr)
    , m_logosAPI(nullptr)
{
    qDebug() << "MainUIPlugin created";
}

MainUIPlugin::~MainUIPlugin()
{
    qDebug() << "MainUIPlugin destroyed";
    destroyWidget(m_mainWindow);
    if (m_coreModuleView) {
        delete m_coreModuleView;
        m_coreModuleView = nullptr;
    }
}

QWidget* MainUIPlugin::createWidget(LogosAPI* logosAPI)
{
    qDebug() << "-----> MainUIPlugin::createWidget: logosAPI:" << logosAPI;
    if (logosAPI) {
        m_logosAPI = logosAPI;
    }

    // print keys
    QList<QString> keys = m_logosAPI->getTokenManager()->getTokenKeys();
    for (const QString& key : keys) {
        qDebug() << "-----> MainUIPlugin::createWidget: Token key:" << key << "value:" << m_logosAPI->getTokenManager()->getToken(key);
    }
    
    if (!m_mainWindow) {
        m_mainWindow = new MainWindow(m_logosAPI);
    }
    return m_mainWindow;
}

void MainUIPlugin::destroyWidget(QWidget* widget)
{
    if (widget) {
        delete widget;
        if (widget == m_mainWindow) {
            m_mainWindow = nullptr;
        }
    }
}

CoreModuleView* MainUIPlugin::createCoreModuleView(QWidget* parent)
{
    if (!m_coreModuleView) {
        m_coreModuleView = new CoreModuleView(parent);
    }
    return m_coreModuleView;
} 