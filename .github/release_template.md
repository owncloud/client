<!--
This is the template for new release issues.

(originated from https://github.com/owncloud/client/wiki/Release%20Checklist%20Template)

-->

Copy below text into a task and tick the items:

```
Release-1 Week:
* [ ] Check if we should update the bundled sqlite3 (https://github.com/owncloud/client/tree/master/src/3rdparty/sqlite3)
* [ ] Check if we should update Sparkle on build machine (https://github.com/sparkle-project/Sparkle/releases)
* [ ] Ensure NSIS is up to date on the build machine
* [ ] Ensure up-to-date dependencies (e.g. [latest Qt version](http://qt-project.org/downloads#qt-lib) is installed on the machine and picked up (cmake output)
* [ ] Ensure the crash reporter server is up
* [ ] Ensure Windows Overlay DLLs are rebuilt
* [ ] Check nightly builds are up and running, that is Jenkins jobs ownCloud-client-linux, ownCloud-client-osx and ownCloud-client-win32 all green.
* [ ] Ensure Linux nightlies are built too for all distros https://build.opensuse.org/package/show/isv:ownCloud:community:nightly/owncloud-client
* [ ] Build branded clients through the scripting machine and smoke test one or two branded clients (especially with predefined url)
* [ ] Upload a nightly build of the windows version to virustotal.com
  * Contact AV vendors whom's engine reports a virus
* [ ] Documentation should be online before the release http://doc.owncloud.org/desktop/1.X/
* [ ] QA goes over https://github.com/owncloud/mirall/wiki/Testing-Scenarios
* [ ] Communicate the release schedule on mailinglist release-coordination@owncloud.com. Give a high level overview of the upcoming new features, changes etc.
* [ ] Make sure to have `client/ChangeLog` updated
 * use `git log --format=oneline v<lastrelease>...master` if your memory fails you
* [ ] Ensure marketing is aware and prepared for the release (social, .com website, cust. communications)
* [ ] Inform GCX knows the next version is about 1 week out (gcx@owncloud.com)

For all Betas and RCs:
* [ ] Branch off a release branch called <version>-rcX or <version>-betaX
* [ ] Edit ```VERSION.cmake``` to set the suffix to beta1, beta2 etc. Commit the result to the release branch only
* [ ] Create build for Windows using rotor job owncloud-client-win32 (uncheck the "nightly build" checkbox, check the "sign package" checkboxes) both themes 'ownCloud' and 'testpilotcould'
* [ ] Create build for Mac using rotor, job owncloud-client-osx (uncheck the "nightly build" checkbox, check the "sign package" checkboxes) both themes 'ownCloud' and 'testpilotcould'
* [ ] Create the beta tarball using Jenkins job ownCloud-client-source
* [ ] Create Linux builds using rotor job owncloud-client-linux building both themes 'ownCloud' and 'testpilotcould' (this magically interacts with the ownCloud-client-source job)
* [ ] Copy builds from ```daily``` to ```testing``` on download.owncloud.com, double check the download links.
* [ ] Update the owncloud.org webpage, section testing, by providing a pull request to the https://github.com/owncloud/owncloud.org github repository.
* [ ] Inform community mailinglists devel@owncloud.org and testpilots@owncloud.org and packaging@owncloud.org
* [ ] Announce on https://central.owncloud.org
* [ ] Create a signed tag using ```git tag -u E94E7B37 tagname``` (https://github.com/owncloud/enterprise/wiki/Desktop-Signing-Knowledge)
* [ ] Check crash reporter

For first Beta of a Major or Minor release:
* [ ] branch off master to new version branch (e.g. master -> 2.1, when releasing 2.1)
* [ ] Adjust `VERSION.cmake` in master and count up (e.g. 2.2)
* [ ] Adjust translation jobs for [client](https://ci.owncloud.org/view/translation-sync/job/translation-sync-client/) and [NSIS](https://ci.owncloud.org/view/translation-sync/job/translation-sync-client-nsis/) to point to the release branch (e.g. 2.1).
* [ ] Make sure there is a job for the docs of the new master branch and the current release branch on rotor.
* [ ] check if enterprise issues are fixed

Day before Release:
* [ ] Check the translations coming from transifex: All synchronized?
* [ ] Run the tx.pl scripts on the final code tag
* [ ] Run ```make test```
* [ ] Run smashbox on the final code tag
* [ ] Inform product management and marketing that we are 1 day out

On Release Day (for final release):
* [ ] Branch off a release branch called <version>
* [ ] Double check ```VERSION.cmake```: Check the version number settings and suffix (beta etc.) to be removed. Commit change to release branch only!
* [ ] Add last updates to Changelog in the client source repository.
* [ ] Create tar ball (automated by `ownCloud-client-source` jenkins job) and **immediately** sign it (asc file). (https://github.com/owncloud/enterprise/wiki/Desktop-Signing-Knowledge)
* [ ] Copy the source tarball from the daily to the stable dir on download.o.o
* [ ] Announce the source tarball on the packaging mailing list packaging@owncloud.org
* [ ] Announce on https://central.owncloud.org
* [ ] Build Windows packages
* [ ] Build Mac OS X packages
* [ ] Build Linux packages by running the jenkins job ownCloud-client-linux with proper parameters
 * Update [OBS repository](https://build.opensuse.org/project/show?project=isv%3AownCloud%3Adesktop) `isv:ownCloud:desktop` (or `isv:ownCloud:community:testing` for RC/Beta)
 * Check if patches still apply in the linux packages
* [ ] Update the testing repository to the latest stable version.
* [ ] Inform GCX that a new tarball is available.
* [ ] Check if the following packages are on download.owncloud.com/desktop/stable:
  * Windows binary package
  * Mac binary package
  * source tarballs
* [ ] Create a pull request to the owncloud.org repository to update the install page (strings.php) and the changelog on owncloud.org. From now on download packages from the staging webserver.
* [ ] Re-download Mac builds and check signature. Interactive in installer window
* [ ] Re-download Win build check signature. From Mac or Linux: ```osslsigncode verify ownCloud-version-setup.exe```
* [ ] Mac: Perform smoke test (Install, make sure it does not explode, and check if all version indicators are correct)
* [ ] Win: Perform smoke test (Install, make sure it does not explode, and check if all version indicators are correct)
* [ ] Update ASCII Changelog on http://download.owncloud.com/download/changelog-client
* [ ] Keep the packaging mailinglist packaging@owncloud.org informed and announce the final sources.
* [ ] Create git signed tag in github client repository using ```git tag -u E94E7B37 tagname```
* [ ] Send out Social (tweet, blog, other)
* [ ] Send out customer communication (if any)
* [ ] Inform GCX that the new version is released (gcx@owncloud.com)
* [ ] Take pride and celebrate!
* [ ] Days later: Update the updater script ```clientupdater.php```
* [ ] Tell GCX to increment the minimum supported version for enterprise customers
* [ ] Check if minimum.supported.desktop.version (https://github.com/owncloud/core/blob/master/config/config.sample.php#L1152) needs to be updated in server
```
