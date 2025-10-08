#include "ChatWindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDebug>

ChatWindow::ChatWindow(QWidget* parent) : QMainWindow(parent) {
    // Create central widget
    chatWidget = new ChatWidget(this);
    setCentralWidget(chatWidget);
    
    // Setup the UI
    setupMenu();
    setupStatusBar();
    
    // Set window properties
    setMinimumSize(640, 480);
}

ChatWindow::~ChatWindow() {
    // ChatWidget will be deleted automatically as it's a child of this window
}

void ChatWindow::setupMenu() {
    // Create main menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    QMenu* wakuMenu = menuBar()->addMenu("&Waku");
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    
    // File menu actions
    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // Waku menu actions
    QAction* initWakuAction = wakuMenu->addAction("&Initialize Waku");
    connect(initWakuAction, &QAction::triggered, this, &ChatWindow::onInitWaku);
    
    QAction* stopWakuAction = wakuMenu->addAction("&Stop Waku");
    connect(stopWakuAction, &QAction::triggered, this, &ChatWindow::onStopWaku);
    
    QAction* retrieveHistoryAction = wakuMenu->addAction("Retrieve &History");
    connect(retrieveHistoryAction, &QAction::triggered, this, &ChatWindow::onRetrieveHistory);
    
    // Help menu actions
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &ChatWindow::onAboutAction);
}

void ChatWindow::setupStatusBar() {
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");
}

void ChatWindow::onAboutAction() {
    QMessageBox::about(this, "About Logos Chat",
                     "Logos Chat Application\n\n"
                     "A sample Qt application demonstrating Waku integration.");
}

void ChatWindow::onInitWaku() {
    qDebug() << "Initializing Waku from menu";
    chatWidget->initWaku();
    statusBar->showMessage("Waku initialization requested");
}

void ChatWindow::onStopWaku() {
    qDebug() << "Stopping Waku from menu";
    chatWidget->stopWaku();
    statusBar->showMessage("Waku stop requested");
}

void ChatWindow::onRetrieveHistory() {
    qDebug() << "Retrieving message history from menu";
    chatWidget->retrieveMessageHistory();
    statusBar->showMessage("Message history retrieval requested");
} 