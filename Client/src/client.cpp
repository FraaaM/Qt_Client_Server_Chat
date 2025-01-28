#include <QHostAddress>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "client.hpp"

namespace MCherevko {
	ChatClient::ChatClient(QWidget *parent) : QWidget(parent), recipient("Everyone") {
		loginPage = setupLoginPage();
		chatPage = setupChatPage();
		chatPage->hide();

		loginPage->setMinimumSize(300, 200);
		chatPage->setMinimumSize(450, 300);
		setWindowTitle("User Login");

		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		mainLayout->addWidget(loginPage);
		mainLayout->addWidget(chatPage);

		networkSocket = new QTcpSocket(this);

		connect(networkSocket, &QTcpSocket::readyRead, this, &ChatClient::onMessageReceived);
		connect(networkSocket, &QTcpSocket::connected, this, &ChatClient::transitionToChatPage);
	}

	QWidget* ChatClient::setupLoginPage() {
		QWidget* page = new QWidget(this);

		QLabel* labelUsername = new QLabel("Enter Username:");
		inputUsername = new QLineEdit();
		inputUsername->setMaxLength(15);

		QLabel* labelServerIp = new QLabel("Server Address:");
		inputServerIp = new QLineEdit();
		inputServerIp->setText("127.0.0.1");

		btnConnect = new QPushButton("Connect");
		connect(btnConnect, &QPushButton::clicked, this, &ChatClient::handleServerConnection);

		QVBoxLayout* layout = new QVBoxLayout(page);
		layout->addWidget(labelUsername);
		layout->addWidget(inputUsername);
		layout->addWidget(labelServerIp);
		layout->addWidget(inputServerIp);
		layout->addWidget(btnConnect);

		connect(inputUsername, &QLineEdit::returnPressed, btnConnect, &QPushButton::click);
		connect(inputServerIp, &QLineEdit::returnPressed, btnConnect, &QPushButton::click);

		return page;
	}

	QWidget* ChatClient::setupChatPage() {
		QWidget* page = new QWidget(this);

		QLabel* labelUserName = new QLabel("Your Name:");
		inputUserName = new QLineEdit();
		inputUserName->setMaxLength(15);

		QPushButton* btnChangeName = new QPushButton("Change");
		connect(btnChangeName, &QPushButton::clicked, this, &ChatClient::updateUserName);
		connect(inputUserName, &QLineEdit::returnPressed, btnChangeName, &QPushButton::click);

		QHBoxLayout* nameLayout = new QHBoxLayout();
		nameLayout->addWidget(labelUserName);
		nameLayout->addWidget(inputUserName);
		nameLayout->addWidget(btnChangeName);

		userListWidget = new QListWidget();
		userListWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		userListWidget->setMaximumWidth(100);

		chatHistory = new QTextEdit();
		chatHistory->setReadOnly(true);
		inputMessage = new QLineEdit();
		btnSendMessage = new QPushButton("Send");

		connect(btnSendMessage, &QPushButton::clicked, this, &ChatClient::sendMessageToServer);
		connect(userListWidget, &QListWidget::itemClicked, this, &ChatClient::onUserListItemSelected);

		QHBoxLayout* mainLayout = new QHBoxLayout(page);
		QVBoxLayout* chatLayout = new QVBoxLayout();

		chatLayout->addLayout(nameLayout);
		chatLayout->addWidget(chatHistory);
		chatLayout->addWidget(inputMessage);
		chatLayout->addWidget(btnSendMessage);

		mainLayout->addWidget(userListWidget);
		mainLayout->addLayout(chatLayout, 3);

		connect(inputMessage, &QLineEdit::returnPressed, btnSendMessage, &QPushButton::click);

		return page;
	}

	void ChatClient::handleServerConnection() {
		QString serverIp = inputServerIp->text();
		userName = inputUsername->text();

		if (serverIp.isEmpty() || userName.isEmpty()) {
			QMessageBox::warning(this, "Connection Error", "Please provide both server IP and username.");
			return;
		}

		networkSocket->connectToHost(QHostAddress(serverIp), 1234);

		if (!networkSocket->waitForConnected(3000)) {
			QMessageBox::critical(this, "Connection Error", "Unable to connect to the server: " + networkSocket->errorString());
			return;
		}
	}

	void ChatClient::updateUserName() {
		QString newUserName = inputUserName->text();
		if (newUserName.isEmpty()) {
			QMessageBox::warning(this, "Invalid Name", "Name cannot be empty.");
			return;
		}
		if (newUserName == userName) {
			QMessageBox::warning(this, "Name Error", "You're already using this name.");
			return;
		}
		if (newUserName != userName) {
			userName = newUserName;
			networkSocket->write(QString("CHANGE_NAME:%1\n").arg(userName).toUtf8());
			return;
		}
	}

	void ChatClient::onConnectionFailed() {
		QMessageBox::critical(this, "Connection Error", "Unable to connect to the server. Check the IP address.");
	}

	void ChatClient::onMessageReceived() {
		while (networkSocket->canReadLine()) {
			setWindowTitle("Chat Room");
			QString receivedData = networkSocket->readLine().trimmed();

			if (receivedData.startsWith("USERS:")) {
				userListWidget->clear();
				QStringList users = receivedData.mid(6).split(',');
				userListWidget->addItem("Everyone");
				userListWidget->addItems(users);
			}
			else if (receivedData.startsWith("MSG:")) {
				QStringList messageParts = receivedData.mid(4).split(':');
				if (messageParts.size() >= 3) {
					QString sender = messageParts[0];
					QString recipient = messageParts[1];
					QString messageContent = messageParts.mid(2).join(':');

					QString formattedMessage;
					if (recipient == "ALL") {
						formattedMessage = QString("%1 -> everyone: %2").arg(sender, messageContent);
					} else {
						formattedMessage = QString("%1 -> %2: %3").arg(sender, recipient, messageContent);
					}

					chatHistory->append(formattedMessage);
				}
			}
			else if (receivedData.startsWith("SERVER:Username Taken")) {
				QString newUserName = receivedData.mid(21);
				userName = newUserName;
				inputUsername->setText(newUserName);
				QMessageBox::information(this, "Username Taken", QString("Your username was already taken. New username: %1").arg(newUserName));
			}
			else if (receivedData.startsWith("SERVER:Username Already Exists")) {
				QMessageBox::warning(this, "Username Error", "This username is already in use. Please choose another.");
				inputUserName->clear();
			}
		}
	}

	void ChatClient::sendMessageToServer() {
		QString messageContent = inputMessage->text();
		if (messageContent.isEmpty()) return;

		QString messageToSend;
		if (recipient == "Everyone") {
			messageToSend = QString("MSG:ALL:%1").arg(messageContent);
		} else {
			messageToSend = QString("MSG:%1:%2").arg(recipient).arg(messageContent);
		}

		networkSocket->write(messageToSend.toUtf8() + "\n");
		inputMessage->clear();
	}

	void ChatClient::onUserListItemSelected(QListWidgetItem* selectedItem) {
		recipient = selectedItem->text();
	}

	void ChatClient::transitionToChatPage() {
		loginPage->hide();
		chatPage->show();

		networkSocket->write(QString("CONNECT:%1\n").arg(userName).toUtf8());
	}

	void ChatClient::appendMessageToChatHistory(const QString& sender, const QString& message) {
		chatHistory->append(QString("%1: %2").arg(sender, message));
	}
}
