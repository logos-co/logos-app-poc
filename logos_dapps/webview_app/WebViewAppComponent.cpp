#include "WebViewAppComponent.h"
#include "WebViewAppWidget.h"

QWidget* WebViewAppComponent::createWidget(LogosAPI* logosAPI) {
    return new WebViewAppWidget(logosAPI);
}

void WebViewAppComponent::destroyWidget(QWidget* widget) {
    delete widget;
}
