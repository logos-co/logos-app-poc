#include "ChatWidget.h"
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <iostream>
#include <csignal>
#include <QTimer>
#include "logos_api_client.h"

// Static pointer to the active ChatWidget for callbacks
static ChatWidget* activeWidget = nullptr;

// Static callback that can be passed to the C API
void ChatWidget::handleWakuMessage(const std::string& timestamp, const std::string& nick, const std::string& message) {
    qDebug() << "RECEIVED: [" << QString::fromStdString(timestamp) << "] " 
             << QString::fromStdString(nick) << ": " 
             << QString::fromStdString(message);
    
    // Forward to the active widget if available
    if (activeWidget) {
        QMetaObject::invokeMethod(activeWidget, [=]() {
            activeWidget->displayMessage(QString::fromStdString(nick), QString::fromStdString(message));
        }, Qt::QueuedConnection);
    }
}

ChatWidget::ChatWidget(QWidget* parent) 
    : QWidget(parent), 
      isWakuInitialized(false),
      isWakuRunning(false),
      m_logosAPI(nullptr) {
    
    // Set as the active widget
    activeWidget = this;
    
    m_logosAPI = new LogosAPI("core", this);
    logos = new LogosModules(m_logosAPI);
    
    // Generate random username with 2 digits that will persist during this class lifetime
    int randomNum = rand() % 100;
    username = QString("LogosUser_%1").arg(randomNum, 2, 10, QChar('0'));
    qDebug() << "Generated username for this session: " << username;
    
    // Main vertical layout
    mainLayout = new QVBoxLayout(this);
    
    // Create status label
    statusLabel = new QLabel("Status: Not initialized", this);
    statusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusLabel->setLineWidth(1);
    statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusLabel->setMinimumHeight(30);
    
    // Create channel join layout
    channelLayout = new QHBoxLayout();
    channelInput = new QLineEdit(this);
    channelInput->setPlaceholderText("Enter channel name...");
    joinButton = new QPushButton("Join", this);
    
    channelLayout->addWidget(new QLabel("Channel:"));
    channelLayout->addWidget(channelInput, 4);
    channelLayout->addWidget(joinButton, 1);
    
    // Create chat display
    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setMinimumHeight(300);
    
    // Create input layout
    inputLayout = new QHBoxLayout();
    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Type your message here...");
    sendButton = new QPushButton("Send", this);
    
    inputLayout->addWidget(messageInput, 4);
    inputLayout->addWidget(sendButton, 1);
    
    // Add all components to main layout
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(channelLayout);
    mainLayout->addWidget(chatDisplay);
    mainLayout->addLayout(inputLayout);
    
    // Set spacing and margins
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Connect signals to slots
    connect(sendButton, &QPushButton::clicked, this, &ChatWidget::onSendButtonClicked);
    connect(joinButton, &QPushButton::clicked, this, &ChatWidget::onJoinChannelClicked);
    connect(messageInput, &QLineEdit::returnPressed, this, &ChatWidget::onSendButtonClicked);
    connect(channelInput, &QLineEdit::returnPressed, this, &ChatWidget::onJoinChannelClicked);
    
    // Disable UI components until Waku is initialized
    channelInput->setEnabled(false);
    joinButton->setEnabled(false);
    messageInput->setEnabled(false);
    sendButton->setEnabled(false);
    
    // Auto-initialize Waku
    initWaku();
}

ChatWidget::~ChatWidget() {
    // Reset the active widget if it's this instance
    if (activeWidget == this) {
        activeWidget = nullptr;
    }
    
    // Cleanup is now handled by the plugin
    stopWaku();
}

void ChatWidget::initWaku()
{
    updateStatus("Status: Initializing Waku...");

    if (!logos->chat.on("chatMessage", [this](const QVariantList& data) {
            if (data.size() < 3) {
                qWarning() << "ChatWidget: chatMessage payload missing fields";
                return;
            }
            handleWakuMessage(data[0].toString().toStdString(), data[1].toString().toStdString(), data[2].toString().toStdString());
        })) {
        qWarning() << "ChatWidget: failed to subscribe to chatMessage events";
    }

    if (!logos->chat.on("historyMessage", [this](const QVariantList& data) {
            if (data.size() < 3) {
                qWarning() << "ChatWidget: historyMessage payload missing fields";
                return;
            }
            QString historyPrefix = "[HISTORY] ";
            QString nick = data[1].toString();
            QString message = data[2].toString();

            QMetaObject::invokeMethod(activeWidget, [=]() {
                QString historyPrefix = "[HISTORY] ";
                activeWidget->displayMessage(historyPrefix + nick, message);
            }, Qt::QueuedConnection);
        })) {
        qWarning() << "ChatWidget: failed to subscribe to historyMessage events";
    }

    bool success = logos->chat.initialize();

    if (!success) {
        updateStatus("Error: Failed to initialize Waku");
        return;
    }

    isWakuInitialized = true;
    isWakuRunning = true;
    updateStatus("Status: Waku initialized and running");

    // Enable UI components
    channelInput->setEnabled(true);
    joinButton->setEnabled(true);
    messageInput->setEnabled(true);
    sendButton->setEnabled(true);

    // Set default channel name
    currentChannel = "baixa-chiado"; // Default channel
    channelInput->setText(currentChannel);

    // Join the default channel
    onJoinChannelClicked();
}

void ChatWidget::stopWaku() {
    // Nothing to do here as the plugin handles the cleanup internally
    updateStatus("Status: Stopping Waku...");
    isWakuInitialized = false;
    isWakuRunning = false;
    updateStatus("Status: Waku stopped");
    
    // Disable UI components
    channelInput->setEnabled(false);
    joinButton->setEnabled(false);
    messageInput->setEnabled(false);
    sendButton->setEnabled(false);
}

void ChatWidget::onJoinChannelClicked() {
    QString channelName = channelInput->text().trimmed();
    if (channelName.isEmpty()) {
        QMessageBox::warning(this, "Channel Error", "Please enter a channel name");
        return;
    }
    
    // Clear the chat display when joining a new channel
    chatDisplay->clear();
    
    // Update the channel name
    currentChannel = channelName;
    
    // Check if Waku is running
    if (!isWakuRunning) {
        QMessageBox::warning(this, "Waku Error", "Waku is not running. Please initialize Waku first.");
        return;
    }

    bool success = logos->chat.joinChannel(currentChannel);
    if (success) {
        updateStatus("Joined channel: " + currentChannel);
        QString joinMessage = "You have joined channel: " + currentChannel;
        chatDisplay->append("<i>" + joinMessage + "</i>");

        // Automatically retrieve message history for the joined channel
        updateStatus("Retrieving message history for the channel...");
        chatDisplay->append("<i>--- Message History ---</i>");

        // Call retrieveHistory - history messages will come via historyMessage events
        bool _hist = logos->chat.retrieveHistory(currentChannel);
        qDebug() << "LogosAPI retrieveHistory result:" << _hist;
    } else {
        updateStatus("Failed to join channel: " + currentChannel);
        QMessageBox::warning(this, "Channel Error", "Failed to join channel: " + currentChannel);
    }

    // Clear input field
    channelInput->clear();
    channelInput->setText(currentChannel);
}

void ChatWidget::onSendButtonClicked() {
    QString message = messageInput->text().trimmed();
    if (message.isEmpty()) return;

    // Check if Waku is running
    if (!isWakuRunning) {
        QMessageBox::warning(this, "Waku Error", "Waku is not running. Please initialize Waku first.");
        return;
    }

    if (m_logosAPI && m_logosAPI->getClient("chat")->isConnected()) {
        logos->chat.sendMessage(currentChannel, username, message);
        qDebug() << "LogosAPI sendMessage called";
    } else {
        qDebug() << "LogosAPI not connected";
    }

    // Clear input field
    messageInput->clear();
}

void ChatWidget::updateStatus(const QString& message) {
    statusLabel->setText(message);
    qDebug() << message;
}

void ChatWidget::displayMessage(const QString& sender, const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString formattedMessage = QString("[%1] %2: %3").arg(timestamp, sender, message);
    chatDisplay->append(formattedMessage);
}
