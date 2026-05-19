#include "modalwrapperwidget.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtWidgets/qlabel.h>

namespace OCC {

ModalWrapperWidget::ModalWrapperWidget(QWidget *content, QWidget *parent)
    : QWidget{parent}
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    content->setParent(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(content);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ModalWrapperWidget::finished);
    // yes I know there is a default "close" button, but it emits rejected which can have consequences
    // I'm choosing close for the naming here because it doesn't imply any sort of "save" action, which will be important
    // wrt the settings view, which updates values "live" as they are changed, not committed on "ok" or "save"
    QPushButton *okButton = buttons->button(QDialogButtonBox::Ok);
    okButton->setObjectName("closeButton");
    okButton->setText(tr("Close"));
    layout->addWidget(buttons);

    setLayout(layout);
}

}
