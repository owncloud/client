/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "ocsynclib.h"
#include "ownsql.h"
#include "common/asserts.h"

namespace OCC {

class OCSYNC_EXPORT PreparedSqlQuery
{
public:
    ~PreparedSqlQuery();

    operator bool() const { return _ok; }

    SqlQuery *operator->() const
    {
        Q_ASSERT(_ok);
        return _query;
    }

    SqlQuery &operator*() const &
    {
        Q_ASSERT(_ok);
        return *_query;
    }

private:
    PreparedSqlQuery(SqlQuery *query, bool ok = true);

    SqlQuery *_query;
    bool _ok;

    friend class PreparedSqlQueryManager;
};

/**
 * @brief Manage PreparedSqlQuery
 */
class OCSYNC_EXPORT PreparedSqlQueryManager
{
public:
    enum Key {
        GetFileRecordQuery,
        GetFileRecordQueryByInode,
        GetFileRecordQueryByFileId,
        GetFilesBelowPathQuery,
        GetAllFilesQuery,
        ListFilesInPathQuery,
        SetFileRecordQuery,
        SetFileRecordChecksumQuery,
        GetDownloadInfoQuery,
        SetDownloadInfoQuery,
        DeleteDownloadInfoQuery,
        GetUploadInfoQuery,
        GetAllUploadInfoQuery,
        SetUploadInfoQuery,
        DeleteUploadInfoQuery,
        DeleteFileRecordPhash,
        DeleteFileRecordRecursively,
        GetErrorBlacklistQuery,
        SetErrorBlacklistQuery,
        GetSelectiveSyncListQuery,
        GetChecksumTypeIdQuery,
        GetChecksumTypeQuery,
        InsertChecksumTypeQuery,
        GetDataFingerprintQuery,
        SetDataFingerprintQuery1,
        SetDataFingerprintQuery2,
        GetConflictRecordQuery,
        SetConflictRecordQuery,
        DeleteConflictRecordQuery,
        GetRawPinStateQuery,
        GetEffectivePinStateQuery,
        GetSubPinsQuery,
        CountDehydratedFilesQuery,
        SetPinStateQuery,
        WipePinStateQuery,

        GetFileReocrdsWithDirtyPlaceholdersQuery,

        PreparedQueryCount
    };
    PreparedSqlQueryManager() = default;
    /**
     * The queries are reset in the destructor to prevent wal locks
     */
    const PreparedSqlQuery get(Key key);
    /**
     * Prepare the SqlQuery if it was not prepared yet.
     */
    const PreparedSqlQuery get(Key key, const QByteArray &sql, SqlDatabase &db);

private:
    SqlQuery _queries[PreparedQueryCount];
    Q_DISABLE_COPY(PreparedSqlQueryManager)
};

}
