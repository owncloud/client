target_sources(owncloudGui PRIVATE
    enums.cpp

    pages/abstractsetupwizardpage.h
    pages/abstractsetupwizardpage.cpp

    pages/serverurlsetupwizardpage.ui
    pages/serverurlsetupwizardpage.h
    pages/serverurlsetupwizardpage.cpp

    pages/legacywebfingersetupwizardpage.ui
    pages/legacywebfingersetupwizardpage.h
    pages/legacywebfingersetupwizardpage.cpp

    pages/credentialssetupwizardpage.ui

    pages/basiccredentialssetupwizardpage.h
    pages/basiccredentialssetupwizardpage.cpp

    pages/oauthcredentialssetupwizardpage.h
    pages/oauthcredentialssetupwizardpage.cpp

    pages/accountconfiguredwizardpage.ui
    pages/accountconfiguredwizardpage.h
    pages/accountconfiguredwizardpage.cpp

    setupwizardwidget.ui
    setupwizardwidget.h
    setupwizardwidget.cpp

    setupwizardcontroller.h
    setupwizardcontroller.cpp

    setupwizardaccountbuilder.cpp
    setupwizardaccountbuilder.h

    navigation.cpp
    navigation.h

    jobs/resolveurljobfactory.cpp
    jobs/resolveurljobfactory.h

    jobs/discoverwebfingerservicejobfactory.cpp
    jobs/discoverwebfingerservicejobfactory.h

    jobs/webfingeruserinfojobfactory.cpp
    jobs/webfingeruserinfojobfactory.h

    postfixlineedit.cpp

    states/abstractsetupwizardstate.cpp
    states/serverurlsetupwizardstate.cpp
    states/legacywebfingersetupwizardstate.cpp
    states/basiccredentialssetupwizardstate.cpp
    states/oauthcredentialssetupwizardstate.cpp
    states/accountconfiguredsetupwizardstate.cpp

    setupwizardcontext.cpp
)
