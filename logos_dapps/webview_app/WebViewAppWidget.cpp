#include "WebViewAppWidget.h"
#include "logos_api.h"
#include "logos_api_client.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QtWebView/QtWebView>
#include <QFile>
#include <QIODevice>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDateTime>

WebViewAppWidget::WebViewAppWidget(LogosAPI* logosAPI, QWidget* parent) 
    : QWidget(parent)
    , m_quickView(nullptr)
    , m_rootItem(nullptr)
    , m_wikipediaButton(nullptr)
    , m_localFileButton(nullptr)
    , m_sendToWebAppButton(nullptr)
    , m_statusLabel(nullptr)
    , m_qmlReady(false)
    , m_logosAPI(logosAPI)
{
    QtWebView::initialize();
    setupUI();
}

WebViewAppWidget::~WebViewAppWidget() {
}

void WebViewAppWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    QWidget* buttonBar = new QWidget(this);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonBar);
    buttonLayout->setContentsMargins(5, 5, 5, 5);
    
    m_wikipediaButton = new QPushButton("Wikipedia", buttonBar);
    m_localFileButton = new QPushButton("Local File", buttonBar);
    m_sendToWebAppButton = new QPushButton("Send Event to WebApp", buttonBar);
    
    connect(m_wikipediaButton, &QPushButton::clicked, this, &WebViewAppWidget::onWikipediaClicked);
    connect(m_localFileButton, &QPushButton::clicked, this, &WebViewAppWidget::onLocalFileClicked);
    connect(m_sendToWebAppButton, &QPushButton::clicked, this, &WebViewAppWidget::onSendToWebAppClicked);
    
    buttonLayout->addWidget(m_wikipediaButton);
    buttonLayout->addWidget(m_localFileButton);
    buttonLayout->addWidget(m_sendToWebAppButton);
    buttonLayout->addStretch();
    
    mainLayout->addWidget(buttonBar);
    
    m_statusLabel = new QLabel("Status: Ready", this);
    m_statusLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc; }");
    mainLayout->addWidget(m_statusLabel);
    
    m_quickView = new QQuickView();
    m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    m_quickView->setColor(Qt::white);
    
    m_quickView->rootContext()->setContextProperty("hostWidget", this);
    QString logosScriptContent;
    QFile logosScriptFile(":/logos-script.js");
    if (logosScriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logosScriptContent = QString::fromUtf8(logosScriptFile.readAll());
        logosScriptFile.close();
    } else {
        qWarning() << "Could not load qrc:/logos-script.js";
    }
    m_quickView->rootContext()->setContextProperty("logosScriptContent", logosScriptContent);
    
    connect(m_quickView, &QQuickView::statusChanged, this, [this](QQuickView::Status status) {
        qDebug() << "QQuickView status:" << status;
        if (status == QQuickView::Error) {
            for (const QQmlError& error : m_quickView->errors()) {
                qWarning() << "QML Error:" << error.toString();
            }
            m_statusLabel->setText("Status: QML Error");
            m_statusLabel->setStyleSheet("QLabel { background-color: #f8d7da; padding: 5px; border: 1px solid #dc3545; }");
        } else if (status == QQuickView::Ready) {
            m_rootItem = m_quickView->rootObject();
            qDebug() << "QML Ready, root item:" << m_rootItem;
        }
    });
    
    qDebug() << "Loading QML from: qrc:/WebView.qml";
    m_quickView->setSource(QUrl("qrc:/WebView.qml"));
    
    QWidget* container = QWidget::createWindowContainer(m_quickView, this);
    container->setMinimumSize(200, 200);
    container->setFocusPolicy(Qt::TabFocus);
    
    mainLayout->addWidget(container, 1);
    
    setMinimumSize(800, 600);
    
    m_pendingUrl = QUrl("https://en.wikipedia.org/wiki/Main_Page");
}

void WebViewAppWidget::qmlReady() {
    qDebug() << "=== QML component completed, WebView ready ===";
    m_qmlReady = true;
    m_rootItem = m_quickView ? m_quickView->rootObject() : nullptr;
    qDebug() << "Root item:" << m_rootItem;
    
    if (!m_pendingUrl.isEmpty()) {
        qDebug() << "Loading pending URL:" << m_pendingUrl;
        QUrl url = m_pendingUrl;
        m_pendingUrl.clear();
        loadURL(url);
    }
}

void WebViewAppWidget::onWikipediaClicked() {
    qDebug() << "Wikipedia button clicked";
    loadURL(QUrl("https://en.wikipedia.org/wiki/Main_Page"));
}

void WebViewAppWidget::onLocalFileClicked() {
    qDebug() << "Local File button clicked - loading from qrc:/local.html";
    loadLocalHtml();
}

void WebViewAppWidget::onSendToWebAppClicked() {
    qDebug() << "Send Event button clicked";
    QVariantMap data;
    data["message"] = "Hello from Qt!";
    data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    sendEventToJS("qtButtonClicked", data);
}

void WebViewAppWidget::loadLocalHtml() {
    if (!m_qmlReady || !m_rootItem) {
        qDebug() << "Cannot load local HTML - not ready";
        return;
    }
    
    QFile file(":/local.html");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open qrc:/local.html";
        m_statusLabel->setText("Status: Error loading local.html");
        m_statusLabel->setStyleSheet("QLabel { background-color: #f8d7da; padding: 5px; border: 1px solid #dc3545; }");
        return;
    }
    
    QString html = QString::fromUtf8(file.readAll());
    file.close();
    
    qDebug() << "Loaded HTML content, length:" << html.length();
    
    QMetaObject::invokeMethod(m_rootItem, "loadHtmlContent",
        Q_ARG(QVariant, html));
}

void WebViewAppWidget::loadURL(const QUrl& url) {
    qDebug() << "loadURL called:" << url;
    qDebug() << "  qmlReady:" << m_qmlReady;
    qDebug() << "  rootItem:" << m_rootItem;
    
    if (!m_qmlReady || !m_rootItem) {
        m_pendingUrl = url;
        qDebug() << "  -> Not ready, storing pending URL";
        return;
    }
    
    qDebug() << "  -> Calling QML loadUrl function";
    QMetaObject::invokeMethod(m_rootItem, "loadUrl", Q_ARG(QVariant, url));
}

void WebViewAppWidget::runJavaScript(const QString& script) {
    if (!m_qmlReady || !m_rootItem) {
        qDebug() << "Cannot run JavaScript - not ready";
        return;
    }
    qDebug() << "Running JavaScript in WebView";
    QMetaObject::invokeMethod(m_rootItem, "runScript", Q_ARG(QVariant, script));
}

void WebViewAppWidget::handleLogosRequest(const QString& moduleName, const QString& methodName, const QVariantList& args, int requestId) {
    qDebug() << "Received logos request:" << moduleName << methodName << args;

    if (moduleName == "host" && methodName == "changeQtLabel") {
        const QString text = args.value(0).toString();
        if (m_statusLabel) {
            m_statusLabel->setText("Status: " + text);
            m_statusLabel->setStyleSheet("QLabel { background-color: #d4edda; padding: 5px; border: 1px solid #28a745; }");
        }
        sendResponseToJS(requestId, QVariantMap{{"success", true}, {"message", "Label updated successfully"}});
        return;
    }

    if (!m_logosAPI) {
        sendResponseToJS(requestId, QVariant(), "LogosAPI not available");
        return;
    }

    LogosAPIClient* client = m_logosAPI->getClient(moduleName);
    if (!client) {
        sendResponseToJS(requestId, QVariant(), QString("Unknown module: %1").arg(moduleName));
        return;
    }

    QVariant response = client->invokeRemoteMethod(moduleName, methodName, args);
    if (!response.isValid()) {
        sendResponseToJS(requestId, QVariant(), QString("Empty response from %1.%2").arg(moduleName, methodName));
        return;
    }

    sendResponseToJS(requestId, response, QString());
}

void WebViewAppWidget::sendResponseToJS(int requestId, const QVariant& result, const QString& error) {
    QJsonObject responseObj;
    responseObj["type"] = "logos_response";
    responseObj["requestId"] = requestId;

    if (!error.isEmpty()) {
        responseObj["error"] = error;
    } else {
        responseObj["result"] = QJsonValue::fromVariant(result);
    }

    QJsonDocument doc(responseObj);
    QString script = QString("window.postMessage(%1, '*');")
        .arg(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    runJavaScript(script);
}

void WebViewAppWidget::sendEventToJS(const QString& eventName, const QVariantMap& data) {
    QJsonObject eventObj;
    eventObj["type"] = "logos_event";
    eventObj["eventName"] = eventName;
    
    QJsonObject dataObj;
    for (auto it = data.begin(); it != data.end(); ++it) {
        dataObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    eventObj["data"] = dataObj;
    
    QJsonDocument doc(eventObj);
    QString script = QString("window.postMessage(%1, '*');")
        .arg(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    runJavaScript(script);
}
