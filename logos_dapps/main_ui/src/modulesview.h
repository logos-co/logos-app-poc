#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>
#include <QDebug>
#include <IComponent.h>
#include "mdiview.h"
#include <QLabel>
#include <QPixmap>

class MainWindow;

class ModulesView : public QWidget
{
    Q_OBJECT

public:
    explicit ModulesView(QWidget *parent = nullptr, MainWindow *mainWindow = nullptr);
    ~ModulesView();
    
    // Refresh the plugin list
    void refreshPluginList();
    
    // Get list of available apps (excluding "main_ui")
    QStringList getAvailableApps();
    
    // Check if an app is currently loaded
    bool isAppLoaded(const QString& appName);
    
    // Get icon for an app
    QPixmap getAppIcon(const QString& appName);
    
    // Get widget for a loaded app
    QWidget* getAppWidget(const QString& appName);

signals:
    // Signal emitted when app state changes (loaded/unloaded)
    void appStateChanged(const QString& appName, bool isLoaded);

public slots:
    void onLoadComponent(const QString& name);

private slots:
    void onUnloadComponent(const QString& name);

private:
    void setupUi();
    void setupPluginButtons(QVBoxLayout* buttonLayout);
    QString getPluginPath(const QString& name);
    QStringList findAvailablePlugins();
    void updateButtonStates(const QString& name, bool isEnabled = true);
    void clearPluginList();
    QStringList getDependencies(const QString& pluginName);
    void loadDependenciesRecursive(const QString& pluginName, QSet<QString>& loading);
    void loadComponentInternal(const QString& name);
    
    QMap<QString, IComponent*> m_loadedComponents;
    QMap<QString, QWidget*> m_componentWidgets;
    QMap<QWidget*, QString> m_widgetToAppName; // Reverse mapping: widget -> app name
    QMap<QString, QPushButton*> m_loadButtons;
    QMap<QString, QPushButton*> m_unloadButtons;
    
    // Reference to the main window to access the MDI view
    MainWindow* m_mainWindow;
    MdiView* m_mdiView;
    
    QVBoxLayout* m_layout;
    QVBoxLayout* m_buttonLayout;

    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QLabel *m_contentLabel;
}; 