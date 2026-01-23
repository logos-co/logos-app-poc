#include "MainUIBackend.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QTimer>
#include <QQmlContext>
#include <QQuickWidget>
#include <QQmlEngine>
#include <QQmlError>
#include <QUrl>
#include <QStandardPaths>
#include <QFileDialog>
#include <QTemporaryDir>
#include "LogosQmlBridge.h"
#include "logos_sdk.h"
#include "token_manager.h"
#include "restricted/DenyAllNAMFactory.h"
#include "restricted/RestrictedUrlInterceptor.h"
#include "lgx.h"

extern "C" {
    char* logos_core_get_module_stats();
    char* logos_core_process_plugin(const char* plugin_path);
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
    
    subscribeToPackageInstallationEvents();
    
    qDebug() << "MainUIBackend created";
}

MainUIBackend::~MainUIBackend()
{
    QStringList moduleNames = m_loadedUiModules.keys();
    for (const QString& name : m_qmlPluginWidgets.keys()) {
        if (!moduleNames.contains(name)) {
            moduleNames.append(name);
        }
    }

    for (const QString& name : moduleNames) {
        unloadUiModule(name);
    }
}

void MainUIBackend::subscribeToPackageInstallationEvents()
{
    if (!m_logosAPI) {
        return;
    }
    
    LogosAPIClient* client = m_logosAPI->getClient("package_manager");
    if (!client || !client->isConnected()) {
        return;
    }
    
    LogosModules logos(m_logosAPI);
    logos.package_manager.on("packageInstallationFinished", [this](const QVariantList& data) {
        if (data.size() < 3) {
            return;
        }
        bool success = data[1].toBool();
        
        if (success) {
            QTimer::singleShot(100, this, [this]() {
                refreshUiModules();
                refreshCoreModules();
                refreshLauncherApps();
            });
        }
    });
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
        module["isLoaded"] = m_loadedUiModules.contains(pluginName) || m_qmlPluginWidgets.contains(pluginName);
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
    
    if (m_loadedUiModules.contains(moduleName) || m_qmlPluginWidgets.contains(moduleName)) {
        qDebug() << "Module" << moduleName << "is already loaded";
        activateApp(moduleName);
        return;
    }
    
    QString pluginPath = getPluginPath(moduleName);
    qDebug() << "Loading plugin from:" << pluginPath;

    if (isQmlPlugin(moduleName)) {
        QJsonObject metadata = readQmlPluginMetadata(moduleName);
        QString mainFile = metadata.value("main").toString("Main.qml");
        QString qmlFilePath = QDir(pluginPath).filePath(mainFile);

        if (!QFile::exists(qmlFilePath)) {
            qWarning() << "Main QML file does not exist for plugin" << moduleName << ":" << qmlFilePath;
            return;
        }

        QQuickWidget* qmlWidget = new QQuickWidget;
        qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
        QQmlEngine* engine = qmlWidget->engine();
        if (engine) {
            QStringList importPaths;
            importPaths << QStringLiteral("qrc:/qt-project.org/imports");
            importPaths << QStringLiteral("qrc:/qt/qml");
            importPaths << pluginPath;     // Plugin-local imports only
            qDebug() << "=======================> QML import paths:" << importPaths;
            engine->setImportPathList(importPaths);

            QStringList pluginPaths;
            // note: commented out to keep this list empty to lock the plugin down
            // could cause issues with other QML components but it's unclear the moment
            //const QString qtPluginPath = QLibraryInfo::path(QLibraryInfo::PluginsPath);
            //if (!qtPluginPath.isEmpty()) {
            //    pluginPaths << qtPluginPath;  // Required for QtQuick C++ backends
            //}
            qDebug() << "=======================> QML plugin paths:" << pluginPaths;
            engine->setPluginPathList(pluginPaths);

            engine->setNetworkAccessManagerFactory(new DenyAllNAMFactory());
            QStringList allowedRoots;
            // allowedRoots << QStringLiteral("qrc:/qt/qml");
            allowedRoots << pluginPath;
            qDebug() << "=======================> QML allowed roots:" << allowedRoots;
            engine->addUrlInterceptor(new RestrictedUrlInterceptor(allowedRoots));
            qDebug() << "=======================> QML base url:" << QUrl::fromLocalFile(pluginPath + "/");
            engine->setBaseUrl(QUrl::fromLocalFile(pluginPath + "/"));
        }
        LogosQmlBridge* bridge = new LogosQmlBridge(m_logosAPI, qmlWidget);
        qmlWidget->rootContext()->setContextProperty("logos", bridge);
        qmlWidget->setSource(QUrl::fromLocalFile(qmlFilePath));

        if (qmlWidget->status() == QQuickWidget::Error) {
            qWarning() << "Failed to load QML plugin" << moduleName;
            const auto errors = qmlWidget->errors();
            for (const QQmlError& error : errors) {
                qWarning() << error.toString();
            }
            qmlWidget->deleteLater();
            return;
        }

        m_qmlPluginWidgets[moduleName] = qmlWidget;
        m_uiModuleWidgets[moduleName] = qmlWidget;
        m_loadedApps.insert(moduleName);

        emit uiModulesChanged();
        emit launcherAppsChanged();

        emit pluginWindowRequested(qmlWidget, moduleName);
        emit navigateToApps();

        qDebug() << "Successfully loaded QML UI module:" << moduleName;
        return;
    }
    
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
    
    bool isQml = m_qmlPluginWidgets.contains(moduleName);
    bool isCpp = m_loadedUiModules.contains(moduleName);

    if (!isQml && !isCpp) {
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

    if (isQml && widget) {
        widget->deleteLater();
    }
    
    m_loadedUiModules.remove(moduleName);
    m_uiModuleWidgets.remove(moduleName);
    m_qmlPluginWidgets.remove(moduleName);
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
    } else if (m_qmlPluginWidgets.contains(pluginName)) {
        QWidget* widget = m_qmlPluginWidgets.value(pluginName);
        if (widget) {
            widget->deleteLater();
        }
        
        m_qmlPluginWidgets.remove(pluginName);
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
    QString libExtension;
#if defined(Q_OS_MAC)
    libExtension = "*.dylib";
#elif defined(Q_OS_WIN)
    libExtension = "*.dll";
#else
    libExtension = "*.so";
#endif
    
    QDir modulesDir(modulesDirectory());
    if (modulesDir.exists()) {
        QStringList entries = modulesDir.entryList(QStringList() << libExtension, QDir::Files);
        for (const QString& entry : entries) {
            QString fullPath = modulesDir.absoluteFilePath(entry);
            logos_core_process_plugin(fullPath.toUtf8().constData());
        }
    }
    
    QFileInfo bundledDirInfo(modulesDirectory());
    if (!bundledDirInfo.isWritable()) {
        QString userModulesDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/modules";
        QDir userModulesDirObj(userModulesDir);
        if (userModulesDirObj.exists()) {
            QStringList entries = userModulesDirObj.entryList(QStringList() << libExtension, QDir::Files);
            for (const QString& entry : entries) {
                QString fullPath = userModulesDirObj.absoluteFilePath(entry);
                logos_core_process_plugin(fullPath.toUtf8().constData());
            }
        }
    }
    
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

void MainUIBackend::openInstallPluginDialog()
{
    QString filter;
#if defined(Q_OS_MAC)
    filter = "Plugin Files (*.dylib *.lgx);;Dynamic Library (*.dylib);;LGX Package (*.lgx);;All Files (*)";
#elif defined(Q_OS_WIN)
    filter = "Plugin Files (*.dll *.lgx);;Dynamic Link Library (*.dll);;LGX Package (*.lgx);;All Files (*)";
#else
    filter = "Plugin Files (*.so *.lgx);;Shared Object (*.so);;LGX Package (*.lgx);;All Files (*)";
#endif
    
    QString filePath = QFileDialog::getOpenFileName(nullptr, tr("Select Plugin to Install"), QString(), filter);
    
    if (!filePath.isEmpty()) {
        installPluginFromPath(filePath);
    }
}

void MainUIBackend::installPluginFromPath(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    
#ifdef LOGOS_DISTRIBUTED_BUILD
    // For distributed builds (DMG/AppImage), always use Application Support
    QString targetDir = userPluginsDirectory();
#else
    // For development builds (nix), use bundled plugins directory
    QString targetDir = pluginsDirectory();
#endif
    
    QDir dir;
    
    if (!dir.exists(targetDir)) {
        dir.mkpath(targetDir);
    }
    
    if (fileInfo.suffix().toLower() == "lgx") {
        qDebug() << "Installing LGX package:" << filePath;
        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            qWarning() << "Failed to create temporary directory for LGX extraction";
            return;
        }
        
        QString errorMsg;
        if (!extractLgxPackage(filePath, tempDir.path(), errorMsg)) {
            qWarning() << "Failed to extract LGX package:" << errorMsg;
            return;
        }
        
        if (!copyLibraryFromExtracted(tempDir.path(), targetDir, false, errorMsg)) {
            qWarning() << "Failed to copy library from extracted LGX package:" << errorMsg;
            return;
        }
        
        qDebug() << "Successfully installed plugin from LGX package to:" << targetDir;
    } else {
        // UI plugins: subdirectory structure
        QString pluginName = fileInfo.completeBaseName();
        QString pluginSubDir = targetDir + "/" + pluginName;
        QDir().mkpath(pluginSubDir);
        QString targetPath = pluginSubDir + "/" + fileInfo.fileName();
        
        if (QFile::exists(targetPath)) {
            QFile::remove(targetPath);
        }
        
        QFile::copy(filePath, targetPath);
    }
    
    emit uiModulesChanged();
    emit launcherAppsChanged();
}

void MainUIBackend::openInstallCoreModuleDialog()
{
    QString filter;
#if defined(Q_OS_MAC)
    filter = "Module Files (*.dylib *.lgx);;Dynamic Library (*.dylib);;LGX Package (*.lgx);;All Files (*)";
#elif defined(Q_OS_WIN)
    filter = "Module Files (*.dll *.lgx);;Dynamic Link Library (*.dll);;LGX Package (*.lgx);;All Files (*)";
#else
    filter = "Module Files (*.so *.lgx);;Shared Object (*.so);;LGX Package (*.lgx);;All Files (*)";
#endif
    
    QString filePath = QFileDialog::getOpenFileName(nullptr, tr("Select Core Module to Install"), QString(), filter);
    
    if (!filePath.isEmpty()) {
        installCoreModuleFromPath(filePath);
    }
}

void MainUIBackend::installCoreModuleFromPath(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    
#ifdef LOGOS_DISTRIBUTED_BUILD
    // For distributed builds (DMG/AppImage), use Application Support
    QString targetDir = userModulesDirectory();
#else
    // For development builds (nix), use bundled modules directory
    QString targetDir = modulesDirectory();
#endif
    
    QDir dir;
    
    if (!dir.exists(targetDir)) {
        dir.mkpath(targetDir);
    }
    
    if (fileInfo.suffix().toLower() == "lgx") {
        qDebug() << "Installing LGX package as core module:" << filePath;
        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            qWarning() << "Failed to create temporary directory for LGX extraction";
            return;
        }
        
        QString errorMsg;
        if (!extractLgxPackage(filePath, tempDir.path(), errorMsg)) {
            qWarning() << "Failed to extract LGX package:" << errorMsg;
            return;
        }
        
        if (!copyLibraryFromExtracted(tempDir.path(), targetDir, true, errorMsg)) {
            qWarning() << "Failed to copy library from extracted LGX package:" << errorMsg;
            return;
        }
        
        qDebug() << "Successfully installed core module from LGX package to:" << targetDir;
    } else {
        QString targetPath = targetDir + "/" + fileInfo.fileName();
        
        if (QFile::exists(targetPath)) {
            QFile::remove(targetPath);
        }
        
        QFile::copy(filePath, targetPath);
    }
    
    refreshCoreModules();
}

QString MainUIBackend::pluginsDirectory() const
{
    return QCoreApplication::applicationDirPath() + "/../plugins";
}

QString MainUIBackend::userPluginsDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/plugins";
}

QString MainUIBackend::modulesDirectory() const
{
    return QCoreApplication::applicationDirPath() + "/../modules";
}

QString MainUIBackend::userModulesDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/modules";
}

QJsonObject MainUIBackend::readQmlPluginMetadata(const QString& pluginName) const
{
    QFileInfo bundledPluginsDirInfo(pluginsDirectory());
    if (!bundledPluginsDirInfo.isWritable()) {
        QString userMetadataPath = userPluginsDirectory() + "/" + pluginName + "/metadata.json";
        QFile userMetadataFile(userMetadataPath);
        if (userMetadataFile.exists() && userMetadataFile.open(QIODevice::ReadOnly)) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(userMetadataFile.readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                return doc.object();
            }
        }
    }
    
    QString metadataPath = pluginsDirectory() + "/" + pluginName + "/metadata.json";
    QFile metadataFile(metadataPath);
    if (!metadataFile.exists()) {
        return QJsonObject();
    }

    if (!metadataFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open metadata for plugin" << pluginName
                   << ":" << metadataFile.errorString();
        return QJsonObject();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(metadataFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse metadata for plugin" << pluginName
                   << ":" << parseError.errorString();
        return QJsonObject();
    }

    return doc.object();
}

bool MainUIBackend::isQmlPlugin(const QString& name) const
{
    QJsonObject metadata = readQmlPluginMetadata(name);
    QString pluginType = metadata.value("pluginType").toString();
    return pluginType.compare("qml", Qt::CaseInsensitive) == 0;
}

QStringList MainUIBackend::findAvailableUiPlugins() const
{
    QStringList plugins;

    auto scanDirectory = [&](const QString& dirPath) {
        QDir pluginsDir(dirPath);
        
        if (!pluginsDir.exists()) {
            return;
        }

        auto addPlugin = [&](const QString& name) {
            if (!plugins.contains(name)) {
                plugins.append(name);
            }
        };

        QString libExtension;
#if defined(Q_OS_MAC)
        libExtension = ".dylib";
#elif defined(Q_OS_WIN)
        libExtension = ".dll";
#else
        libExtension = ".so";
#endif

        // Scan subdirectories for plugins (both QML and C++ plugins are in subdirectories)
        QStringList dirEntries = pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : dirEntries) {
            // Check if it's a QML plugin (has metadata.json with pluginType: "qml")
            QJsonObject metadata = readQmlPluginMetadata(entry);
            QString pluginType = metadata.value("pluginType").toString();
            if (pluginType.compare("qml", Qt::CaseInsensitive) == 0) {
                addPlugin(entry);
            } else {
                // Check if it's a C++ plugin (has <dirname>.<ext> inside the subdirectory)
                QString pluginLibPath = dirPath + "/" + entry + "/" + entry + libExtension;
                if (QFile::exists(pluginLibPath)) {
                    addPlugin(entry);
                }
            }
        }
    };
    
    scanDirectory(pluginsDirectory());
    
    QFileInfo bundledPluginsDirInfo(pluginsDirectory());
    if (!bundledPluginsDirInfo.isWritable()) {
        scanDirectory(userPluginsDirectory());
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
    
    QFileInfo bundledPluginsDirInfo(pluginsDirectory());
    bool shouldCheckUserDir = !bundledPluginsDirInfo.isWritable();
    
    if (isQmlPlugin(name)) {
        // QML plugins: return directory path (unchanged)
        if (shouldCheckUserDir) {
            QString userPath = userPluginsDirectory() + "/" + name;
            if (QFileInfo::exists(userPath)) {
                return userPath;
            }
        }
        
        return pluginsDirectory() + "/" + name;
    }

    // C++ plugins: return path to dylib inside subdirectory
    if (shouldCheckUserDir) {
        QString userPluginPath = userPluginsDirectory() + "/" + name + "/" + name + libExtension;
        if (QFile::exists(userPluginPath)) {
            return userPluginPath;
        }
    }
    
    return pluginsDirectory() + "/" + name + "/" + name + libExtension;
}

QString MainUIBackend::getPluginIconPath(const QString& pluginPath) const
{
    QFileInfo pluginInfo(pluginPath);
    if (pluginInfo.isDir()) {
        QFile metadataFile(pluginInfo.filePath() + "/metadata.json");
        if (!metadataFile.exists()) {
            return "";
        }

        if (!metadataFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open metadata for QML plugin icon at" << metadataFile.fileName()
                       << ":" << metadataFile.errorString();
            return "";
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(metadataFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "Failed to parse metadata for QML plugin icon at" << metadataFile.fileName()
                       << ":" << parseError.errorString();
            return "";
        }

        QJsonObject metaDataObj = doc.object();
        if (metaDataObj.contains("icon")) {
            QString iconPath = metaDataObj.value("icon").toString();

            if (iconPath.startsWith(":/")) {
                iconPath = "qrc" + iconPath;
            } else if (!iconPath.isEmpty()) {
                QString resolvedPath = pluginInfo.filePath() + "/" + iconPath;
                iconPath = QUrl::fromLocalFile(resolvedPath).toString();
            }

            return iconPath;
        }

        return "";
    }

    QPluginLoader loader(pluginPath);
    QJsonObject metadata = loader.metaData();
    QJsonObject metaDataObj = metadata.value("MetaData").toObject();
    
    if (metaDataObj.contains("icon")) {
        QString iconPath = metaDataObj.value("icon").toString();
        
        if (iconPath.startsWith(":/")) {
            iconPath = "qrc" + iconPath;
        }
        
        return iconPath;
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

QString MainUIBackend::currentPlatformVariant() const
{
#if defined(Q_OS_MAC)
  #if defined(__aarch64__) || defined(__arm64__)
    return "darwin-arm64";
  #else
    return "darwin-amd64";
  #endif
#elif defined(Q_OS_LINUX)
  #if defined(__aarch64__) || defined(__arm64__)
    return "linux-arm64";
  #else
    return "linux-amd64";
  #endif
#elif defined(Q_OS_WIN)
  #if defined(__aarch64__) || defined(__arm64__)
    return "windows-arm64";
  #else
    return "windows-amd64";
  #endif
#else
    return "unknown";
#endif
}

bool MainUIBackend::extractLgxPackage(const QString& lgxPath, const QString& outputDir, QString& errorMsg)
{
    lgx_package_t pkg = lgx_load(lgxPath.toUtf8().constData());
    if (!pkg) {
        errorMsg = QString("Failed to load LGX package: %1").arg(lgx_get_last_error());
        return false;
    }
    
    QString variant = currentPlatformVariant();
    qDebug() << "Extracting variant:" << variant << "from LGX package";
    
    if (!lgx_has_variant(pkg, variant.toUtf8().constData())) {
        errorMsg = QString("Package does not contain variant for platform: %1").arg(variant);
        lgx_free_package(pkg);
        return false;
    }
    
    lgx_result_t result = lgx_extract(pkg, variant.toUtf8().constData(), outputDir.toUtf8().constData());
    
    if (!result.success) {
        errorMsg = QString("Failed to extract variant: %1").arg(result.error ? result.error : "unknown error");
        lgx_free_package(pkg);
        return false;
    }
    
    lgx_free_package(pkg);
    return true;
}

bool MainUIBackend::copyLibraryFromExtracted(const QString& extractedDir, const QString& targetDir, bool isCoreModule, QString& errorMsg)
{
    QString variant = currentPlatformVariant();
    QString variantDir = extractedDir + "/" + variant;
    
    if (!QDir(variantDir).exists()) {
        errorMsg = QString("Extracted variant directory not found: %1").arg(variantDir);
        return false;
    }
    
    QDir dir(variantDir);
    QStringList filters;
#if defined(Q_OS_MAC)
    filters << "*.dylib";
#elif defined(Q_OS_WIN)
    filters << "*.dll";
#else
    filters << "*.so";
#endif
    
    QFileInfoList libraryFiles = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    
    if (libraryFiles.isEmpty()) {
        errorMsg = QString("No library files found in extracted variant directory: %1").arg(variantDir);
        return false;
    }
    
    for (const QFileInfo& fileInfo : libraryFiles) {
        QString sourceFile = fileInfo.absoluteFilePath();
        QString targetPath;
        
        if (isCoreModule) {
            // Core modules: flat structure
            targetPath = targetDir + "/" + fileInfo.fileName();
        } else {
            // UI plugins: subdirectory structure
            QString pluginName = fileInfo.completeBaseName();
            QString pluginSubDir = targetDir + "/" + pluginName;
            QDir().mkpath(pluginSubDir);
            targetPath = pluginSubDir + "/" + fileInfo.fileName();
        }
        
        if (QFile::exists(targetPath)) {
            if (!QFile::remove(targetPath)) {
                errorMsg = QString("Failed to remove existing file: %1").arg(targetPath);
                return false;
            }
        }
        
        if (!QFile::copy(sourceFile, targetPath)) {
            errorMsg = QString("Failed to copy library from %1 to %2").arg(sourceFile, targetPath);
            return false;
        }
        
        qDebug() << "Copied library file:" << targetPath;
    }
    
    return true;
}
