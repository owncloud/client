#include "greeter.h"
#include <QCoreApplication>
#include <QFile>
#include <QObject>
#include <QString>
#include <QTimer>

#include <QDomText>
#include <QNetworkAccessManager>
#include <QSqlDatabase>
#include <QtConcurrent>

#include <qplatformdefs.h>

void f()
{
    qDebug() << "inside f";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("Application Example");
    QCoreApplication::setApplicationVersion("1.0.0");

    QString name = argc > 0 ? argv[1] : "";
    if (name.isEmpty()) {
        name = "World";
    }

    Greeter *greeter = new Greeter(name, &app);
    QObject::connect(greeter, SIGNAL(finished()), &app, SLOT(quit()));
    QTimer::singleShot(0, greeter, SLOT(run()));

    QFile f(":/resource.txt");
    if (!f.open(QIODevice::ReadOnly))
        qFatal("Could not open resource file");
    qDebug() << "Resource content:" << f.readAll();
    f.close();

    qDebug() << W_OK;

    QNetworkAccessManager networkTester;

    QSqlDatabase sqlTester;

    QFuture<void> future = QtConcurrent::run(::f);
    future.waitForFinished();

    QDomText xmlTester;

    return app.exec();
}
