#include "server.hpp"
#include <QTextStream>
#include <QHostAddress>
#include <QDebug>
#include <random>

namespace MCherevko{

	ChatServer::ChatServer(QObject* parent)
		: QTcpServer(parent) {}

	bool ChatServer::initializeServer(quint16 port) {
		return listen(QHostAddress::Any, port);
	}

	void ChatServer::incomingConnection(qintptr socketDescriptor) {
		auto* newClient = new QTcpSocket(this);
		newClient->setSocketDescriptor(socketDescriptor);

		connect(newClient, &QTcpSocket::readyRead, this, &ChatServer::handleReadyRead);
		connect(newClient, &QTcpSocket::disconnected, this, &ChatServer::handleClientDisconnected);

		connectedClients.append(newClient);
		clientUsernames.append("");
	}

	void ChatServer::handleReadyRead() {
		auto* client = qobject_cast<QTcpSocket*>(sender());
		if (!client) return;

		while (client->canReadLine()) {
			QString message = QString::fromUtf8(client->readLine()).trimmed();

			if (message.startsWith("CONNECT:")) {
				QString username = message.mid(8);

				if (clientUsernames.contains(username)) {
					QString newUsername = createUniqueUsername();
					int clientIndex = connectedClients.indexOf(client);
					clientUsernames[clientIndex] = newUsername;
					transmitToClient(client, QString("SERVER:Username taken. New username: %1").arg(newUsername));
				} else {
					int clientIndex = connectedClients.indexOf(client);
					clientUsernames[clientIndex] = username;
					qDebug() << "New user connected: " << username;
				}

				updateUserList();
			} else if (message.startsWith("CHANGE_NAME:")) {
				QString newUsername = message.mid(12);

				if (clientUsernames.contains(newUsername)) {
					transmitToClient(client, "SERVER:Username already in use.");
					return;
				}

				int clientIndex = connectedClients.indexOf(client);
				QString oldUsername = clientUsernames[clientIndex];
				clientUsernames[clientIndex] = newUsername;

				qDebug() << "Username updated: " << oldUsername << " -> " << newUsername;
				sendGlobalMessage("SERVER", QString("%1 is now %2").arg(oldUsername, newUsername));
				updateUserList();
			} else if (message.startsWith("MSG:ALL:")) {
				QString chatMessage = message.mid(8);
				QString senderName = clientUsernames[connectedClients.indexOf(client)];
				sendGlobalMessage(senderName, chatMessage);
			} else if (message.startsWith("MSG:")) {
				QStringList components = message.mid(4).split(':');
				if (components.size() >= 2) {
					QString recipient = components[0];
					QString privateMessage = components[1];
					sendPrivateMessage(client, recipient, privateMessage);
				}
			}
		}
	}

	void ChatServer::handleClientDisconnected() {
		auto* client = qobject_cast<QTcpSocket*>(sender());
		if (!client) return;

		int clientIndex = connectedClients.indexOf(client);
		QString disconnectedUsername = clientUsernames[clientIndex];
		clientUsernames.removeAt(clientIndex);
		connectedClients.removeAt(clientIndex);
		client->deleteLater();

		qDebug() << "User disconnected: " << disconnectedUsername;
		updateUserList();
	}

	QString ChatServer::createUniqueUsername() {
		static const QString characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
		QString username;

		std::random_device rd; // Источник случайности
		std::mt19937 gen(rd()); // Генератор на основе Mersenne Twister
		std::uniform_int_distribution<> dist(0, characters.size() - 1);
		for (int i = 0; i < 10; ++i) {
			username.append(characters.at(dist(gen)));
		}
		return username;
	}


	void ChatServer::sendGlobalMessage(const QString& sender, const QString& message) {
		QString formattedMessage = QString("MSG:%1:ALL:%2\n").arg(sender, message);
		for (QTcpSocket* client : connectedClients) {
			transmitToClient(client, formattedMessage);
		}
	}

	void ChatServer::updateUserList() {
		QString userList = clientUsernames.join(',');
		QString userListMessage = QString("USERS:%1\n").arg(userList);
		for (QTcpSocket* client : connectedClients) {
			transmitToClient(client, userListMessage);
		}
	}

	void ChatServer::sendPrivateMessage(QTcpSocket* sender, const QString& recipient, const QString& message) {
		QString senderName = clientUsernames[connectedClients.indexOf(sender)];
		QString formattedMessage = QString("MSG:%1:%2:%3\n").arg(senderName, recipient, message);

		bool recipientFound = false;

		for (QTcpSocket* client : connectedClients) {
			int clientIndex = connectedClients.indexOf(client);
			if (clientUsernames[clientIndex] == recipient) {
				transmitToClient(client, formattedMessage);
				recipientFound = true;
			}
		}

		if (recipient != senderName) {
			transmitToClient(sender, formattedMessage);
		}

		if (!recipientFound) {
			transmitToClient(sender, "SERVER:Recipient not found.");
		}
	}


	void ChatServer::transmitToClient(QTcpSocket* client, const QString& message) {
		if (client && client->isOpen()) {
			client->write(message.toUtf8() + "\n");
		}
	}

}
