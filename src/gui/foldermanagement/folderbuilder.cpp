#include "folderbuilder.h"

#include "common/syncjournaldb.h"
#include "common/vfs.h"
#include "folder.h"
#include "syncengine.h"


namespace OCC {

Q_LOGGING_CATEGORY(lcFolderBuilder, "gui.folderbuilder", QtInfoMsg)

FolderBuilder::FolderBuilder(const FolderDefinition &definition, QObject *parent)
    : QObject(parent)
    , _definition(definition)
{
}

Folder *FolderBuilder::buildFolder(AccountState *accountState, bool ignoreHiddenFiles, QObject *parent)
{
    if (!accountState || !accountState->account())
        return nullptr;

    SyncJournalDb *db = buildJournal();
    Vfs *vfs = buildVfs();
    if (db && vfs)
        return new Folder(_definition, accountState, db, vfs, ignoreHiddenFiles, parent);

    return nullptr;
}

SyncJournalDb *FolderBuilder::buildJournal()
{
    SyncJournalDb *journal = new SyncJournalDb(_definition.absoluteJournalPath(), this);
    if (!journal->open()) {
        qCWarning(lcFolderBuilder) << "Could not open database when creating new folder: " << _definition.absoluteJournalPath();
        return nullptr;
    }
    // those errors should not persist over sessions so kill them if there are any in an existing journal
    journal->wipeErrorBlacklistCategory(SyncJournalErrorBlacklistRecord::Category::LocalSoftError);
    return journal;
}

Vfs *FolderBuilder::buildVfs()
{
    Vfs *vfs = VfsPluginManager::instance().createVfsFromPlugin(_definition.virtualFilesMode(), this);
    if (vfs)
        return vfs;

    qCWarning(lcFolderBuilder) << "Could not load plugin for mode" << _definition.virtualFilesMode();
    return nullptr;
}

SyncEngine *FolderBuilder::buildEngine()
{
    return nullptr;
}


}
