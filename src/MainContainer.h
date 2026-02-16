#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QStackedWidget>

class QQuickWidget;
class MainUIBackend;
class MdiView;
class LogosAPI;

class MainContainer : public QWidget
{
    Q_OBJECT

public:
    explicit MainContainer(LogosAPI* logosAPI = nullptr, QWidget* parent = nullptr);
    ~MainContainer();

    // Get the MDI view
    MdiView* getMdiView() const { return m_mdiView; }
    
    // Get the backend
    MainUIBackend* getBackend() const { return m_backend; }
    
    // Get the LogosAPI instance
    LogosAPI* getLogosAPI() const { return m_logosAPI; }

private slots:
    void onViewIndexChanged();
    void onNavigateToApps();
    void onPluginWindowRequested(QWidget* widget, const QString& title);
    void onPluginWindowRemoveRequested(QWidget* widget);
    void onPluginWindowActivateRequested(QWidget* widget);

private:
    void setupUi();
    QUrl resolveQmlUrl(const QString& qmlFile);
    
    // Main layout
    QHBoxLayout* m_mainLayout;
    
    // Sidebar (QML)
    QQuickWidget* m_sidebarWidget;
    
    // Content area
    QStackedWidget* m_contentStack;
    
    // MdiView (C++ widget for Apps)
    MdiView* m_mdiView;
    
    // Content views (QML for Dashboard, Modules, PackageManager, Settings)
    QQuickWidget* m_contentWidget;
    
    // Backend
    MainUIBackend* m_backend;
    
    // LogosAPI instance
    LogosAPI* m_logosAPI;
};

