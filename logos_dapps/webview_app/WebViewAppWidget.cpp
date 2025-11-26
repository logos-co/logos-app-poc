#include "WebViewAppWidget.h"
#include "WebViewAppWidget_platform.h"
#include "WebViewBridge.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimer>
#include <QDateTime>

WebViewAppWidget::WebViewAppWidget(QWidget* parent) 
    : QWidget(parent), webView(nullptr), m_wikipediaButton(nullptr), m_localFileButton(nullptr),
      m_qtButton(nullptr), m_statusLabel(nullptr), m_bridge(nullptr) {
    // Create bridge instance
    m_bridge = new WebViewBridge(this);
    connect(m_bridge, &WebViewBridge::requestReceived, this, &WebViewAppWidget::onBridgeRequest);
    
    setupWebView();
}

WebViewAppWidget::~WebViewAppWidget() {
    // Bridge will be deleted automatically as child of this widget
}

void WebViewAppWidget::setupWebView() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create horizontal layout for buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(5, 5, 5, 5);
    
    // Create buttons
    m_wikipediaButton = new QPushButton("Wikipedia", this);
    m_localFileButton = new QPushButton("Local File", this);
    m_qtButton = new QPushButton("Send Event to WebApp", this);
    
    // Create status label
    m_statusLabel = new QLabel("Status: Ready", this);
    m_statusLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc; }");
    
    // Connect button signals to slots
    connect(m_wikipediaButton, &QPushButton::clicked, this, &WebViewAppWidget::onWikipediaClicked);
    connect(m_localFileButton, &QPushButton::clicked, this, &WebViewAppWidget::onLocalFileClicked);
    connect(m_qtButton, &QPushButton::clicked, this, &WebViewAppWidget::onQtButtonClicked);
    
    // Add buttons to horizontal layout
    buttonLayout->addWidget(m_wikipediaButton);
    buttonLayout->addWidget(m_localFileButton);
    buttonLayout->addWidget(m_qtButton);
    buttonLayout->addStretch(); // Push buttons to the left
    
    // Add button layout to main layout
    mainLayout->addLayout(buttonLayout);
    
    // Add status label
    mainLayout->addWidget(m_statusLabel);
    
#if defined(Q_OS_MAC)
    webView = createMacWebViewWidget(this);
#elif defined(Q_OS_WIN)
    webView = createWinWebViewWidget(this);
#else
    // Linux
    webView = createLinuxWebViewWidget(this);
#endif
    
    if (webView) {
        // Inject JavaScript bridge
        injectJavaScriptBridge();
        
        mainLayout->addWidget(webView);
        loadURLInWebView(QUrl("https://en.wikipedia.org/wiki/Main_Page"));
    }
    
    // Set minimum size for the widget
    setMinimumSize(800, 600);
    // Set a reasonable default size
    resize(800, 600);
}

void WebViewAppWidget::onWikipediaClicked() {
    if (!webView) return;
    
    QUrl wikipediaUrl("https://en.wikipedia.org/wiki/Main_Page");
    loadURLInWebView(wikipediaUrl);
}

void WebViewAppWidget::onLocalFileClicked() {
    loadLocalHTML();
}

void WebViewAppWidget::loadLocalHTML() {
    if (!webView) return;
    
    // Get the path to the local.html file relative to the executable
    QDir appDir(QCoreApplication::applicationDirPath());
    QString htmlPath = appDir.absoluteFilePath("local.html");
    
    // Check if file exists, if not try source directory
    if (!QFileInfo::exists(htmlPath)) {
        // Try to find it in the source directory
        QDir sourceDir = QDir::current();
        if (sourceDir.cd("logos_dapps") && sourceDir.cd("webview_app")) {
            QString sourcePath = sourceDir.absoluteFilePath("local.html");
            if (QFileInfo::exists(sourcePath)) {
                htmlPath = sourcePath;
            }
        }
    }
    
    qDebug() << "Loading local HTML from:" << htmlPath;
    qDebug() << "File exists:" << QFileInfo::exists(htmlPath);
    
    // Convert to file:// URL
    QUrl fileUrl = QUrl::fromLocalFile(htmlPath);
    qDebug() << "File URL:" << fileUrl.toString();
    
    loadURLInWebView(fileUrl);
}

void WebViewAppWidget::loadURLInWebView(const QUrl& url) {
    if (!webView) return;
    
#if defined(Q_OS_MAC)
    loadURLInMacWebViewWidget(webView, url);
#elif defined(Q_OS_WIN)
    loadURLInWinWebViewWidget(webView, url);
#else
    loadURLInLinuxWebViewWidget(webView, url);
#endif
    
    // Re-inject bridge after page load (with delay to ensure page is ready)
    QTimer::singleShot(500, this, &WebViewAppWidget::injectJavaScriptBridge);
}

void WebViewAppWidget::onBridgeRequest(const QString& method, const QVariantMap& params) {
    qDebug() << "WebViewAppWidget: Handling request - method:" << method << "params:" << params;
    
    if (method == "changeQtLabel") {
        QString text = params.value("text").toString();
        if (m_statusLabel) {
            m_statusLabel->setText("Status: " + text);
            m_statusLabel->setStyleSheet("QLabel { background-color: #d4edda; padding: 5px; border: 1px solid #28a745; }");
            
            // Send response back to JavaScript
            QVariantMap response;
            response["success"] = true;
            response["message"] = "Label updated successfully";
            
            // For now, we'll use a simple approach - in a real implementation,
            // we'd need to track request IDs and send proper responses
            // This is a simplified version for the demo
        }
    } else {
        qDebug() << "WebViewAppWidget: Unknown method:" << method;
    }
}

void WebViewAppWidget::onQtButtonClicked() {
    QVariantMap eventData;
    eventData["message"] = "Hello from Qt!";
    eventData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    sendEventToWebApp("qtButtonClicked", eventData);
}

void WebViewAppWidget::executeJavaScript(const QString& script) {
    if (!webView) return;
    
#if defined(Q_OS_MAC)
    executeJavaScriptInMacWebView(webView, script);
#elif defined(Q_OS_WIN)
    executeJavaScriptInWinWebView(webView, script);
#else
    executeJavaScriptInLinuxWebView(webView, script);
#endif
}

void WebViewAppWidget::sendEventToWebApp(const QString& eventName, const QVariantMap& data) {
    QJsonObject eventObj;
    eventObj["type"] = "logos_event";
    eventObj["eventName"] = eventName;
    
    QJsonObject dataObj;
    for (auto it = data.begin(); it != data.end(); ++it) {
        dataObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    eventObj["data"] = dataObj;
    
    QJsonDocument doc(eventObj);
    QString script = QString("window.postMessage(%1, '*');").arg(QString::fromUtf8(doc.toJson()));
    
    executeJavaScript(script);
}

void WebViewAppWidget::injectJavaScriptBridge() {
    if (!webView || !m_bridge) return;
    
#if defined(Q_OS_MAC)
    injectJavaScriptBridgeInMacWebView(webView, m_bridge);
#elif defined(Q_OS_WIN)
    injectJavaScriptBridgeInWinWebView(webView, m_bridge);
#else
    injectJavaScriptBridgeInLinuxWebView(webView, m_bridge);
#endif
}
