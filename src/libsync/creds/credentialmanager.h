#pragma once

#include <QSettings>
#include <QVariant>

#include "owncloudlib.h"

#include <qt6keychain/keychain.h>

#include <memory>

namespace OCC {
class Account;
class CredentialJob;

class OWNCLOUDSYNC_EXPORT CredentialManager : public QObject
{
    Q_OBJECT
public:
    // generic credentials - this is currently needed by the proxy network settings
    CredentialManager(QObject *parent);
    // account related credentials
    explicit CredentialManager(Account *acc);

    CredentialJob *get(const QString &key);
    QKeychain::Job *set(const QString &key, const QVariant &data);
    QKeychain::Job *remove(const QString &key);
    /**
     * Delete all credentials asigned with an account
     */
    QVector<QPointer<QKeychain::Job>> clear(const QString &group = {});

    bool contains(const QString &key) const;

protected:
    QString scopedKey(const QString &key) const;
    QString scope() const;
    QString accountKey() const;

private:
    QSettings &credentialsList() const;

    // TestCredentialManager
    QStringList knownKeys(const QString &group = {}) const;

    const Account *const _account = nullptr;
    mutable std::unique_ptr<QSettings> _credentialsList;

    friend class TestCredentialManager;
};

class OWNCLOUDSYNC_EXPORT CredentialJob : public QObject
{
    Q_OBJECT
public:
    QKeychain::Error error() const;
    // this is questionable. variants are implicitly shared so I don't see any value returning a ref
    const QVariant &data() const;
    QString errorString() const;

Q_SIGNALS:
    void finished();

private:
    // I cannot understand why these are private. review this with cohort
    CredentialJob(CredentialManager *parent, const QString &scopedKey);
    void start();

    QString _scopedKey;
    QVariant _data;
    QKeychain::Error _error = QKeychain::NoError;
    QString _errorString;
    QKeychain::ReadPasswordJob *_job;

    friend class CredentialManager;
    friend class TestCredentialManager;
};


}
