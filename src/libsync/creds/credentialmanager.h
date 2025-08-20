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

    /**
     * @brief isEmpty tests the manager to see if it has any keys
     * @return true if the manager has no keys
     */
    bool isEmpty();

protected:
    QString scopedKey(const QString &key) const;
    QString scope() const;
    QString accountKey() const;

private:
    QSettings &credentialsList() const;
    QStringList knownKeys(const QString &group = {}) const;

    const Account *const _account = nullptr;
    mutable std::unique_ptr<QSettings> _credentialsList;
};

class OWNCLOUDSYNC_EXPORT CredentialJob : public QObject
{
    Q_OBJECT

public:
    CredentialJob(CredentialManager *parent, const QString &scopedKey);
    void start();

    QKeychain::Error error() const;
    // this is questionable. variants are implicitly shared so I don't see any value returning a ref
    const QVariant &data() const;
    QString errorString() const;

Q_SIGNALS:
    void finished();

private:

    QString _scopedKey;
    QVariant _data;
    QKeychain::Error _error = QKeychain::NoError;
    QString _errorString;
    QKeychain::ReadPasswordJob *_job;
    // mac and linux only!
    bool _retryOnKeyChainError = true;

    // this is "required" to allow the test to get the actual underlying job which is passed to setFallbackEnabled, which is only implemented
    // in TestCredentialManager. I do not believe tests should ever implement extra functionality that does not exist in the actual runtime,
    // they should only rely on REAL functionality.
    // this absolutely breaks that rule and has to be fixed.
    friend class TestCredentialManager;
};


}
