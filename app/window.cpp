#include "window.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QPluginLoader>
#include <QDir>

Window::Window(QWidget *parent)
    : QMainWindow(parent)
    , m_logosAPI(nullptr)
{
    setupUi();
}

Window::Window(LogosAPI* logosAPI, QWidget *parent)
    : QMainWindow(parent)
    , m_logosAPI(logosAPI)
{
    setupUi();
}

Window::~Window()
{
}

void Window::setupUi()
{
    // Determine the appropriate plugin extension based on the platform
    QString pluginExtension;
    #if defined(Q_OS_WIN)
        pluginExtension = ".dll";
    #elif defined(Q_OS_MAC)
        pluginExtension = ".dylib";
    #else // Linux and other Unix-like systems
        pluginExtension = ".so";
    #endif

    // Load the main_ui plugin with the appropriate extension
    QString pluginPath = QCoreApplication::applicationDirPath() + "/plugins/main_ui" + pluginExtension;
    QPluginLoader loader(pluginPath);

    QWidget* mainContent = nullptr;

    if (loader.load()) {
        QObject* plugin = loader.instance();
        if (plugin) {
            // Try to create the main window using the plugin's createWidget method
            QMetaObject::invokeMethod(plugin, "createWidget",
                                    Qt::DirectConnection,
                                    Q_RETURN_ARG(QWidget*, mainContent),
                                    Q_ARG(LogosAPI*, m_logosAPI));
        }
    }

    if (mainContent) {
        setCentralWidget(mainContent);
    } else {
        qWarning() << "================================================";
        qWarning() << "Failed to load main UI plugin from:" << pluginPath;
        qWarning() << "Error:" << loader.errorString();
        qWarning() << "================================================";
        // Fallback: show a message when plugin is not found
        QWidget* fallbackWidget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(fallbackWidget);

        QLabel* messageLabel = new QLabel("No main UI module found", fallbackWidget);
        QFont font = messageLabel->font();
        font.setPointSize(14);
        messageLabel->setFont(font);
        messageLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(messageLabel);
        setCentralWidget(fallbackWidget);

        qWarning() << "Failed to load main UI plugin from:" << pluginPath;
    }

    // Set window title and size
    setWindowTitle("Logos Core POC");
    resize(1024, 768);
} 
