/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHeaderView>

namespace OCC {

class ExpandingHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    ExpandingHeaderView(const QString &objectName, QWidget *parent = nullptr);
    ~ExpandingHeaderView();

    int expandingColumn() const;
    void setExpandingColumn(int newExpandingColumn);

    void resizeColumns(bool reset = false);
    void addResetActionToMenu(QMenu *menu);

    bool resizeToContent() const;
    void setResizeToContent(bool newResizeToContent);

protected:
    void resizeEvent(QResizeEvent *event) override;


private:
    bool _requiresReset = false;
    bool _resizeToContent = false;
    int _expandingColumn = 0;
};

}
