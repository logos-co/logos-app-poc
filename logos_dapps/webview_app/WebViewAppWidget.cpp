#include "WebViewAppWidget.h"
#include "WebViewAppWidget_platform.h"
#include <QVBoxLayout>
#include <QUrl>

WebViewAppWidget::WebViewAppWidget(QWidget* parent) : QWidget(parent), webView(nullptr) {
    setupWebView();
}

void WebViewAppWidget::setupWebView() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
#if defined(Q_OS_MAC)
    webView = createMacWebViewWidget(this);
    if (webView) {
        layout->addWidget(webView);
        loadURLInMacWebViewWidget(webView, QUrl("https://en.wikipedia.org/wiki/Main_Page"));
    }
#elif defined(Q_OS_WIN)
    webView = createWinWebViewWidget(this);
    if (webView) {
        layout->addWidget(webView);
        loadURLInWinWebViewWidget(webView, QUrl("https://en.wikipedia.org/wiki/Main_Page"));
    }
#else
    // Linux
    webView = createLinuxWebViewWidget(this);
    if (webView) {
        layout->addWidget(webView);
        loadURLInLinuxWebViewWidget(webView, QUrl("https://en.wikipedia.org/wiki/Main_Page"));
    }
#endif
    
    // Set minimum size for the widget
    setMinimumSize(800, 600);
    // Set a reasonable default size
    resize(800, 600);
}
