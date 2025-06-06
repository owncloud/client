#pragma once

#include <QObject>

class NewAccountWizardController : public QObject
{
    Q_OBJECT
public:
    explicit NewAccountWizardController(QObject *parent = nullptr);
};
