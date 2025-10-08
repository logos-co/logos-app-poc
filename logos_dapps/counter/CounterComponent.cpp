#include "CounterComponent.h"
#include "CounterWidget.h"

QWidget* CounterComponent::createWidget(LogosAPI* logosAPI) {
    // LogosAPI parameter available but not used in this simple widget
    return new CounterWidget();
}

void CounterComponent::destroyWidget(QWidget* widget) {
    delete widget;
} 