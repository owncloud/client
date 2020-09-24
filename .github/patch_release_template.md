### Create Changelog

* [ ] DEV: Update [ChangeLog](https://handbook.owncloud.com/release_processes/client/change_log.html)
* [ ] QA: 1h later check [ChangeLog on website](https://owncloud.org/changelog/desktop-client/)

### Prerequisites

* [ ] QA: Update [Test Plans](https://handbook.owncloud.com/release_processes/client/testlink.html) 
* [ ] QA: Update [documentation](https://handbook.owncloud.com/release_processes/client/documentation.html)
* [ ] QA: Check the translations coming from [Transifex](https://handbook.owncloud.com/release_processes/client/miscellaneous.html#transifex)
* [ ] DEV: Check [dependencies](https://handbook.owncloud.com/release_processes/client/dependencies.html) for updates
* [ ] DEV: Branch off a sprint branch called e.g. `sprint-1`

### Build

* [ ] QA: Create [builds](https://handbook.owncloud.com/release_processes/client/build.html#sprint-build) for theme 'ownCloud' and 'testpilotcloud'
* [ ] QA: Check if `*tar.xz.asc` files are [here](https://download.owncloud.com/desktop/testing). If not follow the [instructions](https://github.com/owncloud/enterprise/wiki/Desktop-Signing-Knowledge)
* [ ] QA: Inform [stakeholders](https://handbook.owncloud.com/release_processes/client/marketing.html#inform-stakeholders-after-successful-sprint-build)

### QA

#### DEV QA
* [ ] DEV: [Smoke test](https://handbook.owncloud.com/release_processes/client/manual_tests.html#dev-smoke-test)
* [ ] DEV: Run [automated tests](https://handbook.owncloud.com/release_processes/client/automated_tests.html)
#### BTR QA
* [ ] QA: Inform subscribers of the beta program about a new version for testing
* [ ] QA: [Antivirus scan](https://handbook.owncloud.com/release_processes/client/virus.html)
* [ ] QA: Changelog testing
* [ ] QA: [Regression test](https://handbook.owncloud.com/release_processes/client/manual_tests.html#regression-test)

### After QA Approval

* [ ] DEV: Remove git suffix in `MIRALL_VERSION_SUFFIX` in [`VERSION.cmake`](https://handbook.owncloud.com/release_processes/client/branch.html#version-cmake)
* [ ] DEV: On sprint branch create a **signed** [tag](https://handbook.owncloud.com/release_processes/client/build.html#client-repository) v2.6.1
* [ ] DEV: Delete the sprint branch
* [ ] DEV: Create the same tag on branch 2.6 of [_client-plugin-vfs-win_](https://handbook.owncloud.com/release_processes/client/build.html#client-plugin-vfs-win)
* [ ] DEV: Create the same tag on branch 2.6 of [_wip-msi_](https://handbook.owncloud.com/release_processes/client/build.html#wip-msi)
* [ ] DEV: Create same tag for [Windows toolchain](https://handbook.owncloud.com/release_processes/client/build.html#windows-toolchain)
* [ ] DEV: Create same tag (actually a symlink) for [macOS toolchain](https://handbook.owncloud.com/release_processes/client/build.html#macos-toolchain)

### Final Rebuild

* [ ] QA: Change the date in `ChangeLog` to the date of creating the tag 
* [ ] QA: Bump `MIRALL_VERSION_PATCH` in [`VERSION.cmake`](https://handbook.owncloud.com/release_processes/client/branch.html#patch-release)
* [ ] QA: Adjust [Linux Templates](https://handbook.owncloud.com/release_processes/client/branch.html#linux-templates) to support the next patch release version (e.g. 2.6.2)
* [ ] QA: Run [client-trigger](https://handbook.owncloud.com/release_processes/client/build.html#final-build) 
* [ ] QA: Ping marketing to do their [actions](https://handbook.owncloud.com/release_processes/client/marketing.html#marketing)
* [ ] QA: [Backup](https://handbook.owncloud.com/release_processes/client/desktop.html#backup-the-previous-desktop-project) the desktop project in OBS to _desktop:client-2.6.0_
* [ ] QA: [Disable publishing](https://handbook.owncloud.com/release_processes/client/desktop.html#disable-publishing-and-on-project-isvownclouddesktop) on project _isv:ownCloud:desktop_ in OBS
* [ ] QA: Copy from [testing to released](https://handbook.owncloud.com/release_processes/client/desktop.html#copy-from-testing-to-released-in-obs) in OBS
* [ ] QA: Re-enable [OBS publishing](https://handbook.owncloud.com/release_processes/client/desktop.html#re-enable-obs-publishing)
* [ ] QA: [Copy](https://handbook.owncloud.com/release_processes/client/desktop.html#copy-from-testing-to-stable) builds from ```testing``` to ```stable```
* [ ] QA: Create [a (draft) release](https://github.com/owncloud/client/releases) with Download links - save as a draft until smoke tested
* [ ] QA: Create a new release issue for a branded release if needed
* [ ] QA: Give [heads-up](https://handbook.owncloud.com/release_processes/client/marketing.html#heads-up-before-the-final-release) before the final release 

### Final QA

* [ ] QA: [Smoke test](https://handbook.owncloud.com/release_processes/client/manual_tests.html#smoke-test)
* [ ] QA: Check if [GitHub download links](https://github.com/owncloud/client/releases) point to correct location
* [ ] QA: Publish the release in GitHub
* [ ] QA: Check [minimum.supported.desktop.version](https://github.com/owncloud/core/blob/master/config/config.sample.php#L1367) on the server

### Communicate the Availability

* [ ] QA: Announce on [central](https://handbook.owncloud.com/release_processes/client/marketing.html#central) listed in [Category News, desktop tag](https://central.owncloud.org/tags/c/news/desktop)
* [ ] QA: Inform [stakeholders](https://handbook.owncloud.com/release_processes/client/marketing.html#inform-stakeholders-about-final)
* [ ] QA: Inform [packagers](https://handbook.owncloud.com/release_processes/client/marketing.html#packagers) - ping @dragotin (openSUSE)

### Final Infrastructure Check

* [ ] QA/DEV: Update [stable channel](https://handbook.owncloud.com/release_processes/client/miscellaneous.html#update-the-updater) in the owncloud hosted auto updater
* [ ] QA/DEV: Ensure that the [client patch release template](https://github.com/owncloud/handbook/blob/master/modules/ROOT/examples/releases/desktop_patch_release_template.md) is up to date
* [ ] QA: Update [supported platforms](https://handbook.owncloud.com/release_processes/client/supported_platforms.html)

### A Few Days After the Release

* [ ] DEV: Check the [crash reporter](https://handbook.owncloud.com/release_processes/client/desktop.html#crash-reporter) for bad/frequent crashes
