/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef ACTIVITYDATA_H
#define ACTIVITYDATA_H

#include <QtCore>

namespace OCC {
/**
 * @brief The ActivityLink class describes actions of an activity
 *
 * These are part of notifications which are mapped into activities.
 */

class ActivityLink
{
public:
    QString _label;
    QString _link;
    QByteArray _verb;
    bool _isPrimary;
};

/* ==================================================================== */
/**
 * @brief Activity Structure
 * @ingroup gui
 *
 * contains all the information describing a single activity.
 */

class Activity
{
public:
    typedef QPair<qlonglong, QString> Identifier;

    enum Type {
        ActivityType,
        NotificationType
    };

    Type _type;
    qlonglong _id;
    QString _subject;
    QString _message;
    QString _file;
    QUrl _link;        /* a link that might be passed as part of the activity */
    QString _linkText; /* unescaped label of the link above */
    QDateTime _dateTime;
    QString _accName;  /* display name of the account involved */

    QVector<ActivityLink> _links; /* These links are transformed into buttons that
                                   * call links as reactions on the activity */
    /**
     * @brief Sort operator to sort the list youngest first.
     * @param val
     * @return
     */


    Identifier ident() const;

    void extractLink( const QString& s )
    {
        if (!s.isEmpty()) {
            // if there is a real whitespace in the link, the part before the space
            // is rendered as a link text
            int charPos = s.lastIndexOf(QChar(' '));
            if (charPos > -1) {
                const QString link = s.mid(charPos+1); // all chars from the char to back
                _link = QUrl::fromEncoded(link.toUtf8());
                _linkText = s.mid(0, charPos);
            } else {
                _link = QUrl(s);
                _linkText.clear();
            }
        }
    }
};

bool operator==(const Activity &rhs, const Activity &lhs);
bool operator<(const Activity &rhs, const Activity &lhs);

/* ==================================================================== */
/**
 * @brief The ActivityList
 * @ingroup gui
 *
 * A QList based list of Activities
 */

typedef QList<Activity> ActivityList;
}

#endif // ACTIVITYDATA_H
