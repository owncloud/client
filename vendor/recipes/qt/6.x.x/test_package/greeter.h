#include <QDebug>
#include <QObject>
#include <QString>

class Greeter : public QObject
{
    Q_OBJECT
public:
    Greeter(const QString &name, QObject *parent = 0)
        : QObject(parent)
        , mName(name)
    {
    }

public slots:
    void run()
    {
        qDebug() << QString("Hello %1!").arg(mName);

        emit finished();
    }

signals:
    void finished();

private:
    const QString &mName;
};
