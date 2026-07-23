/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "gui/qmlutils.h"

#include "common/asserts.h"
#include "resources/resources.h"

#include <QMessageBox>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QTimer>

void OCC::QmlUtils::OCQuickWidget::setOCContext(const QUrl &src, QWidget *parentFocusWidget, QObject *ocContext, QJSEngine::ObjectOwnership ownership)
{
    if (ownership == QJSEngine::CppOwnership) {
        // Destroying the `ocContext` will result in property changed signals, causing the re-evaluation
        // of the bindings in the QML file, which in turn results in warnings about accessing a property
        // of a `null` object.
        // To prevent this, reset the source to an empty URL.
        connect(
            ocContext, &QObject::destroyed, this, [this] { setSource(QUrl()); }, Qt::DirectConnection);
    }
    rootContext()->setContextProperty(QStringLiteral("ocQuickWidget"), this);
    rootContext()->setContextProperty(QStringLiteral("ocContext"), ocContext);
    engine()->setObjectOwnership(ocContext, ownership);
    engine()->addImageProvider(QStringLiteral("ownCloud"), new OCC::Resources::CoreImageProvider());
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    // Ensure the parent widget used OC_DECLARE_WIDGET_FOCUS
    Q_ASSERT(parentFocusWidget->metaObject()->indexOfMethod("focusNext()") != -1);
    Q_ASSERT(parentFocusWidget->metaObject()->indexOfMethod("focusPrevious()") != -1);
    _parentFocusWidget = parentFocusWidget;

    setSource(src);
    if (!errors().isEmpty()) {
        auto box = new QMessageBox(QMessageBox::Critical, QStringLiteral("QML Error"), QDebug::toString(errors()));
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->exec();
        qFatal("A qml error occured %s", qPrintable(QDebug::toString(errors())));
    }
}

void OCC::QmlUtils::OCQuickWidget::setOCContext(const QUrl &src, QWidget *ocContext)
{
    setOCContext(src, ocContext, ocContext, QJSEngine::ObjectOwnership::CppOwnership);
}

void OCC::QmlUtils::OCQuickWidget::focusInEvent(QFocusEvent *event)
{
    switch (event->reason()) {
    case Qt::TabFocusReason:
        Q_EMIT focusFirst();
        break;
    case Qt::BacktabFocusReason:
        Q_EMIT focusLast();
        break;
    default:
        break;
    }
    QQuickWidget::focusInEvent(event);
}

bool OCC::QmlUtils::OCQuickWidget::event(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange) {
        QTimer::singleShot(0, this, &OCQuickWidget::enabledChanged);
    }
    return QQuickWidget::event(event);
}
