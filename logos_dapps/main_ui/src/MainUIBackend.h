#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QSet>
#include <QTimer>
#include <QPluginLoader>
#include "logos_api.h"
#include "logos_api_client.h"
#include "IComponent.h"

class QQuickWidget;

class MainUIBackend : public QObject {
    Q_OBJECT
    
    // Navigation
    Q_PROPERTY(int currentViewIndex READ currentViewIndex WRITE setCurrentViewIndex NOTIFY currentViewIndexChanged)
    Q_PROPERTY(QStringList sidebarItems READ sidebarItems CONSTANT)
    Q_PROPERTY(QStringList sidebarIcons READ sidebarIcons CONSTANT)
    
    // UI Modules (Apps)
    Q_PROPERTY(QVariantList uiModules READ uiModules NOTIFY uiModulesChanged)
    
    // Core Modules
    Q_PROPERTY(QVariantList coreModules READ coreModules NOTIFY coreModulesChanged)
    
    // App Launcher
    Q_PROPERTY(QVariantList launcherApps READ launcherApps NOTIFY launcherAppsChanged)

public:
    explicit MainUIBackend(LogosAPI* logosAPI = nullptr, QObject* parent = nullptr);
    ~MainUIBackend();
    
    // Navigation
    int currentViewIndex() const;
    void setCurrentViewIndex(int index);
    QStringList sidebarItems() const;
    QStringList sidebarIcons() const;
    Q_INVOKABLE QString sidebarIconAt(int index) const;
    
    // UI Modules
    QVariantList uiModules() const;
    
    // Core Modules
    QVariantList coreModules() const;
    
    // App Launcher
    QVariantList launcherApps() const;

public slots:
    // UI Module operations
    void loadUiModule(const QString& moduleName);
    void unloadUiModule(const QString& moduleName);
    void refreshUiModules();
    void activateApp(const QString& appName);
    Q_INVOKABLE void installPluginFromPath(const QString& filePath);
    Q_INVOKABLE void openInstallPluginDialog();
    
    // LGX package operations
    QString currentPlatformVariant() const;
    bool extractLgxPackage(const QString& lgxPath, const QString& outputDir, QString& errorMsg);
    
    // Core Module operations
    void loadCoreModule(const QString& moduleName);
    void unloadCoreModule(const QString& moduleName);
    Q_INVOKABLE void refreshCoreModules();
    Q_INVOKABLE QString getCoreModuleMethods(const QString& moduleName);
    Q_INVOKABLE QString callCoreModuleMethod(const QString& moduleName, const QString& methodName, const QString& argsJson);
    Q_INVOKABLE void installCoreModuleFromPath(const QString& filePath);
    Q_INVOKABLE void openInstallCoreModuleDialog();
    
    // Helper to copy library files from extracted directory
    bool copyLibraryFromExtracted(const QString& extractedDir, const QString& targetDir, QString& errorMsg);
    
    // App Launcher operations
    void onAppLauncherClicked(const QString& appName);
    void refreshLauncherApps();
    
    // Called when a plugin window is closed from MdiView
    void onPluginWindowClosed(const QString& pluginName);

signals:
    void currentViewIndexChanged();
    void uiModulesChanged();
    void coreModulesChanged();
    void launcherAppsChanged();
    void navigateToApps();
    
    // Signals for C++ MdiView coordination
    void pluginWindowRequested(QWidget* widget, const QString& title);
    void pluginWindowRemoveRequested(QWidget* widget);
    void pluginWindowActivateRequested(QWidget* widget);

private:
    void initializeSidebarItems();
    void subscribeToPackageInstallationEvents();
    QStringList findAvailableUiPlugins() const;
    QString getPluginPath(const QString& name) const;
    QString pluginsDirectory() const;
    QString userPluginsDirectory() const;
    QString modulesDirectory() const;
    QString userModulesDirectory() const;
    bool isQmlPlugin(const QString& name) const;
    QJsonObject readQmlPluginMetadata(const QString& pluginName) const;
    void updateModuleStats();
    QString getPluginIconPath(const QString& pluginPath) const;
    
    // Navigation state
    int m_currentViewIndex;
    QStringList m_sidebarItems;
    QStringList m_sidebarIcons;
    
    // UI Modules state
    QMap<QString, IComponent*> m_loadedUiModules;
    QMap<QString, QWidget*> m_uiModuleWidgets;
    QMap<QString, QQuickWidget*> m_qmlPluginWidgets;
    
    // Core Modules state
    QTimer* m_statsTimer;
    QMap<QString, QVariantMap> m_moduleStats;  // Stores per-module CPU/memory stats
    
    // App Launcher state
    QSet<QString> m_loadedApps;
    
    // LogosAPI
    LogosAPI* m_logosAPI;
    bool m_ownsLogosAPI;
};
