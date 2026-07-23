/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "constexpr_list.h"
#include "ocsynclib.h"
#include "utility.h"

#include <QCryptographicHash>
#include <QString>


namespace OCC {
namespace CheckSums {
    OCSYNC_EXPORT Q_NAMESPACE;

    enum class Algorithm {
        NONE,
        SHA3_256 = QCryptographicHash::Sha3_256,
        SHA256 = QCryptographicHash::Sha256,
        SHA1 = QCryptographicHash::Sha1,
        MD5 = QCryptographicHash::Md5,
        ADLER32 = 100,
        DUMMY_FOR_TESTS,
        PARSE_ERROR
    };
    Q_ENUM_NS(Algorithm);

    constexpr std::string_view toString(Algorithm algo)
    {
        switch (algo) {
        case Algorithm::SHA3_256:
            return "SHA3-256";
        case Algorithm::SHA256:
            return "SHA256";
        case Algorithm::SHA1:
            return "SHA1";
        case Algorithm::MD5:
            return "MD5";
        case Algorithm::ADLER32:
            return "ADLER32";
        case Algorithm::DUMMY_FOR_TESTS:
            return "DUMMY_FOR_TESTS";
        case Algorithm::PARSE_ERROR:
            break;
        case Algorithm::NONE:
            // while none is a valid enum value, it is not valid when used to create a checksum string
            Q_UNREACHABLE();
            break;
        }
        return "ERROR";
    }

    inline QString toQString(Algorithm algo)
    {
        const auto n = toString(algo);
        return QString::fromUtf8(n.data(), n.size());
    }

    constexpr auto pair(Algorithm a)
    {
        return std::make_pair(a, toString(a));
    }

    constexpr_list auto All = {
        // Sorted by priority
        pair(Algorithm::SHA3_256),
        pair(Algorithm::SHA256),
        pair(Algorithm::SHA1),
        pair(Algorithm::MD5),
        pair(Algorithm::ADLER32),
        pair(Algorithm::DUMMY_FOR_TESTS)
    };

    constexpr_list auto UnsafeAlgorithms = {
        pair(Algorithm::ADLER32),
        pair(Algorithm::DUMMY_FOR_TESTS)
    };

    constexpr_list auto SafeAlgorithms = {
        pair(Algorithm::SHA3_256),
        pair(Algorithm::SHA256),
        pair(Algorithm::SHA1),
        pair(Algorithm::MD5)
    };

    inline Algorithm fromName(std::string_view s)
    {
        auto it = std::find_if(All.begin(), All.end(), [s](const auto &it) {
            return it.second == s;
        });
        if (it != All.end()) {
            return it->first;
        }
        return Algorithm::PARSE_ERROR;
    }

    OCSYNC_EXPORT Algorithm fromByteArray(const QByteArray &s);
} // namespace CheckSums

template <>
OCSYNC_EXPORT QString Utility::enumToString(CheckSums::Algorithm algo);

} // namespace OCC
