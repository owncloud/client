/*
 * This file was originally licensed under ownCloud Commercial License.
 * see <https://owncloud.com/licenses/owncloud-commercial/>
 * As of 2025, it is relicensed under MIT.
 */
// SPDX-License-Identifier: GPL-2.0-or-later
#include "vfs_win.h"
#include "common/vfs.h"

#include <QtPlugin>


namespace OCC {

class SuffixVfsPluginFactory : public QObject, public DefaultPluginFactory<VfsWin>
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.owncloud.PluginFactory" FILE "vfspluginmetadata.json")
    Q_INTERFACES(OCC::PluginFactory)
};

}

#include "vfs_win_plugin_factory.moc"
