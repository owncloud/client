#ifndef SHAREEMAILDIALOG_H
#define SHAREEMAILDIALOG_H

#include "accountfwd.h"
#include "QProgressIndicator.h"
#include "share.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSharedPointer>
#include <QString>
#include <QWidget>

namespace OCC {

class ShareManager;

class ShareEmailWidget : public QWidget
{
public:

    explicit ShareEmailWidget(AccountPtr account,
                              const QString &sharePath,
                              QWidget *parent = 0);
    ~ShareEmailWidget();

private slots:
    void slotLinkShareCreated(const QSharedPointer<LinkShare> share);
    void slotPasswordEnterd();
    void slotPasswordRequired();

private:
    void share(const QString &sharePath, const QString &password);

    AccountPtr _account;
    QString _sharePath;

    QHBoxLayout _layout;
    QLabel _label;
    QProgressIndicator _spinner;

    ShareManager *_manager;
};

}
#endif // SHAREEMAILDIALOG_H
