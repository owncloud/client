Sync Algorithm
==============

Overview
--------

This is a technical description of the synchronization (sync) algorithm used by the ownCloud client.

The sync algorithm is the thing that looks at the local tree, database and remote tree and decides which steps need to be taken to bring the two trees into synchronization. It's different from the propagator, whose job it is to actually execute these steps.


Definitions
-----------

  - local tree: The files and directories on the local file system that shall be kept in sync with the remote tree.
  - remote tree: The files and directories on the ownCloud server that shall be kept in sync with the local tree.
  - database (db): A snapshot of file and directory metadata that the sync algorithm uses as a baseline to detect local or remote changes.
  - file and directory metadata:
    - mtimes
    - sizes
    - inodes (db and local only): Representation of filesystem object. Useful for rename detection.
    - etags (db and remote only): The server assigns a new etag when a file or directory changes.
    - checksums (db and remote only): Checksum algorithm applied to a file's contents.
    - permissions (db and remote only)


Phases
------

### Discovery (aka Update)

The discovery phase collects file and directory metadata from the local and remote trees, detecting differences between each tree and the database.

Afterwards, we have two trees that tell us what happened relative to the db. But there may still be conflicts if something happened to an entity both locally and on the remote.

  - Input: file system, server data, database
  - Output: c_rbtree_t* for the local and remote trees

  - Note on remote discovery: Since a change to a file on the server causes the etags of all parent folders to change, folders with an unchanged etag can be read from the db directly and don't need to be walked into.

  - Details
    - csync_update() uses csync_ftw() on the local and remote trees, one after the other.
    - csync_ftw() iterates through the entities in a tree and calls csync_walker() for each.
    - csync_walker() calls _csync_detect_update() on each.
    - _csync_detect_update() compares the item to its db entry (if any) to detect new, changed or renamed files. This is the main function of this pass.



### Reconcile

The reconcile phase compares and adjusts the local and remote trees (in both directions), detecting conflicts.

Afterwards, there are still two trees, but conflicts are marked in them.

  - Input: c_rbtree_t* for the local and remote trees, database (for some rename-related queries)
  - Output: changes c_rbtree_t* in-place

  - Details
    - csync_reconcile() runs csync_reconcile_updates() for the local and remote trees, one after the other.
    - csync_reconcile_updates() uses c_rbtree_walk() to iterate through the entries, calling _csync_merge_algorithm_visitor() for each.
    - _csync_merge_algorithm_visitor() checks whether the other tree also has an entry for that node and merges the actions, detecting conflicts. This is the main function of this pass.


### Post-Reconcile

The post-reconcile phase merges the two trees into one set of SyncFileItems.

Afterwards, there is a list of items that can tell the propagator what needs to be done.

  - Input: c_rbtree_t* for the local and remote trees
  - Output: QMap<QString, SyncFileItemPtr>

  - Note that some "propagations", specifically cheap metadata-only updates, are already done at this stage.

  - Details
    - csync_walk_local_tree() and csync_walk_remote_tree() are called.
    - They use _csync_walk_tree() to run SyncEngine::treewalkFile() on each entry.
    - treewalkFile() creates and fills SyncFileItems for each entry, ensuring that each file only has a single instance. This is the main function of this pass.

