#include "package_manager_ui_plugin.h"
#include "PackageManagerBackend.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickStyle>

PackageManagerUIPlugin::PackageManagerUIPlugin(QObject* parent)
    : QObject(parent)
{
}

PackageManagerUIPlugin::~PackageManagerUIPlugin()
{
}

QWidget* PackageManagerUIPlugin::createWidget(LogosAPI* logosAPI)
{
    QQuickStyle::setStyle("Basic");

    QQuickWidget* quickWidget = new QQuickWidget();
    quickWidget->setMinimumSize(800, 600);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    PackageManagerBackend* backend = new PackageManagerBackend(logosAPI, quickWidget);
    quickWidget->rootContext()->setContextProperty("backend", backend);
    quickWidget->setSource(QUrl("qrc:/PackageManager.qml"));

    return quickWidget;
}

void PackageManagerUIPlugin::destroyWidget(QWidget* widget)
{
    delete widget;
}
