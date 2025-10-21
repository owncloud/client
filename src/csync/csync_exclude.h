/*
 * libcsync -- a library to sync a directory with another
 *
 * Copyright (c) 2008-2013 by Andreas Schneider <asn@cryptomilk.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _CSYNC_EXCLUDE_H
#define _CSYNC_EXCLUDE_H

#include "ocsynclib.h"

#include "csync.h"

#include <QObject>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QVersionNumber>

#include <functional>

enum CSYNC_EXCLUDE_TYPE {
    CSYNC_NOT_EXCLUDED = 0,
    CSYNC_FILE_SILENTLY_EXCLUDED,
    CSYNC_FILE_EXCLUDE_AND_REMOVE, // TODO: is there still a difference to CSYNC_FILE_SILENTLY_EXCLUDED
    CSYNC_FILE_EXCLUDE_LIST,
    CSYNC_FILE_EXCLUDE_INVALID_CHAR,
    CSYNC_FILE_EXCLUDE_TRAILING_SPACE,
    CSYNC_FILE_EXCLUDE_LONG_FILENAME,
    CSYNC_FILE_EXCLUDE_HIDDEN,
    CSYNC_FILE_EXCLUDE_STAT_FAILED,
    CSYNC_FILE_EXCLUDE_CONFLICT,
    CSYNC_FILE_EXCLUDE_CANNOT_ENCODE,
    CSYNC_FILE_EXCLUDE_SERVER_BLACKLISTED,
    CSYNC_FILE_EXCLUDE_RESERVED,
};

/**
 * Manages file/directory exclusion.
 *
 * Most commonly exclude patterns are loaded from files. See
 * addExcludeFilePath() and reloadExcludeFiles().
 *
 * Excluded files are primarily relevant for sync runs, and for
 * file watcher filtering.
 *
 * Excluded files and ignored files are the same thing. But the
 * selective sync blacklist functionality is a different thing
 * entirely.
 */
class OCSYNC_EXPORT ExcludedFiles : public QObject
{
    Q_OBJECT
public:
    ExcludedFiles();
    ~ExcludedFiles() override;

    /**
     * Adds a new path to a file containing exclude patterns.
     *
     * Does not load the file. Use reloadExcludeFiles() afterwards.
     */
    void addExcludeFilePath(const QString &path);

    /**
     * Checks whether a file or directory should be excluded.
     *
     * @param filePath     the absolute path to the file
     * @param basePath     folder path from which to apply exclude rules, ends with a /
     */
    bool isExcluded(QStringView filePath, QStringView basePath, bool excludeHidden) const;

    /**
     * Checks whether a remote file or directory should be excluded.
     *
     * @param filePath     the absolute path to the file
     * @param basePath     folder path from which to apply exclude rules, ends with a /
     */
    bool isExcludedRemote(QStringView filePath, QStringView basePath, bool excludeHidden, ItemType type) const;

    /**
     * Adds an exclude pattern.
     *
     * Primarily used in tests. Patterns added this way are preserved when
     * reloadExcludeFiles() is called.
     */
    void addManualExclude(const QString &expr);

    /**
     * Removes all manually added exclude patterns.
     *
     * Primarily used in tests.
     */
    void clearManualExcludes();

    /**
     * Adjusts behavior of wildcards. Only used for testing.
     */
    void setWildcardsMatchSlash(bool onoff);

    /**
     * Sets the client version, only used for testing.
     */
    void setClientVersion(const QVersionNumber &version);

    /**
     * @brief Check if the given path should be excluded in a traversal situation.
     *
     * It does only part of the work that full() does because it's assumed
     * that all leading directories have been run through traversal()
     * before. This can be significantly faster.
     *
     * That means for 'foo/bar/file' only ('foo/bar/file', 'file') is checked
     * against the exclude patterns.
     *
     * @param Path is folder-relative, should not start with a /.
     *
     * Note that this only matches patterns. It does not check whether the file
     * or directory pointed to is hidden (or whether it even exists).
     */
    CSYNC_EXCLUDE_TYPE traversalPatternMatch(QStringView path, ItemType filetype) const;

public Q_SLOTS:
    /**
     * Reloads the exclude patterns from the registered paths.
     */
    bool reloadExcludeFiles();

private:
    /**
     * Returns true if the version directive indicates the next line
     * should be skipped.
     *
     * A version directive has the form "#!version <op> <version>"
     * where <op> can be <, <=, ==, >, >= and <version> can be any version
     * like 2.5.0.
     *
     * Example:
     *
     * #!version < 2.5.0
     * myexclude
     *
     * Would enable the "myexclude" pattern only for versions before 2.5.0.
     */
    bool versionDirectiveKeepNextLine(const QByteArray &directive) const;

    /**
     * @brief Match the exclude pattern against the full path.
     *
     * @param Path is folder-relative, should not start with a /.
     *
     * Note that this only matches patterns. It does not check whether the file
     * or directory pointed to is hidden (or whether it even exists).
     */
    CSYNC_EXCLUDE_TYPE fullPatternMatch(QStringView path, ItemType filetype) const;

    /**
     * Generate optimized regular expressions for the exclude patterns.
     *
     * The optimization works in two steps: First, all supported patterns are put
     * into _fullRegexFile/_fullRegexDir. These regexes can be applied to the full
     * path to determine whether it is excluded or not.
     *
     * The second is a performance optimization. The particularly common use
     * case for excludes during a sync run is "traversal": Instead of checking
     * the full path every time, we check each parent path with the traversal
     * function incrementally.
     *
     * Example: When the sync run eventually arrives at "a/b/c it can assume
     * that the traversal matching has already been run on "a", "a/b"
     * and just needs to run the traversal matcher on "a/b/c".
     *
     * The full matcher is equivalent to or-combining the traversal match results
     * of all parent paths:
     *   full("a/b/c/d") == traversal("a") || traversal("a/b") || traversal("a/b/c")
     *
     * The traversal matcher can be extremely fast because it has a fast early-out
     * case: It checks the bname part of the path against _bnameTraversalRegex
     * and only runs a simplified _fullTraversalRegex on the whole path if bname
     * activation for it was triggered.
     *
     * Note: The traversal matcher will return not-excluded on some paths that the
     * full matcher would exclude. Example: "b" is excluded. traversal("b/c")
     * returns not-excluded because "c" isn't a bname activation pattern.
     */
    void prepare();

    static QString extractBnameTrigger(const QString &exclude, bool wildcardsMatchSlash);
    static QString convertToRegexpSyntax(QString exclude, bool wildcardsMatchSlash);

    /// Files to load excludes from
    QSet<QString> _excludeFiles;

    /// Exclude patterns added with addManualExclude()
    QStringList _manualExcludes;

    /// List of all active exclude patterns
    QStringList _allExcludes;

    /// see prepare()
    QRegularExpression _bnameTraversalRegexFile;
    QRegularExpression _bnameTraversalRegexDir;
    QRegularExpression _fullTraversalRegexFile;
    QRegularExpression _fullTraversalRegexDir;
    QRegularExpression _fullRegexFile;
    QRegularExpression _fullRegexDir;

    /**
     * Whether * and ? in patterns can match a /
     *
     * Unfortunately this was how matching was done on Windows so
     * it continues to be enabled there.
     */
    bool _wildcardsMatchSlash = false;

    /**
     * The client version. Used to evaluate version-dependent excludes,
     * see versionDirectiveKeepNextLine().
     */
    QVersionNumber _clientVersion;

    friend class TestExcludedFiles;
};

#endif /* _CSYNC_EXCLUDE_H */
