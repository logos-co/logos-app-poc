#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include <QStackedWidget>

class PluginMethodsView;

class CoreModuleView : public QWidget
{
    Q_OBJECT

public:
    explicit CoreModuleView(QWidget *parent = nullptr);
    ~CoreModuleView();
    
    void updatePluginList();

private slots:
    void onViewMethodsClicked();
    void onBackToPluginList();
    void onLoadPluginClicked();
    void onUnloadPluginClicked();
    void onAddPluginClicked();

private:
    void setupUi();
    void createPluginList();

    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QListWidget* m_pluginList;
    QTimer* m_updateTimer;

    // Stacked widget to hold the plugin list and plugin methods views
    QStackedWidget* m_stackedWidget;

    // The plugins list container widget
    QWidget* m_pluginsListWidget;

    // The current plugin methods view (if any)
    PluginMethodsView* m_currentMethodsView;
}; 