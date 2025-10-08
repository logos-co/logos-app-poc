#ifndef MDIVIEW_H
#define MDIVIEW_H

#include <QWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QMap>

class MdiView : public QWidget
{
    Q_OBJECT

public:
    explicit MdiView(QWidget *parent = nullptr);
    ~MdiView();
    
    // Add a plugin widget as an MDI window
    QMdiSubWindow* addPluginWindow(QWidget* pluginWidget, const QString& title);
    
    // Remove a plugin window
    void removePluginWindow(QWidget* pluginWidget);

private slots:
    void addMdiWindow();
    void toggleViewMode();
    void updateTabCloseButtons();

private:
    void setupUi();
    
    QMdiArea *mdiArea;
    QPushButton *addButton;
    QPushButton *toggleButton;
    QToolBar *toolBar;
    QVBoxLayout *mainLayout;
    
    // Map to keep track of plugin widgets and their MDI windows
    QMap<QWidget*, QMdiSubWindow*> m_pluginWindows;
    
    int windowCounter;
};

#endif // MDIVIEW_H 