/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <QJSEngine>
#include <QtQuickWidgets/QQuickWidget>

class QUrl;

#define OC_DECLARE_WIDGET_FOCUS                                                                                                                                \
public:                                                                                                                                                        \
    Q_INVOKABLE void focusNext()                                                                                                                               \
    {                                                                                                                                                          \
        focusNextChild();                                                                                                                                      \
    }                                                                                                                                                          \
    Q_INVOKABLE void focusPrevious()                                                                                                                           \
    {                                                                                                                                                          \
        focusPreviousChild();                                                                                                                                  \
    }                                                                                                                                                          \
                                                                                                                                                               \
Q_SIGNALS:                                                                                                                                                     \
    void focusFirst();                                                                                                                                         \
    void focusLast();                                                                                                                                          \
                                                                                                                                                               \
private:

namespace OCC::QmlUtils {
class OCQuickWidget : public QQuickWidget
{
    Q_OBJECT
    // override of the enabled property of QWidget, but with a notifier
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(QWidget* parentFocusWidget MEMBER _parentFocusWidget FINAL)
    QML_ELEMENT
    QML_UNCREATABLE("C++")
public:
    using QQuickWidget::QQuickWidget;
    void setOCContext(const QUrl &src, QWidget *parentFocusWidget, QObject *ocContext, QJSEngine::ObjectOwnership ownership);
    void setOCContext(const QUrl &src, QWidget *ocContext);

Q_SIGNALS:
    void focusFirst();
    void focusLast();

    void enabledChanged();

protected:
    void focusInEvent(QFocusEvent *event) override;

    bool event(QEvent *event) override;

private:
    QWidget *_parentFocusWidget;
};
}
