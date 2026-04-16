//
// Created by deepdiver on 27/08/2025.
//

#include "oauthhtmlpage.h"
#include "resources/template.h"

#include <QMap>

namespace OCC {

QString OAuthHtmlPage::buildPage(bool success, const QString &title, const QString &message)
{
    auto templateFile = success ? QStringLiteral(":/client/resources/oauth/success.html.in") : QStringLiteral(":/client/resources/oauth/error.html.in");
    QMap<QString, QString> values = {
        {QStringLiteral("TITLE"), title},
        {QStringLiteral("MESSAGE"), message},
    };
    return Resources::Template::renderTemplateFromFile(templateFile, values);
}

} // OCC