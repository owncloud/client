#include "shareemailwidget.h"
#include "share.h"

#include <QDesktopServices>
#include <QSharedPointer>
#include <QUrl>

namespace OCC {

ShareEmailWidget::ShareEmailWidget(AccountPtr account,
                                   const QString &sharePath,
                                   QWidget *parent)
    : QWidget(parent),
     _account(account),
     _sharePath(sharePath)
{
    resize(200,200);

    _layout.addWidget(&_spinner);
    _layout.addWidget(&_label);

    _label.setText(tr("Fetching share link"));
    _spinner.startAnimation();

    setLayout(&_layout);

    /*
     * Create the share manager and connect it properly
     */
    _manager = new ShareManager(_account, this);
    connect(_manager, SIGNAL(linkShareCreated(QSharedPointer<LinkShare>)), this, SLOT(slotLinkShareCreated(QSharedPointer<LinkShare>)));

    _manager->createLinkShare(_sharePath);
}

ShareEmailWidget::~ShareEmailWidget()
{

}

void ShareEmailWidget::slotLinkShareCreated(const QSharedPointer<LinkShare> share)
{
    QUrl mailto("mailto:");

    QString body = tr("I shared a file with you at %1.").arg(share->getLink().toString());

    if (share->getExpireDate().isValid()) {
        body += tr("\nThis share expires at %1.").arg(share->getExpireDate().toString());
    }

    mailto.setQuery("body?"+body);

    // Open app
    QDesktopServices::openUrl(mailto);

    close();
}

}
