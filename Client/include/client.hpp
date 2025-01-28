#pragma once

#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QTcpSocket>
#include <QWidget>

namespace MCherevko {
	class ChatClient : public QWidget {
		Q_OBJECT

		private:
		QWidget* setupLoginPage();
		QWidget* setupChatPage();

		QWidget* loginPage;
		QLineEdit* inputUsername;
		QLineEdit* inputServerIp;
		QLineEdit* inputUserName;
		QPushButton* btnConnect;

		QWidget* chatPage;
		QListWidget* userListWidget;
		QTextEdit* chatHistory;
		QLineEdit* inputMessage;
		QPushButton* btnSendMessage;

		QTcpSocket* networkSocket;
		QString recipient;
		QString userName;

		public:
		ChatClient(QWidget *parent = nullptr);

		private slots:
		void handleServerConnection();
		void updateUserName();
		void onConnectionFailed();
		void onMessageReceived();
		void sendMessageToServer();
		void onUserListItemSelected(QListWidgetItem* selectedItem);

		private:
		void transitionToChatPage();
		void appendMessageToChatHistory(const QString& sender, const QString& message);
	};
}
