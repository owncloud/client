#pragma once

#include <QWidget>

#include "abstractcredswizardpage.h"

namespace OCC {
class WebFinger;
namespace Ui {
    class OwncloudUserNamePage;
}

class OwncloudUserNamePage : public AbstractWizardPage
{
    Q_OBJECT
public:
    explicit OwncloudUserNamePage(OwncloudWizard *parent);
    ~OwncloudUserNamePage();

    bool isComplete() const override;
    bool validatePage() override;
    void initializePage() override;

private:
    void stopWebFinger();
    Ui::OwncloudUserNamePage *ui;
    bool _validated = false;
    WebFinger *_webFingerJob = nullptr;
};

}
