#include "logos_api.h"
#include "token_manager.h"
#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QStringList>
#include <QStringList>
#include <QDebug>
#include <QMetaObject>
#include <QPluginLoader>
#include <IComponent.h>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QWidget>
#include <QProcessEnvironment>

// Replace CoreManager with direct C API functions
extern "C" {
    void logos_core_set_plugins_dir(const char* plugins_dir);
    void logos_core_add_plugins_dir(const char* plugins_dir);
    void logos_core_start();
    void logos_core_cleanup();
    char** logos_core_get_loaded_plugins();
    int logos_core_load_plugin(const char* plugin_name);
    char* logos_core_process_plugin(const char* plugin_path);
}

static QString pluginExtension() {
#if defined(Q_OS_WIN)
    return ".dll";
#elif defined(Q_OS_MAC)
    return ".dylib";
#else
    return ".so";
#endif
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString modulesDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../modules");
    logos_core_set_plugins_dir(modulesDir.toUtf8().constData());

    QFileInfo bundledDirInfo(modulesDir);
    if (!bundledDirInfo.isWritable()) {
        QString userModulesDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/modules";
        logos_core_add_plugins_dir(userModulesDir.toUtf8().constData());
    }

    logos_core_start();

    QString pluginExt = pluginExtension();
    logos_core_process_plugin((modulesDir + "/package_manager_plugin" + pluginExt).toUtf8().constData());
    if (!logos_core_load_plugin("package_manager"))
        qWarning() << "Failed to load package_manager plugin by default.";

    LogosAPI logosAPI("core", nullptr);

    app.setWindowIcon(QIcon(":/icons/logos.png"));
    app.setQuitOnLastWindowClosed(false);

    QString pluginsDir = QCoreApplication::applicationDirPath() + "/../plugins/";

    QString packageManagerPluginPath = pluginsDir + "package_manager_ui/package_manager_ui" + pluginExt;
    QPluginLoader packageManagerLoader(packageManagerPluginPath);
    QWidget* packageManagerWidget = nullptr;
    if (packageManagerLoader.load()) {
        QObject* pmPlugin = packageManagerLoader.instance();
        if (pmPlugin) {
            IComponent* component = qobject_cast<IComponent*>(pmPlugin);
            if (component) {
                packageManagerWidget = component->createWidget(&logosAPI);
            }
        }
    }
    if (!packageManagerWidget)
        qWarning() << "Failed to load package_manager_ui plugin:" << packageManagerLoader.errorString();

    QString mainUiPluginPath = pluginsDir + "main_ui/main_ui" + pluginExt;
    QPluginLoader mainUiLoader(mainUiPluginPath);
    QWidget* mainContent = nullptr;
    QObject* mainUiPlugin = nullptr;
    if (mainUiLoader.load()) {
        mainUiPlugin = mainUiLoader.instance();
        if (mainUiPlugin) {
            QMetaObject::invokeMethod(mainUiPlugin, "createWidget",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(QWidget*, mainContent),
                                      Q_ARG(LogosAPI*, &logosAPI));
        }
    }
    if (!mainContent) {
        qWarning() << "Failed to load main_ui plugin from:" << mainUiPluginPath;
        return 1;
    }
    if (packageManagerWidget && mainUiPlugin) {
        QMetaObject::invokeMethod(mainUiPlugin, "setPackageManagerWidget",
                                  Qt::DirectConnection,
                                  Q_ARG(QWidget*, packageManagerWidget));
    }

    // MainContainer must have a QWindow for embedding: create as top-level then embed
    mainContent->setParent(nullptr);
    mainContent->setAttribute(Qt::WA_NativeWindow, true);
    if (!mainContent->windowHandle()) {
        mainContent->winId(); // force native window creation
    }
    QWindow* mainContentWindow = mainContent->windowHandle();
    if (!mainContentWindow) {
        qWarning() << "MainContainer has no QWindow, cannot embed in QML.";
        return 1;
    }

    // --- QML engine: load main.qml (ApplicationWindow) ---
    QQmlApplicationEngine engine;

    // Expose MainContainer's QWindow so QML can embed it via WindowContainer
    engine.rootContext()->setContextProperty("mainContentWindow", QVariant::fromValue(mainContentWindow));

    // Import paths for Logos.DesignSystem and main_ui QML
    QString qmlUiPath = QProcessEnvironment::systemEnvironment().value("QML_UI", "");
    if (!qmlUiPath.isEmpty()) {
        QDir qmlDir(qmlUiPath);
        engine.addImportPath(qmlDir.absoluteFilePath("qml"));
        engine.addImportPath(qmlDir.absolutePath());
    }
    engine.addImportPath("qrc:/qml");
    // Design system: try env and common locations
    QString designQml = QProcessEnvironment::systemEnvironment().value("LOGOS_DESIGN_SYSTEM_QML", "");
    if (!designQml.isEmpty()) {
        engine.addImportPath(designQml);
    } else {
        // Fallbacks: sibling logos-design-system or app resources
        QDir appDir(QCoreApplication::applicationDirPath());
        QStringList tryPaths;
        tryPaths << appDir.absoluteFilePath("../Resources/qml")
                 << appDir.absoluteFilePath("../qml")
                 << appDir.absoluteFilePath("../../logos-design-system/src/qml");
        for (const QString& p : tryPaths) {
            if (QDir(p).exists())
                engine.addImportPath(p);
        }
    }

    // Load main.qml from filesystem when MAIN_QML_PATH is set (e.g. run-dev.sh), else from qrc
    QStringList qmlCandidates;
    QString envPath = QProcessEnvironment::systemEnvironment().value("MAIN_QML_PATH", "");
    if (!envPath.isEmpty()) {
        if (QFileInfo(envPath).isFile())
            qmlCandidates << QFileInfo(envPath).absoluteFilePath();
        else
            qmlCandidates << QDir(envPath).absoluteFilePath("main.qml");
    }
    QUrl mainQml;
    for (const QString& p : qmlCandidates)
        if (QFileInfo::exists(p)) { mainQml = QUrl::fromLocalFile(p); break; }
    if (mainQml.isEmpty())
        mainQml = QUrl(QStringLiteral("qrc:/main.qml"));

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(mainQml);
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "No root QML object created.";
        return 1;
    }

    QObject* root = engine.rootObjects().first();
    QQuickWindow* quickWindow = qobject_cast<QQuickWindow*>(root);
    if (!quickWindow) {
        qWarning() << "Root object is not a QQuickWindow.";
        return 1;
    }
    // --- System tray: show/hide the QML window ---
    QSystemTrayIcon* trayIcon = nullptr;
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(&app);
        QIcon icon(":/icons/logos.png");
        if (icon.isNull())
            icon = QIcon::fromTheme("application-x-executable");
        trayIcon->setIcon(icon);
        trayIcon->setToolTip("Logos Core POC");

        auto showOrHide = [quickWindow]() {
            if (quickWindow->isVisible())
                quickWindow->hide();
            else {
                quickWindow->show();
                quickWindow->raise();
                quickWindow->requestActivate();
            }
        };
        QMenu* trayMenu = new QMenu();
        QObject::connect(trayMenu->addAction(QObject::tr("Show/Hide")), &QAction::triggered, &app, showOrHide);
        trayMenu->addSeparator();
        QObject::connect(trayMenu->addAction(QObject::tr("Quit")), &QAction::triggered, &app, &QApplication::quit);
        trayIcon->setContextMenu(trayMenu);
        QObject::connect(trayIcon, &QSystemTrayIcon::activated, &app, [showOrHide](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::MiddleClick)
                showOrHide();
        });
        trayIcon->show();
    }

    engine.rootContext()->setContextProperty("trayIconAvailable", QVariant(trayIcon != nullptr));

    quickWindow->show();

    int result = app.exec();
    logos_core_cleanup();
    return result;
}
