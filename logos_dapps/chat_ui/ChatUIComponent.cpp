#include "ChatUIComponent.h"
#include "src/ChatWidget.h"

QWidget* ChatUIComponent::createWidget(LogosAPI* logosAPI) {
    // LogosAPI parameter available but not used - ChatWidget creates its own
    return new ChatWidget();
}

void ChatUIComponent::destroyWidget(QWidget* widget) {
    delete widget;
}
