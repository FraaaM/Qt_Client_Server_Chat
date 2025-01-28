#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QVector>

namespace MCherevko {

	class ChatServer : public QTcpServer {
		Q_OBJECT

		private:
		QVector<QTcpSocket*> connectedClients;
		QVector<QString> clientUsernames;

		public:
		explicit ChatServer(QObject* parent = nullptr);
		bool initializeServer(quint16 port);

		protected:
		void incomingConnection(qintptr socketDescriptor) override;

		private slots:
		void handleReadyRead();
		void handleClientDisconnected();

		private:
		QString createUniqueUsername();
		void sendGlobalMessage(const QString& sender, const QString& message);
		void updateUserList();
		void sendPrivateMessage(QTcpSocket* sender, const QString& recipient, const QString& message);
		void transmitToClient(QTcpSocket* client, const QString& message);
	};

}

