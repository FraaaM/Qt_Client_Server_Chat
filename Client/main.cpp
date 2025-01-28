#include <QApplication>

#include "client.hpp"

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	MCherevko::ChatClient client;
	client.show();

	return app.exec();
}
