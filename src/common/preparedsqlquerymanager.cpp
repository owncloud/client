/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "preparedsqlquerymanager.h"

#include <sqlite3.h>

using namespace OCC;

PreparedSqlQuery::PreparedSqlQuery(SqlQuery *query, bool ok)
    : _query(query)
    , _ok(ok)
{
}

PreparedSqlQuery::~PreparedSqlQuery()
{
    _query->reset_and_clear_bindings();
}

const PreparedSqlQuery PreparedSqlQueryManager::get(PreparedSqlQueryManager::Key key)
{
    auto &query = _queries[key];
    OC_ENFORCE(query._stmt);
    Q_ASSERT(!sqlite3_stmt_busy(query._stmt));
    return { &query };
}

const PreparedSqlQuery PreparedSqlQueryManager::get(PreparedSqlQueryManager::Key key, const QByteArray &sql, SqlDatabase &db)
{
    auto &query = _queries[key];
    Q_ASSERT(!sqlite3_stmt_busy(query._stmt));
    OC_ENFORCE(!query._sqldb || &db == query._sqldb);
    if (!query._stmt) {
        query._sqldb = &db;
        query._db = db.sqliteDb();
        return { &query, query.prepare(sql) == 0 };
    }
    return { &query };
}
