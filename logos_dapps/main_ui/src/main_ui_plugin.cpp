#include "main_ui_plugin.h"
#include "MainContainer.h"
#include <QDebug>
#include "logos_api.h"
#include "token_manager.h"

MainUIPlugin::MainUIPlugin(QObject* parent)
    : QObject(parent)
    , m_mainContainer(nullptr)
    , m_logosAPI(nullptr)
{
    qDebug() << "MainUIPlugin created";
}

MainUIPlugin::~MainUIPlugin()
{
    qDebug() << "MainUIPlugin destroyed";
    destroyWidget(m_mainContainer);
}

QWidget* MainUIPlugin::createWidget(LogosAPI* logosAPI)
{
    qDebug() << "-----> MainUIPlugin::createWidget: logosAPI:" << logosAPI;
    if (logosAPI) {
        m_logosAPI = logosAPI;
    }

    // print keys
    if (m_logosAPI) {
        QList<QString> keys = m_logosAPI->getTokenManager()->getTokenKeys();
        for (const QString& key : keys) {
            qDebug() << "-----> MainUIPlugin::createWidget: Token key:" << key << "value:" << m_logosAPI->getTokenManager()->getToken(key);
        }
    }
    
    if (!m_mainContainer) {
        m_mainContainer = new MainContainer(m_logosAPI);
    }
    return m_mainContainer;
}

void MainUIPlugin::destroyWidget(QWidget* widget)
{
    if (widget) {
        delete widget;
        if (widget == m_mainContainer) {
            m_mainContainer = nullptr;
        }
    }
} 