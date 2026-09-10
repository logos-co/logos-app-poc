// shell-preview — the Basecamp UI shell, with no Logos code in the process.
//
// Loads the real, shipped main_ui.so and drives it through IShellHost with
// fixture data. main_ui links no Logos library (verified: zero Logos entries in
// its NEEDED list), so nothing here needs liblogos, logos-protocol,
// logos-qt-host, ui-host or any module.
//
// What it is for: building and iterating on the UI without the runtime, and
// without tracking a core rework.
//
// What it is NOT: UI plugins do not load. PluginLoader is host-side and pulls
// in LogosAPI and ui-host.

#include "FixtureShellHost.h"
#include "IShellHost.h"
#include "IShellView.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QPluginLoader>

#if defined(SHELL_PREVIEW_STATIC_SHELL)
// main_ui built with MAIN_UI_STATIC is linked in (always on iOS, where static
// Qt cannot dlopen a plugin) and reached through staticInstances() instead.
Q_IMPORT_PLUGIN(MainShellView)
#endif

namespace {

IShellView* resolveShell(const QString& shellPath)
{
#if defined(SHELL_PREVIEW_STATIC_SHELL)
    Q_UNUSED(shellPath);
    for (QObject* instance : QPluginLoader::staticInstances()) {
        if (auto* shell = qobject_cast<IShellView*>(instance))
            return shell;
    }
    qFatal("No static plugin implements IShellView; was main_ui linked with Q_IMPORT_PLUGIN?");
#else
    auto* loader = new QPluginLoader(shellPath, QCoreApplication::instance());
    if (!loader->load())
        qFatal("Failed to load the shell from %s: %s",
               qUtf8Printable(shellPath), qUtf8Printable(loader->errorString()));

    auto* shell = qobject_cast<IShellView*>(loader->instance());
    if (!shell)
        qFatal("%s does not implement IShellView", qUtf8Printable(shellPath));
    return shell;
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Logos");
    app.setApplicationName("BasecampShellPreview");

    QCommandLineParser parser;
    parser.setApplicationDescription("Basecamp UI shell against a fixture, no Logos runtime");
    parser.addHelpOption();
    QCommandLineOption shellOpt({"s", "shell"}, "Path to main_ui plugin", "path");
    QCommandLineOption fixtureOpt({"f", "fixture"}, "Path to shell fixture JSON", "path");
    parser.addOption(shellOpt);
    parser.addOption(fixtureOpt);
    parser.process(app);

    QString shellPath = parser.value(shellOpt);
    if (shellPath.isEmpty()) {
#if defined(Q_OS_WIN)
        const QString ext = ".dll";
#elif defined(Q_OS_MAC)
        const QString ext = ".dylib";
#else
        const QString ext = ".so";
#endif
        shellPath = QDir::cleanPath(QCoreApplication::applicationDirPath()
                                    + "/../plugins/main_ui/main_ui" + ext);
    }

    QJsonObject fixture;
    const QString fixturePath = parser.value(fixtureOpt);
    if (!fixturePath.isEmpty()) {
        QFile f(fixturePath);
        if (!f.open(QIODevice::ReadOnly))
            qFatal("Could not open fixture %s", qUtf8Printable(fixturePath));
        fixture = QJsonDocument::fromJson(f.readAll()).object();
    } else {
        QFile f(":/shell-preview/shell-fixture.json");
        if (f.open(QIODevice::ReadOnly))
            fixture = QJsonDocument::fromJson(f.readAll()).object();
    }

    IShellView* shell = resolveShell(shellPath);

    // Same check the real host makes: a shell built against a different
    // revision of IShellHost.h would jump through a mismatched vtable.
    if (shell->hostAbiVersion() != IShellHost_abi)
        qFatal("shell was built against IShellHost ABI %d, host is %d",
               shell->hostAbiVersion(), IShellHost_abi);
    qInfo("shell-preview: shell reports IShellHost ABI %d, host has %d", shell->hostAbiVersion(),
          IShellHost_abi);

    FixtureShellHost host(fixture);

    QMainWindow window;
    window.setWindowTitle("Logos Basecamp — shell preview (fixture data)");
    window.setCentralWidget(shell->createShell(&host));
    host.replaySection();
#if defined(Q_OS_IOS)
    window.showFullScreen();
#else
    window.resize(1280, 860);
    window.show();
#endif

    const int rc = app.exec();
    shell->destroyShell(window.centralWidget());
    return rc;
}
