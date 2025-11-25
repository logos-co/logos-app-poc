#include "window.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QPluginLoader>
#include <QDir>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QIcon>
#include <QPixmap>

Window::Window(QWidget *parent)
    : QMainWindow(parent)
    , m_logosAPI(nullptr)
    , m_trayIcon(nullptr)
    , m_trayIconMenu(nullptr)
    , m_showHideAction(nullptr)
    , m_quitAction(nullptr)
{
    setupUi();
    createTrayIcon();
}

Window::Window(LogosAPI* logosAPI, QWidget *parent)
    : QMainWindow(parent)
    , m_logosAPI(logosAPI)
    , m_trayIcon(nullptr)
    , m_trayIconMenu(nullptr)
    , m_showHideAction(nullptr)
    , m_quitAction(nullptr)
{
    setupUi();
    createTrayIcon();
}

Window::~Window()
{
    if (m_trayIcon) {
        delete m_trayIcon;
    }
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
    QString pluginPath = QCoreApplication::applicationDirPath() + "/../plugins/main_ui" + pluginExtension;
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

void Window::createTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray is not available on this system";
        return;
    }

    // Create tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    setIcon();
    m_trayIcon->setToolTip("Logos Core POC");

    // Create context menu
    m_trayIconMenu = new QMenu(this);

    m_showHideAction = m_trayIconMenu->addAction(tr("Show/Hide"));
    m_showHideAction->setCheckable(false);
    connect(m_showHideAction, &QAction::triggered, this, &Window::showHideWindow);

    m_trayIconMenu->addSeparator();

    m_quitAction = m_trayIconMenu->addAction(tr("Quit"));
    connect(m_quitAction, &QAction::triggered, this, &Window::quitApplication);

    m_trayIcon->setContextMenu(m_trayIconMenu);

    // Connect icon activation signal
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &Window::iconActivated);

    // Show the tray icon
    m_trayIcon->show();
}

void Window::setIcon()
{
    if (!m_trayIcon) {
        return;
    }

    QIcon icon(":/icons/logos.png");
    if (icon.isNull()) {
        qWarning() << "Failed to load tray icon from resource";
        // Create a simple fallback icon
        icon = QIcon::fromTheme("application-x-executable");
        if (icon.isNull()) {
            // Last resort: create a minimal icon
            QPixmap pixmap(16, 16);
            pixmap.fill(Qt::blue);
            icon = QIcon(pixmap);
        }
    }
    m_trayIcon->setIcon(icon);
}

void Window::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        // Hide the window instead of closing
        hide();
        event->ignore();
        
        // Show a message to inform the user
        if (m_trayIcon->supportsMessages()) {
            m_trayIcon->showMessage(
                tr("Logos Core POC"),
                tr("The application will continue to run in the system tray. "
                   "Click the tray icon to restore the window."),
                QSystemTrayIcon::Information,
                2000
            );
        }
    } else {
        // If system tray is not available, quit normally
        event->accept();
    }
}

void Window::showHideWindow()
{
    if (isVisible()) {
        hide();
    } else {
        show();
        raise();
        activateWindow();
    }
}

void Window::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        // Single click - toggle window visibility
        showHideWindow();
        break;
    case QSystemTrayIcon::DoubleClick:
        // Double click - also toggle (some platforms use double click)
        showHideWindow();
        break;
    case QSystemTrayIcon::MiddleClick:
        // Middle click - toggle as well
        showHideWindow();
        break;
    default:
        break;
    }
}

void Window::quitApplication()
{
    // Hide tray icon first
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    
    // Quit the application
    QApplication::quit();
} 
