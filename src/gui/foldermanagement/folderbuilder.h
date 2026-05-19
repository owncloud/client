#pragma once

#include <QObject>

#include "folder.h"

namespace OCC {

class SyncJournalDb;
class SyncEngine;
class Vfs;

class FolderBuilder : public QObject
{
    Q_OBJECT

public:
    FolderBuilder(const FolderDefinition &definition, QObject *parent = nullptr);

    Folder *buildFolder(AccountState *accountState, bool ignoreHiddenFiles, bool moveToTrash, QObject *parent);


private:
    SyncJournalDb *buildJournal();
    Vfs *buildVfs();
    SyncEngine *buildEngine(Account *account, SyncJournalDb *journal, bool ignoreHiddenFiles, bool moveToTrash);

    FolderDefinition _definition;
};
}
