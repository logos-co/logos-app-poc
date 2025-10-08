#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QStatusBar>
#include "ChatWidget.h"

class ChatWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ChatWindow(QWidget* parent = nullptr);
    ~ChatWindow();

private slots:
    void onAboutAction();
    void onInitWaku();
    void onStopWaku();
    void onRetrieveHistory();
    
private:
    void setupMenu();
    void setupStatusBar();
    
    ChatWidget* chatWidget;
    QStatusBar* statusBar;
}; 