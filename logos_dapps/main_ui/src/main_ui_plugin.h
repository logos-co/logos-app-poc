#pragma once

#include <IComponent.h>
#include <QObject>

class MainWindow;
class CoreModuleView;
class LogosAPI;

class MainUIPlugin : public QObject, public IComponent
{
    Q_OBJECT
    Q_INTERFACES(IComponent)
    Q_PLUGIN_METADATA(IID IComponent_iid FILE "metadata.json")

public:
    explicit MainUIPlugin(QObject* parent = nullptr);
    ~MainUIPlugin();

    // IComponent implementation
    Q_INVOKABLE QWidget* createWidget(LogosAPI* logosAPI = nullptr) override;
    void destroyWidget(QWidget* widget) override;

    // UI-specific methods
    Q_INVOKABLE CoreModuleView* createCoreModuleView(QWidget* parent = nullptr);

private:
    MainWindow* m_mainWindow;
    CoreModuleView* m_coreModuleView;
    LogosAPI* m_logosAPI;
}; 