Change: Drive network job timeouts with a job-owned timer

We reworked how network requests enforce their timeout. Instead of relying on
Qt's request transfer timeout - which is owned by the network reply - each job
now owns its timeout timer and resets it on transfer progress. This removes the
reply-internal timer whose queued callback could reach an already-deleted reply
after waking from sleep, fully eliminating the class of crash addressed by the
earlier hotfix.

https://github.com/owncloud/client/issues/12600
