#pragma once

#include <IComponent.h>
#include <QObject>

class PackageManagerView;
class LogosAPI;

class PackageManagerUIPlugin : public QObject, public IComponent {
    Q_OBJECT
    Q_INTERFACES(IComponent)
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "metadata.json")

public:
    explicit PackageManagerUIPlugin(QObject* parent = nullptr);
    ~PackageManagerUIPlugin();

    // IComponent implementation
    QWidget* createWidget(LogosAPI* logosAPI = nullptr) override;
    void destroyWidget(QWidget* widget) override;

private:
    PackageManagerView* m_packageManagerView;
    LogosAPI* m_logosAPI;
};
