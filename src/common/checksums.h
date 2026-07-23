/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "checksumalgorithms.h"
#include "ocsynclib.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QFutureWatcher>
#include <QObject>

#include <memory>

class QFile;

namespace OCC {

/**
 * Tags for checksum headers values.
 * They are here for being shared between Upload- and Download Job
 */

class SyncJournalDb;

/**
 * Returns the highest-quality checksum in a 'checksums'
 * property retrieved from the server.
 *
 * Example: "ADLER32:1231 SHA1:ab124124 MD5:2131affa21"
 *       -> "SHA1:ab124124"
 */
OCSYNC_EXPORT QByteArray findBestChecksum(const QByteArray &checksums);

class OCSYNC_EXPORT ChecksumHeader
{
public:
    static ChecksumHeader parseChecksumHeader(const QByteArray &header);

    ChecksumHeader() = default;
    ChecksumHeader(CheckSums::Algorithm type, const QByteArray &checksum);

    QByteArray makeChecksumHeader() const;


    CheckSums::Algorithm type() const;

    QByteArray checksum() const;

    bool isValid() const;

    QString error() const;

    bool operator==(const ChecksumHeader &other) const;

    bool operator!=(const ChecksumHeader &other) const;

private:
    CheckSums::Algorithm _checksumType = CheckSums::Algorithm::NONE;
    QByteArray _checksum;
    QString _error;
};

/// Checks OWNCLOUD_DISABLE_CHECKSUM_UPLOAD
OCSYNC_EXPORT bool uploadChecksumEnabled();

/**
 * Computes the checksum of a file.
 * \ingroup libsync
 */
class OCSYNC_EXPORT ComputeChecksum : public QObject
{
    Q_OBJECT
public:
    explicit ComputeChecksum(QObject *parent = nullptr);
    ~ComputeChecksum() override;

    /**
     * Sets the checksum type to be used.
     */
    void setChecksumType(CheckSums::Algorithm type);

    CheckSums::Algorithm checksumType() const;

    /**
     * Computes the checksum for the given file path.
     *
     * done() is emitted when the calculation finishes.
     */
    void start(const QString &filePath);

    /**
     * Computes the checksum for the given device.
     *
     * done() is emitted when the calculation finishes.
     *
     * The device ownership transfers into the thread that
     * will compute the checksum. It must not have a parent.
     */
    void start(std::unique_ptr<QIODevice> device);

    /**
     * Computes the checksum synchronously.
     */
    static QByteArray computeNow(QIODevice *device, CheckSums::Algorithm algo);

    /**
     * Computes the checksum synchronously on file. Convenience wrapper for computeNow().
     */
    static QByteArray computeNowOnFile(const QString &filePath, CheckSums::Algorithm checksumType);

Q_SIGNALS:
    void done(CheckSums::Algorithm checksumType, const QByteArray &checksum);

private Q_SLOTS:
    void slotCalculationDone();

private:
    void startImpl(std::unique_ptr<QIODevice> device);

    CheckSums::Algorithm _checksumType;

    // watcher for the checksum calculation thread
    QFutureWatcher<QByteArray> _watcher;
};

/**
 * Checks whether a file's checksum matches the expected value.
 * @ingroup libsync
 */
class OCSYNC_EXPORT ValidateChecksumHeader : public QObject
{
    Q_OBJECT
public:
    explicit ValidateChecksumHeader(QObject *parent = nullptr);

    /**
     * Check a file's actual checksum against the provided checksumHeader
     *
     * If no checksum is there, or if a correct checksum is there, the signal validated()
     * will be emitted. In case of any kind of error, the signal validationFailed() will
     * be emitted.
     */
    void start(const QString &filePath, const QByteArray &checksumHeader);

    /**
     * Check a device's actual checksum against the provided checksumHeader
     *
     * Like the other start() but works on an device.
     *
     * The device ownership transfers into the thread that
     * will compute the checksum. It must not have a parent.
     */
    void start(std::unique_ptr<QIODevice> device, const QByteArray &checksumHeader);

Q_SIGNALS:
    void validated(CheckSums::Algorithm checksumType, const QByteArray &checksum);
    void validationFailed(const QString &errMsg);

private Q_SLOTS:
    void slotChecksumCalculated(CheckSums::Algorithm checksumType, const QByteArray &checksum);

private:
    ComputeChecksum *prepareStart(const QByteArray &checksumHeader);

    ChecksumHeader _expectedChecksum;
};
}
