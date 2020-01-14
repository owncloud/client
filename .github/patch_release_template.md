## Desktop Client Patch Release Template


### Create Changelog

* [ ] Dev: Update [ChangeLog](https://handbook.owncloud.com/release_processes/client/change_log.html) - @TheOneRing @guruz
* [ ] Create a (draft) release [here](https://github.com/owncloud/client/releases)
* [ ] 1h later check [changelog on website](https://owncloud.org/changelog/desktop-client/) -> it pulls from the master branch ChangeLog file hourly.  - @jnweiger


### Prerequisites

* [ ] Dev: Check [dependencies](https://handbook.owncloud.com/release_processes/client/dependencies.html) for updates - @TheOneRing @guruz
* [ ] QA: Update [Test Plans](https://handbook.owncloud.com/release_processes/client/testlink.html) - @HanaGemela @jnweiger
* [ ] QA: Review list of [supported platforms](https://handbook.owncloud.com/release_processes/client/supported_platforms.html) -  @HanaGemela @jnweiger @TheOneRing @guruz
* [ ] QA: Update [documentation](https://handbook.owncloud.com/release_processes/client/documentation.html) -  @HanaGemela @jnweiger
* [ ] Branch off a sprint branch called sprint-1, sprint-2, ... (unless done earlier???)


### Sprint Build

* [ ] Create [builds](https://handbook.owncloud.com/release_processes/client/build.html#sprint-build) for theme 'ownCloud' and 'testpilotcloud'  @jnweiger @hvonreth
* [ ] Check if *tar.xz.asc files are [here](https://download.owncloud.com/desktop/testing). If not follow the [instructions](https://github.com/owncloud/enterprise/wiki/Desktop-Signing-Knowledge)
* [ ] Linux: add  [build targets](https://handbook.owncloud.com/release_processes/client/supported_platforms.html) @hvonreth @jnweiger
* [ ] Branch isv:ownCloud:desktop to isv:ownCloud:desktop:client-X.Y.Z using [obs_integration](https://github.com/owncloud/administration/blob/master/jenkins/obs_integration/) the Linux packages will always land in the :testing repository - @jnweiger
  ```obs-deepcopy-prj.sh isv:ownCloud:desktop isv:ownCloud:desktop:client-2.5.1```


### After build succeeds

* [ ] QA: [Antivirus scan](https://handbook.owncloud.com/release_processes/client/virus.html) - @HanaGemela @jnweiger 
* [ ] Announce the new branch to community - marketing
* [ ] Communicate the release schedule on rocket-chat #release-coordination and mailinglist release-coordination@owncloud.com. Give a high level overview of the upcoming new features, changes etc.
* [ ] Ensure marketing is aware (marketing@owncloud.com) and prepared for the release (social, .com website, cust.communications) - 1 week before minor, 2 weeks before major (minor/major is about impact)
* [ ] Inform Achim (ageissel@owncloud.com) and GCX that the next version will be in 1 week (gcx@owncloud.com) - marketing


### Sprint QA

#### DEV QA
* [ ] DEV: Run [the smoke test](https://handbook.owncloud.com/release_processes/client/smoke_test.html)
* [ ] Linux: Run [test](https://gitea.owncloud.services/client/linux-docker-install/src/branch/master/RUN.sh) with repo=https://download.opensuse.org/repositories/isv:/ownCloud:/desktop:/testing
* [ ] Run [the smoke test](https://handbook.owncloud.com/release_processes/client/smoke_test.html)
* [ ] Linux: Run [test](https://gitea.owncloud.services/client/linux-docker-install/src/branch/master/RUN.sh) repo=https://download.opensuse.org/repositories/isv:/ownCloud:/desktop:/testing - @jnweiger
* [ ] Review drone results: `make test` TODO: Mac, [Lin](https://drone.owncloud.services/client/build-linux), Win? 
* [ ] Run the tx.pl scripts on the final code tag (20181109jw: really? What does that test?) @ogoffart
* [ ] Run smashbox (20180719 jw: FIXME: add details, how?) (ask @dschmidt, put link to smashbox results here)

#### BTR QA
* [ ] changelog testing, TO-BE-DEFINED @HanaGemela
* [ ] small regression test, TO_BE_DEFINED @HanaGemela

* [ ] If QA fails: Abort here? or rather 1) Back to Dev team, 2) goto start of "Build" section


### After QA Approval

* [ ] DEV: remove git suffix "2.6.1git" -> "2.6.1" in [`VERSION.cmake`](https://handbook.owncloud.com/release_processes/client/branch.html#version-cmake) - @TheOneRing @guruz
* [ ] On branch sprint-1 add the release tag v2.6.1
* [ ] Create a [signed tag](https://github.com/owncloud/enterprise/wiki/Desktop-Signing-Knowledge) using ```git tag -u E94E7B37 tagname```  @guruz @jnweiger
* [ ] Create same tag for MSI code - @dschmidt 
* [ ] Create same tag for Windows toolchain - @dschmidt 
* [ ] Create same tag (actually a symlink) for macOS toolchain - @dschmidt 

### Final Rebuild

* [ ] tarball build  (owncloud-v2.6.1.tar.xz) using 
* [ ] bump version.cmake  on 2.6 branch(2.6.1 -> 2.6.2)
* [ ] Make sure to increase the version number of the branch of release to name the next patch release version, e.g. if you release 2.3.2 then you should change VERSION.cmake in 2.3 to 2.3.3 since that branch now will be 2.3.3
* [ ] QA: Adjust [Linux Templates](https://handbook.owncloud.com/release_processes/client/branch.html#linux-templates) to support the next patch release version - @HanaGemela @jnweiger
* [ ] QA: Adjust [ownBrander](https://handbook.owncloud.com/release_processes/client/branch.html#ownbrander) - @HanaGemela @jnweiger
* [ ] QA: Use `obs-copyprj.sh` to backup the desktop project to `desktop:client-2.6.x` (unless already done) - @HanaGemela @jnweiger
* [ ] Win/Mac Copy builds from ```testing``` to ```stable``` on download.owncloud.com, double check the download links. (make sure the .asc is there too.
* [ ] Linux: also copy the *.linux-repo.html files from ```testing``` to ```stable``` **and** edit away the `:testing` strings.
* [ ] Linux: disable publishing on project isv:ownCloud:desktop
* [ ] Linux: copy from testing to released in OBS:
  ```obs-deepcopy-prj.sh isv:ownCloud:desktop:testing isv:ownCloud:desktop```
  ```obs-deepcopy-prj.sh isv:ownCloud:testpilot:testing isv:ownCloud:testpilot```
* [ ] Linux: Wait until everything is built then disable rebuild on project isv:ownCloud:desktop
* [ ] Linux: Re-enable OBS publishing on the project after official release date and if all distros build (check for accidentially disabled packages too) 


### Final QA

* [ ] Test all advertised download links to have the expected version
* [ ] Inform product management and marketing and #general channel in rocker chat that we are 1 day out
* [ ] QA: Final release [ smoke test](https://handbook.owncloud.com/release_processes/client/smoke_test.html)


### Communicate the Availability

* [ ] Update the wordpress content at owncloud.org/download @florian - marketing
* [ ] Add the previous release to [older version](https://owncloud.org/download/older-versions/) @jnweiger + fwittek
* [ ] Inform packagers: @dragotin (openSUSE) - marketing
* [ ] Announce on [central](https://central.owncloud.org) (copy old announcement, link to changelog, download links etc) TODO: itemize what goes into the announcement: deprecation warnings. ... - marketing
* [ ] Inform community mailinglists devel@owncloud.org and testpilots@owncloud.org (make sure to mention it is an rc). Link to the central post so discussion happens there. - marketing
* [ ] Update [org website](https://owncloud.org/download/#owncloud-desktop-client) -> Download ownCloud -> click open 'Desktop Client', edit win/mac/lin, each all three tabs "Production", "Technical Preview" [disabled], "Test pilot" enabled, edit the links.
* [ ] Ping marketing to do their [actions](https://handbook.owncloud.com/release_processes/client/marketing.html)
* [ ] Take pride and celebrate!
* [ ] Tell the support team (GCX) to increment the minimum supported version for enterprise customers - @mstingl
* [ ] Check if [minimum.supported.desktop.version](https://github.com/owncloud/core/blob/master/config/config.sample.php#L1152) needs to be updated in server

### Final Infrastructure Check

* [ ] Update unstable channel in the owncloud hosted auto updater. Instructions [here](https://github.com/owncloud/enterprise/blob/master/client_update_checker/README.md#deploy) and [here](https://handbook.owncloud.com/release_processes/client/desktop.html#update-the-updater) @hgemela @jnweiger
* [ ] Check [crash reporter](https://handbook.owncloud.com/release_processes/client/desktop.html#crash-reporter) for bad crashes of this RC (same crash happening to many users) @guruz @hvonreth
* [ ] Check the translations coming from transifex: All synchronized? TODO: (20181109jw: where? how?)
* [ ] Increment version number in daily builds. Special case: after the last release in a branch, jump forward to the 'next release branch'... That may mean, this is nightly is the same as edge then. WHAT EXACTLY?
* [ ] Ensure that the [client patch release template](https://github.com/owncloud/client/master/.github/release_template_patch.md) is up to date: TODO: in github, or in handbook?


### A Few Days After the Release

* [ ] Check the crash reporter if auto update is a good idea or we need a new release
* [ ] Update throtteling in the owncloud hosted auto [updater](https://github.com/owncloud/enterprise/blob/master/client_update_checker/README.md#deploy)  

