#pragma once

#include "abstractsetupwizardpage.h"

namespace Ui {
class ServerUrlSetupWizardPage;
}

namespace OCC::Wizard {
class ServerUrlSetupWizardPage : public AbstractSetupWizardPage
{
    Q_OBJECT

public:
    ServerUrlSetupWizardPage(const QUrl &serverUrl);

    QString userProvidedUrl() const;

private:
    ::Ui::ServerUrlSetupWizardPage *_ui;

public:
    ~ServerUrlSetupWizardPage() noexcept override;
};
}
