## Reason

* Needed for branded client X.x.x release (link to branded release ticket)

### Template
[Release Template](https://github.com/owncloud/client/blob/master/.github/release_template.md)

__TODO__:
* Check if anything is missing from [release_template_outdated_2020.md](https://github.com/owncloud/client/blob/master/.github/release_template_outdated_2020.md) and merge here. We use the same template for a patch, minor or major release now.
* For each item add a link to the respective action if available

### Administration

* [ ] QA: Update [Test Plans](https://infinite.owncloud.com/f/31e6d44f-f373-557c-9ab3-1748fc0c650d$4994cd9c-1c17-4254-829a-f5ef6e1ff7e3%21c0c0d47e-5da5-4254-8b02-bd8e97d43dfb)
* [ ] Update [supported platforms](https://doc.owncloud.com/desktop/next/installing.html#system-requirements) @DeepDiver1975
* [ ] QA: Check the translations coming from transifex: https://github.com/owncloud/client/commits/ -> Filter based on a release branch/tag and search for `[tx] updated client translations from transifex [skip ci]`
* [ ] DEV: Check for new OpenSSL version 
* [ ] DEV: Update [dependencies](https://infinite.owncloud.com/f/31e6d44f-f373-557c-9ab3-1748fc0c650d$4994cd9c-1c17-4254-829a-f5ef6e1ff7e3%21d231d3ed-498f-42b8-87e0-87420e66e16c)
* [ ] DEV: For a major release create `X` version branch
  * [ ] QA: In drone adjust the branch for nightly [GUI tests](https://infinite.owncloud.com/f/31e6d44f-f373-557c-9ab3-1748fc0c650d$4994cd9c-1c17-4254-829a-f5ef6e1ff7e3%21be0a8bd0-0029-4335-9c4a-98303d89aa9f) @individual-it
* [ ] QA: Communicate documentation changes  
   * [ ] Inform documentation team about the start of testing phase (latest a week before the release!). They'll prepare a PR with respective doc version
   * [ ] Open issues in ``docs-client-desktop`` repo for already known doc-relevant items and mark them accordingly, e.g. backport to 2.X.x necessary
* [ ] Decide if the prerelease stage will be public or internal @DeepDiver1975 
  
### Copy for Each Build (Beta/RC)

* [ ] DEV: Tag (Beta or RC) and build [builds](https://gitea.owncloud.services/client/do_release) for theme 'ownCloud' and 'testpilotcloud'
* [ ] QA: [Smoke test](https://infinite.owncloud.com/f/31e6d44f-f373-557c-9ab3-1748fc0c650d$4994cd9c-1c17-4254-829a-f5ef6e1ff7e3%21bc6b66c2-84e3-4a92-926c-c7fa2492f85e)
* [ ] DEV: GitHub release
* [ ] Beta/RC Communication
    * [ ] TODO
    * [ ] For public prerelease: Write/edit Central post https://central.owncloud.org/tags/c/news/desktop with link to GitHub release 
* [ ] DEV: Prepare the update server for new version (AppImages included)
   * [ ] DEV: Provide 'testpilotcloud' on **Beta** update channel
     
### QA 

* [ ] QA: [Antivirus scan](https://www.virustotal.com/) the first RC
* [ ] QA: [Upload](https://confluence.owncloud.com/display/OG/Upload+linux+gpg+keys+to+key+server) linux gpg keys to key server
* [ ] QA: Check Crash reporter:  start 'owncloud --debug' on cmd line, system tray right click menu: 'Crash now - qt fatal' -> report window not empty, sending the report works
  * [ ] Windows  
  * [ ] macOS
  * [ ] AppImage (Linux)
* [Automated tests](https://infinite.owncloud.com/f/31e6d44f-f373-557c-9ab3-1748fc0c650d$4994cd9c-1c17-4254-829a-f5ef6e1ff7e3%21073f546f-59af-42f2-8a47-1480a051b6cc) (for the first beta and the last RC):
   * [ ] QA: GUI tests passed on a tag
   * [ ] QA: All [Linux platform install](https://infinite.owncloud.com/f/31e6d44f-f373-557c-9ab3-1748fc0c650d$4994cd9c-1c17-4254-829a-f5ef6e1ff7e3%21bd275422-5fff-414e-aa67-23cad9b4aac8)
* Manual tests:
   * [ ] QA: [Changelog](https://github.com/owncloud/client/blob/master/CHANGELOG.md) test
   * [ ] QA: Regression test
   * [ ] QA: Branded regression test

### Prerequisites for final release

* [ ] DEV: Create vX.Y.Z release tag in client repo and everywhere else
* [ ] DEV: bump VERSION.cmake in master to say X.(Y+1).Z unless already done.

### Final Rebuild after QA Approval

* [ ] QA: Inform documentation team that the tag for the final release will be set a day or at least half a day __before__ (only for a major/minor release). They'll merge docs PR before that.
* [ ] DEV: Create final release tag (e.g., `vX.Y.Z`)
* [ ] DEV: Create [builds](https://confluence.owncloud.com/display/OG/Build+and+Tags#BuildandTags-Tags) for themes 'ownCloud' and 'testpilotcloud' for final release tag
* [ ] DEV: Update version for future builds
* [ ] QA: Check [squish tests](https://infinite.owncloud.com/f/31e6d44f-f373-557c-9ab3-1748fc0c650d$4994cd9c-1c17-4254-829a-f5ef6e1ff7e3%21073f546f-59af-42f2-8a47-1480a051b6cc) running successfully on [drone](https://drone.owncloud.com/owncloud/client) for the final tag vX.Y.Z
* [ ] QA: Create a new release issue for a branded release if needed [Branded Client Release Template](https://github.com/owncloud/enterprise/blob/master/internal_release_templates/internal_client_release_template.md)

### Final Steps

* [ ] QA: [Smoke test](https://infinite.owncloud.com/f/31e6d44f-f373-557c-9ab3-1748fc0c650d$4994cd9c-1c17-4254-829a-f5ef6e1ff7e3%21bc6b66c2-84e3-4a92-926c-c7fa2492f85e)
* [ ] DEV: Publish the release in GitHub
* [ ] QA: Update lines 4 and 5 of [install docs](https://github.com/owncloud/docs-client-desktop/blob/master/modules/ROOT/pages/installing.adoc) with the final build number. Merge into master and backport to the respective branch
* [ ] QA: Check that [documentation](https://doc.owncloud.com/desktop/next/) offers the new version
* [ ] DEV: Merge version branch into master
* [ ] DEV: Update [SBOM](https://cloud.owncloud.com/f/6072843)

### [Marketing and Communication](https://confluence.owncloud.com/display/OG/Marketing+and+Communication)
   
* [ ] QA: Inform Kiteworks marketing to update links on https://owncloud.com/desktop-app/ (provide links from GitHub releases) and [wiki de](https://de.wikipedia.org/wiki/OwnCloud), [wiki en](https://en.wikipedia.org/wiki/OwnCloud), [wikidata](https://www.wikidata.org/wiki/Q20763576)) 
* [ ] QA: Central post https://central.owncloud.org/tags/c/news/desktop

### Infrastructure Check

* [ ] QA/DEV: Update [stable channel](https://confluence.owncloud.com/display/OG/Online+Updater%2C+Crash+reporter%2C+Transifex#OnlineUpdater,Crashreporter,Transifex-UpdatetheUpdater) in the owncloud hosted auto updater
* [ ] QA: Check the linux download pages whether the URLs are correct (e.g. contain stable, not testing)
* [ ] QA: Ensure that the [client release template](https://github.com/owncloud/client/blob/master/.github/release_template.md) is up to date

### A Few Days After the Release

* [ ] DEV: Check the [crash reporter](https://confluence.owncloud.com/display/OG/Online+Updater%2C+Crash+reporter%2C+Transifex#OnlineUpdater,Crashreporter,Transifex-CrashReporter) for bad/frequent crashes
