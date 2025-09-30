// SPDX-License-Identifier: GPL-2.0
#pragma once

namespace OCC {

struct SocketProtocol
{
    // Separator used to separate multiple paths in a single argument
    static constexpr QChar PathSeparator = QLatin1Char('\x1e');

    // Commands
    inline static const QString CommandCopyPrivateLinkMenuTitle = QString("COPY_PRIVATE_LINK_MENU_TITLE");
    inline static const QString CommandContextMenuTitle = QString("CONTEXT_MENU_TITLE");
    inline static const QString CommandRegisterPath = QString("REGISTER_PATH");
    inline static const QString CommandShareMenuTitle = QString("SHARE_MENU_TITLE");
    inline static const QString CommandStatus = QString("STATUS");
    inline static const QString CommandUnregisterPath = QString("UNREGISTER_PATH");
    inline static const QString CommandUpdateView = QString("UPDATE_VIEW");
};

}
