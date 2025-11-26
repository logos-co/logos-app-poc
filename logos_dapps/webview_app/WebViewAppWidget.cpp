#include "WebViewAppWidget.h"
#include "WebViewAppWidget_platform.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>

WebViewAppWidget::WebViewAppWidget(QWidget* parent) 
    : QWidget(parent), webView(nullptr), m_wikipediaButton(nullptr), m_localFileButton(nullptr) {
    setupWebView();
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
    
    // Connect button signals to slots
    connect(m_wikipediaButton, &QPushButton::clicked, this, &WebViewAppWidget::onWikipediaClicked);
    connect(m_localFileButton, &QPushButton::clicked, this, &WebViewAppWidget::onLocalFileClicked);
    
    // Add buttons to horizontal layout
    buttonLayout->addWidget(m_wikipediaButton);
    buttonLayout->addWidget(m_localFileButton);
    buttonLayout->addStretch(); // Push buttons to the left
    
    // Add button layout to main layout
    mainLayout->addLayout(buttonLayout);
    
#if defined(Q_OS_MAC)
    webView = createMacWebViewWidget(this);
#elif defined(Q_OS_WIN)
    webView = createWinWebViewWidget(this);
#else
    // Linux
    webView = createLinuxWebViewWidget(this);
#endif
    
    if (webView) {
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
}
