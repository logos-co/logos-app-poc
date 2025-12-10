#include "MainUIBackend.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include "logos_sdk.h"
#include "token_manager.h"

extern "C" {
    char* logos_core_get_module_stats();
}

MainUIBackend::MainUIBackend(LogosAPI* logosAPI, QObject* parent)
    : QObject(parent)
    , m_currentViewIndex(0)
    , m_logosAPI(logosAPI)
    , m_ownsLogosAPI(false)
    , m_statsTimer(nullptr)
{
    if (!m_logosAPI) {
        m_logosAPI = new LogosAPI("core", this);
        m_ownsLogosAPI = true;
    }
    
    initializeSidebarItems();
    
    m_statsTimer = new QTimer(this);
    connect(m_statsTimer, &QTimer::timeout, this, &MainUIBackend::updateModuleStats);
    m_statsTimer->start(2000);
    
    refreshUiModules();
    refreshCoreModules();
    refreshLauncherApps();
    
    qDebug() << "MainUIBackend created";
}

MainUIBackend::~MainUIBackend()
{
    QStringList moduleNames = m_loadedUiModules.keys();
    for (const QString& name : moduleNames) {
        unloadUiModule(name);
    }
}

void MainUIBackend::initializeSidebarItems()
{
    m_sidebarItems << "Apps" << "Dashboard" << "Modules" << "Settings";
    m_sidebarIcons << "qrc:/icons/home.png" << "qrc:/icons/chart.png" << "qrc:/icons/modules.png" 
                   << "qrc:/icons/settings.png";
}

int MainUIBackend::currentViewIndex() const
{
    return m_currentViewIndex;
}

void MainUIBackend::setCurrentViewIndex(int index)
{
    if (m_currentViewIndex != index && index >= 0 && index < m_sidebarItems.size()) {
        m_currentViewIndex = index;
        emit currentViewIndexChanged();
        
        if (m_sidebarItems[index] == "Modules") {
            refreshUiModules();
            refreshCoreModules();
        }
    }
}

QStringList MainUIBackend::sidebarItems() const
{
    return m_sidebarItems;
}

QStringList MainUIBackend::sidebarIcons() const
{
    return m_sidebarIcons;
}

QString MainUIBackend::sidebarIconAt(int index) const
{
    if (index >= 0 && index < m_sidebarIcons.size()) {
        return m_sidebarIcons[index];
    }
    return QString();
}

QVariantList MainUIBackend::uiModules() const
{
    QVariantList modules;
    QStringList availablePlugins = findAvailableUiPlugins();
    
    for (const QString& pluginName : availablePlugins) {
        QVariantMap module;
        module["name"] = pluginName;
        module["isLoaded"] = m_loadedUiModules.contains(pluginName);
        module["isMainUi"] = (pluginName == "main_ui");
        
        QString pluginPath = getPluginPath(pluginName);
        module["iconPath"] = getPluginIconPath(pluginPath);
        
        modules.append(module);
    }
    
    return modules;
}

void MainUIBackend::loadUiModule(const QString& moduleName)
{
    qDebug() << "Loading UI module:" << moduleName;
    
    if (m_loadedUiModules.contains(moduleName)) {
        qDebug() << "Module" << moduleName << "is already loaded";
        activateApp(moduleName);
        return;
    }
    
    QString pluginPath = getPluginPath(moduleName);
    qDebug() << "Loading plugin from:" << pluginPath;
    
    QPluginLoader loader(pluginPath);
    if (!loader.load()) {
        qDebug() << "Failed to load plugin:" << moduleName << "-" << loader.errorString();
        return;
    }
    
    QObject* plugin = loader.instance();
    if (!plugin) {
        qDebug() << "Failed to get plugin instance:" << moduleName << "-" << loader.errorString();
        return;
    }
    
    IComponent* component = qobject_cast<IComponent*>(plugin);
    if (!component) {
        qDebug() << "Failed to cast plugin to IComponent:" << moduleName;
        loader.unload();
        return;
    }
    
    QWidget* componentWidget = component->createWidget(m_logosAPI);
    if (!componentWidget) {
        qDebug() << "Component returned null widget:" << moduleName;
        loader.unload();
        return;
    }
    
    m_loadedUiModules[moduleName] = component;
    m_uiModuleWidgets[moduleName] = componentWidget;
    m_loadedApps.insert(moduleName);
    
    emit uiModulesChanged();
    emit launcherAppsChanged();
    
    emit pluginWindowRequested(componentWidget, moduleName);
    emit navigateToApps();
    
    qDebug() << "Successfully loaded UI module:" << moduleName;
}

void MainUIBackend::unloadUiModule(const QString& moduleName)
{
    qDebug() << "Unloading UI module:" << moduleName;
    
    if (!m_loadedUiModules.contains(moduleName)) {
        qDebug() << "Module" << moduleName << "is not loaded";
        return;
    }
    
    QWidget* widget = m_uiModuleWidgets.value(moduleName);
    IComponent* component = m_loadedUiModules.value(moduleName);
    
    if (widget) {
        emit pluginWindowRemoveRequested(widget);
    }
    
    if (component && widget) {
        component->destroyWidget(widget);
    }
    
    m_loadedUiModules.remove(moduleName);
    m_uiModuleWidgets.remove(moduleName);
    m_loadedApps.remove(moduleName);
    
    emit uiModulesChanged();
    emit launcherAppsChanged();
    
    qDebug() << "Successfully unloaded UI module:" << moduleName;
}

void MainUIBackend::refreshUiModules()
{
    emit uiModulesChanged();
}

void MainUIBackend::activateApp(const QString& appName)
{
    QWidget* widget = m_uiModuleWidgets.value(appName);
    if (widget) {
        emit pluginWindowActivateRequested(widget);
        emit navigateToApps();
    }
}

void MainUIBackend::onPluginWindowClosed(const QString& pluginName)
{
    qDebug() << "Plugin window closed:" << pluginName;
    
    if (m_loadedUiModules.contains(pluginName)) {
        IComponent* component = m_loadedUiModules.value(pluginName);
        QWidget* widget = m_uiModuleWidgets.value(pluginName);
        
        if (component && widget) {
            component->destroyWidget(widget);
        }
        
        m_loadedUiModules.remove(pluginName);
        m_uiModuleWidgets.remove(pluginName);
        m_loadedApps.remove(pluginName);
        
        emit uiModulesChanged();
        emit launcherAppsChanged();
    }
}

QVariantList MainUIBackend::coreModules() const
{
    QVariantList modules;
    
    if (!m_logosAPI) {
        return modules;
    }
    
    LogosAPIClient* coreManagerClient = m_logosAPI->getClient("core_manager");
    if (!coreManagerClient || !coreManagerClient->isConnected()) {
        qWarning() << "Core manager client is not available";
        return modules;
    }
    
    LogosModules logos(m_logosAPI);
    QJsonArray pluginsArray = logos.core_manager.getKnownPlugins();
    
    for (const QJsonValue& val : pluginsArray) {
        QJsonObject pluginObj = val.toObject();
        QString name = pluginObj["name"].toString();
        QVariantMap module;
        module["name"] = name;
        module["isLoaded"] = pluginObj["loaded"].toBool();
        
        if (m_moduleStats.contains(name)) {
            module["cpu"] = m_moduleStats[name]["cpu"];
            module["memory"] = m_moduleStats[name]["memory"];
        } else {
            module["cpu"] = "0.0";
            module["memory"] = "0.0";
        }
        
        modules.append(module);
    }
    
    return modules;
}

void MainUIBackend::loadCoreModule(const QString& moduleName)
{
    qDebug() << "Loading core module:" << moduleName;
    
    if (!m_logosAPI) {
        qWarning() << "LogosAPI not available";
        return;
    }
    
    LogosAPIClient* coreManagerClient = m_logosAPI->getClient("core_manager");
    if (!coreManagerClient || !coreManagerClient->isConnected()) {
        qWarning() << "Core manager client is not available";
        return;
    }
    
    LogosModules logos(m_logosAPI);
    bool success = logos.core_manager.loadPlugin(moduleName);
    
    if (success) {
        qDebug() << "Successfully loaded core module:" << moduleName;
        emit coreModulesChanged();
    } else {
        qDebug() << "Failed to load core module:" << moduleName;
    }
}

void MainUIBackend::unloadCoreModule(const QString& moduleName)
{
    qDebug() << "Unloading core module:" << moduleName;
    
    if (!m_logosAPI) {
        qWarning() << "LogosAPI not available";
        return;
    }
    
    LogosAPIClient* coreManagerClient = m_logosAPI->getClient("core_manager");
    if (!coreManagerClient || !coreManagerClient->isConnected()) {
        qWarning() << "Core manager client is not available";
        return;
    }
    
    LogosModules logos(m_logosAPI);
    bool success = logos.core_manager.unloadPlugin(moduleName);
    
    if (success) {
        qDebug() << "Successfully unloaded core module:" << moduleName;
        emit coreModulesChanged();
    } else {
        qDebug() << "Failed to unload core module:" << moduleName;
    }
}

void MainUIBackend::refreshCoreModules()
{
    emit coreModulesChanged();
}

QString MainUIBackend::getCoreModuleMethods(const QString& moduleName)
{
    if (!m_logosAPI) {
        return "[]";
    }
    
    LogosAPIClient* client = m_logosAPI->getClient(moduleName);
    if (!client || !client->isConnected()) {
        return "[]";
    }
    
    QVariant result = client->invokeRemoteMethod(moduleName, "getMethods");
    if (result.canConvert<QJsonArray>()) {
        QJsonArray methods = result.toJsonArray();
        QJsonDocument doc(methods);
        return doc.toJson(QJsonDocument::Compact);
    }
    
    return "[]";
}

QString MainUIBackend::callCoreModuleMethod(const QString& moduleName, const QString& methodName, const QString& argsJson)
{
    if (!m_logosAPI) {
        return "{\"error\": \"LogosAPI not available\"}";
    }
    
    LogosAPIClient* client = m_logosAPI->getClient(moduleName);
    if (!client || !client->isConnected()) {
        return "{\"error\": \"Module not connected\"}";
    }
    
    QJsonDocument argsDoc = QJsonDocument::fromJson(argsJson.toUtf8());
    QJsonArray argsArray = argsDoc.array();
    
    QVariantList args;
    for (const QJsonValue& val : argsArray) {
        args.append(val.toVariant());
    }
    
    QVariant result;
    if (args.isEmpty()) {
        result = client->invokeRemoteMethod(moduleName, methodName);
    } else if (args.size() == 1) {
        result = client->invokeRemoteMethod(moduleName, methodName, args[0]);
    } else if (args.size() == 2) {
        result = client->invokeRemoteMethod(moduleName, methodName, args[0], args[1]);
    } else if (args.size() == 3) {
        result = client->invokeRemoteMethod(moduleName, methodName, args[0], args[1], args[2]);
    } else {
        return "{\"error\": \"Too many arguments\"}";
    }
    
    QJsonObject wrapper;
    wrapper["result"] = QJsonValue::fromVariant(result);
    QJsonDocument resultDoc(wrapper);
    return resultDoc.toJson(QJsonDocument::Compact);
}

QVariantList MainUIBackend::launcherApps() const
{
    QVariantList apps;
    QStringList availablePlugins = findAvailableUiPlugins();
    
    for (const QString& pluginName : availablePlugins) {
        if (pluginName == "main_ui") {
            continue;
        }
        
        QVariantMap app;
        app["name"] = pluginName;
        app["isLoaded"] = m_loadedApps.contains(pluginName);
        
        QString pluginPath = getPluginPath(pluginName);
        app["iconPath"] = getPluginIconPath(pluginPath);
        
        apps.append(app);
    }
    
    return apps;
}

void MainUIBackend::onAppLauncherClicked(const QString& appName)
{
    qDebug() << "App launcher clicked:" << appName;
    
    if (m_loadedApps.contains(appName)) {
        activateApp(appName);
    } else {
        loadUiModule(appName);
    }
}

void MainUIBackend::refreshLauncherApps()
{
    emit launcherAppsChanged();
}

QStringList MainUIBackend::findAvailableUiPlugins() const
{
    QStringList plugins;
    
    QString pluginsPath = QCoreApplication::applicationDirPath() + "/../plugins";
    QDir pluginsDir(pluginsPath);
    
    if (!pluginsDir.exists()) {
        qWarning() << "Plugins directory does not exist:" << pluginsPath;
        return plugins;
    }
    
    QStringList entries = pluginsDir.entryList(QDir::Files);
    
    QString libExtension;
    int extLength;
#if defined(Q_OS_MAC)
    libExtension = ".dylib";
    extLength = 6;
#elif defined(Q_OS_WIN)
    libExtension = ".dll";
    extLength = 4;
#else
    libExtension = ".so";
    extLength = 3;
#endif
    
    for (const QString& entry : entries) {
        if (entry.endsWith(libExtension)) {
            if (entry.startsWith("lib")) {
                continue;
            }
            
            QString pluginName = entry;
            pluginName.chop(extLength);
            plugins.append(pluginName);
        }
    }
    
    return plugins;
}

QString MainUIBackend::getPluginPath(const QString& name) const
{
    QString libExtension;
#if defined(Q_OS_MAC)
    libExtension = ".dylib";
#elif defined(Q_OS_WIN)
    libExtension = ".dll";
#else
    libExtension = ".so";
#endif
    
    return QCoreApplication::applicationDirPath() + "/../plugins/" + name + libExtension;
}

QString MainUIBackend::getPluginIconPath(const QString& pluginPath) const
{
    QPluginLoader loader(pluginPath);
    
    if (loader.load()) {
        QJsonObject metadata = loader.metaData();
        QJsonObject metaDataObj = metadata.value("MetaData").toObject();
        
        if (metaDataObj.contains("icon")) {
            QString iconPath = metaDataObj.value("icon").toString();
            
            if (iconPath.startsWith(":/")) {
                iconPath = "qrc" + iconPath;
            }
            
            return iconPath;
        }
        
        loader.unload();
    }
    
    return "";
}

void MainUIBackend::updateModuleStats()
{
    char* stats_json = logos_core_get_module_stats();
    if (!stats_json) {
        return;
    }
    
    QString jsonStr = QString::fromUtf8(stats_json);
    QJsonDocument doc = QJsonDocument::fromJson(stats_json);
    free(stats_json);
    
    if (doc.isNull()) {
        qWarning() << "Failed to parse module stats JSON";
        return;
    }
    
    QJsonArray modulesArray;
    if (doc.isArray()) {
        modulesArray = doc.array();
    } else if (doc.isObject()) {
        QJsonObject root = doc.object();
        modulesArray = root["modules"].toArray();
    }
    
    for (const QJsonValue& val : modulesArray) {
        QJsonObject moduleObj = val.toObject();
        QString name = moduleObj["name"].toString();
        
        if (!name.isEmpty()) {
            QVariantMap stats;
            double cpu = moduleObj["cpu_percent"].toDouble();
            if (cpu == 0) cpu = moduleObj["cpu"].toDouble();
            
            double memory = moduleObj["memory_mb"].toDouble();
            if (memory == 0) memory = moduleObj["memory"].toDouble();
            if (memory == 0) memory = moduleObj["memory_MB"].toDouble();
            
            stats["cpu"] = QString::number(cpu, 'f', 1);
            stats["memory"] = QString::number(memory, 'f', 1);
            m_moduleStats[name] = stats;
        }
    }
    
    emit coreModulesChanged();
}
