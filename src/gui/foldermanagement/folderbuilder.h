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

    Folder *buildFolder(AccountState *accountState, bool ignoreHiddenFiles, QObject *parent);


private:
    SyncJournalDb *buildJournal();
    SyncEngine *buildEngine();
    Vfs *buildVfs();

    FolderDefinition _definition;
};
}
