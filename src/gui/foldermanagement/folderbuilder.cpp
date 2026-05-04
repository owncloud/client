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
    SyncEngine *engine = buildEngine(accountState->account(), db, ignoreHiddenFiles);
    if (db && vfs && engine) {
        // for unknown reasons I am getting a warning on potential memory leak for the engine only - it's safe to ignore this as
        // all pointers created in this class are parented by the builder (then transferred to folder) so even if
        // the folder build fails, the pointers will be cleaned up when builder goes out of scope.
        return new Folder(_definition, accountState, db, vfs, engine, parent);
    }
    return nullptr;
}

SyncJournalDb *FolderBuilder::buildJournal()
{
    SyncJournalDb *journal = new SyncJournalDb(_definition.absoluteJournalPath(), this);
    if (!journal->open()) {
        qCWarning(lcFolderBuilder) << "Could not open database when creating new folder: " << _definition.absoluteJournalPath() << ". Aborting Folder build.";
        return nullptr;
    }
    journal->close();
    journal->allowReopen();
    // those errors should not persist over sessions so kill them if there are any in an existing journal
    journal->wipeErrorBlacklistCategory(SyncJournalErrorBlacklistRecord::Category::LocalSoftError);
    return journal;
}

Vfs *FolderBuilder::buildVfs()
{
    Vfs *vfs = VfsPluginManager::instance().createVfsFromPlugin(_definition.virtualFilesMode(), this);
    if (vfs)
        return vfs;

    qCWarning(lcFolderBuilder) << "Could not load vfs plugin for mode" << _definition.virtualFilesMode() << ". Aborting Folder build.";
    return nullptr;
}

SyncEngine *FolderBuilder::buildEngine(Account *account, SyncJournalDb *journal, bool ignoreHiddenFiles)
{
    if (!account || !journal)
        return nullptr;
    SyncEngine *engine = new SyncEngine(account, _definition.webDavUrl(), _definition.canonicalPath(), _definition.targetPath(), journal, this);
    engine->setIgnoreHiddenFiles(ignoreHiddenFiles);
    if (!engine->loadDefaultExcludes()) {
        qCWarning(lcFolderBuilder, "Engine could not read system exclude file. Aborting Folder build");
        return nullptr;
    }
    return engine;
}


}
