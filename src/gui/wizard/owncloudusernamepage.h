#pragma once

#include <QWidget>

#include "abstractcredswizardpage.h"

namespace OCC {

namespace Ui {
    class OwncloudUserNamePage;
}

class OwncloudUserNamePage : public AbstractWizardPage
{
    Q_OBJECT
public:
    explicit OwncloudUserNamePage(QWidget *parent = nullptr);
    ~OwncloudUserNamePage();

    void cleanupPage() override;
    bool isComplete() const override;
    int nextId() const override;
    bool validatePage() override;
    void initializePage() override;

private:
    Ui::OwncloudUserNamePage *ui;
    bool _validated = false;
};

}
