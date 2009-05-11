#include <qapplication.h>
#include "SCTPClientSocket.h"

int main(int argc,char **argv)
{
	QApplication app(argc,argv,false);

	SCTPClientSocket *client = new SCTPClientSocket();

	QObject::connect(client,SIGNAL(serverclosed()),qApp,SLOT(quit()));

	client->connectToHost("127.0.0.1",2000);

	client->sendMsg(QString("Hello World!!"));

	return app.exec();
}
