#include "CounterComponent.h"
#include "CounterBackend.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickStyle>

QWidget* CounterComponent::createWidget(LogosAPI* logosAPI) {
    Q_UNUSED(logosAPI)

    QQuickStyle::setStyle("Basic");

    QQuickWidget* quickWidget = new QQuickWidget();
    quickWidget->setMinimumSize(200, 150);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    CounterBackend* backend = new CounterBackend(quickWidget);
    quickWidget->rootContext()->setContextProperty("counterBackend", backend);
    quickWidget->setSource(QUrl("qrc:/Counter.qml"));

    return quickWidget;
}

void CounterComponent::destroyWidget(QWidget* widget) {
    delete widget;
} 