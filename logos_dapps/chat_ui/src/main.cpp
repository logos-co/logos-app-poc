#include <QApplication>
#include <QDebug>
#include "ChatWindow.h"
#include "chat_api.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    qDebug() << "Starting Chat Qt Application";
    
    // Create and show the main window
    ChatWindow mainWindow;
    mainWindow.setWindowTitle("Logos Chat App");
    mainWindow.resize(800, 600);
    mainWindow.show();
    
    return app.exec();
} 