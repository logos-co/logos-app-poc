#include "WebViewAppWidget.h"
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
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

WebViewAppWidget::WebViewAppWidget(QWidget* parent) 
    : QWidget(parent)
    , m_quickView(nullptr)
    , m_rootItem(nullptr)
    , m_wikipediaButton(nullptr)
    , m_localFileButton(nullptr)
    , m_sendToWebAppButton(nullptr)
    , m_statusLabel(nullptr)
    , m_qmlReady(false)
{
    // Initialize QtWebView before creating QML content
    QtWebView::initialize();
    setupUI();
}

WebViewAppWidget::~WebViewAppWidget() {
}

void WebViewAppWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Button bar
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
    
    // Status label
    m_statusLabel = new QLabel("Status: Ready", this);
    m_statusLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc; }");
    mainLayout->addWidget(m_statusLabel);
    
    // Use QQuickView + createWindowContainer for better native view handling
    m_quickView = new QQuickView();
    m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    m_quickView->setColor(Qt::white);
    
    // Expose this widget to QML
    m_quickView->rootContext()->setContextProperty("hostWidget", this);
    
    // Connect to status changed
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
    
    // Load QML from embedded resources
    qDebug() << "Loading QML from: qrc:/WebView.qml";
    m_quickView->setSource(QUrl("qrc:/WebView.qml"));
    
    // Create a container widget from the QQuickView window
    QWidget* container = QWidget::createWindowContainer(m_quickView, this);
    container->setMinimumSize(200, 200);
    container->setFocusPolicy(Qt::TabFocus);
    
    mainLayout->addWidget(container, 1);
    
    setMinimumSize(800, 600);
    
    // Store pending URL - will be loaded when QML is ready
    m_pendingUrl = QUrl("https://en.wikipedia.org/wiki/Main_Page");
}

void WebViewAppWidget::qmlReady() {
    qDebug() << "=== QML component completed, WebView ready ===";
    m_qmlReady = true;
    m_rootItem = m_quickView ? m_quickView->rootObject() : nullptr;
    qDebug() << "Root item:" << m_rootItem;
    
    // Load pending URL if any
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
    
    // Read HTML content from embedded resources
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
    
    // Call QML function to load HTML content with empty baseUrl
    // (the HTML is self-contained, no relative URLs need resolving)
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
    
    // Call QML function directly
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

void WebViewAppWidget::handleLogosRequest(const QString& method, const QVariantMap& params, int requestId) {
    qDebug() << "Received logos request:" << method << params;
    
    QVariantMap result;
    
    if (method == "changeQtLabel") {
        QString text = params.value("text").toString();
        if (m_statusLabel) {
            m_statusLabel->setText("Status: " + text);
            m_statusLabel->setStyleSheet("QLabel { background-color: #d4edda; padding: 5px; border: 1px solid #28a745; }");
        }
        result["success"] = true;
        result["message"] = "Label updated successfully";
    } else {
        result["success"] = false;
        result["error"] = "Unknown method: " + method;
    }
    
    sendResponseToJS(requestId, result);
}

void WebViewAppWidget::sendResponseToJS(int requestId, const QVariantMap& result) {
    QJsonObject responseObj;
    responseObj["type"] = "logos_response";
    responseObj["requestId"] = requestId;
    
    QJsonObject resultObj;
    for (auto it = result.begin(); it != result.end(); ++it) {
        resultObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    responseObj["result"] = resultObj;
    
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
