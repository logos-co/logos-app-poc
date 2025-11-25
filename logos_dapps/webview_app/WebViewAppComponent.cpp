#include "WebViewAppComponent.h"
#include "WebViewAppWidget.h"

QWidget* WebViewAppComponent::createWidget(LogosAPI* logosAPI) {
    // LogosAPI parameter available but not used in this simple widget
    return new WebViewAppWidget();
}

void WebViewAppComponent::destroyWidget(QWidget* widget) {
    delete widget;
}
