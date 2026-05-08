# ownCloud Desktop Client

<!-- OSPO-managed README | Generated: 2026-04-16 | v2 -->

[![License](https://img.shields.io/badge/License-GPL--2.0-blue.svg)](COPYING) [![ownCloud OSPO](https://img.shields.io/badge/OSPO-ownCloud-blue)](https://kiteworks.com/opensource)

The ownCloud Desktop Client synchronizes files between an ownCloud server and your local computer. It runs on Windows, macOS, and Linux, providing automatic background synchronization, selective sync, conflict resolution, and virtual file support. The client integrates with the native file manager through shell extensions for overlay icons and context menu actions.

## Part of Desktop Client

This is the core repository for the [ownCloud Desktop Client](https://github.com/owncloud/client). It connects to both [ownCloud Infinite Scale (oCIS)](https://github.com/owncloud/ocis) and [ownCloud Server (Classic)](https://github.com/owncloud/core). Download the latest release from [owncloud.com/desktop-app](https://owncloud.com/desktop-app/).

## Getting Started

Follow the steps below to install or build the desktop client.

### Binary Packages

Download from https://owncloud.com/desktop-app/

### Building from Source

This project uses CMake as its build system and KDE Craft as a meta build system.

- [CMake documentation](https://cmake.org/documentation/)
- [KDE Craft documentation](https://community.kde.org/Craft)
- [Build instructions](https://github.com/owncloud/ownbuild)

## Documentation

- [CHANGELOG.md](https://github.com/owncloud/client/blob/master/CHANGELOG.md) - Release history
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [PACKAGING.md](https://github.com/owncloud/client/blob/master/PACKAGING.md) - Packaging instructions
- [ownCloud documentation](https://doc.owncloud.com)

## Community & Support

**[Star](https://github.com/owncloud/client)** this repo and **Watch** for release notifications!

- [ownCloud Website](https://owncloud.com)
- [Community Discussions](https://github.com/orgs/owncloud/discussions)
- [Matrix Chat](https://app.element.io/#/room/#owncloud:matrix.org)
- [Documentation](https://doc.owncloud.com)
- [Enterprise Support](https://owncloud.com/contact-us/)
- [OSPO Home](https://kiteworks.com/opensource)

## Contributing

We welcome contributions! Please read the [Contributing Guidelines](CONTRIBUTING.md)
and our [Code of Conduct](CODE_OF_CONDUCT.md) before getting started.

### Workflow

- **Rebase Early, Rebase Often!** We use a rebase workflow. Always rebase on the target branch before submitting a PR.
- **Dependabot**: Automated dependency updates are managed via Dependabot. Review and merge dependency PRs promptly.
- **Signed Commits**: All commits **must** be PGP/GPG signed. See [GitHub's signing guide](https://docs.github.com/en/authentication/managing-commit-signature-verification).
- **DCO Sign-off**: Every commit must carry a `Signed-off-by` line:
  ```
  git commit -s -S -m "your commit message"
  ```
- **GitHub Actions Policy**: Workflows may only use actions that are (a) owned by `owncloud`, (b) created by GitHub (`actions/*`), or (c) verified in the GitHub Marketplace.

## Translations

Help translate this project on Transifex:
**<https://explore.transifex.com/owncloud-org/owncloud/>**

Please submit translations via Transifex -- do not open pull requests for translation changes.

## Security

**Do not open a public GitHub issue for security vulnerabilities.**

Report vulnerabilities at **<https://security.owncloud.com>** -- see [SECURITY.md](SECURITY.md).

Bug bounty: [YesWeHack ownCloud Program](https://yeswehack.com/programs/owncloud-bug-bounty-program)

## License

This project is licensed under the [GPL-2.0](COPYING).

## About the ownCloud OSPO

The [Kiteworks Open Source Program Office](https://kiteworks.com/opensource), operating under
the [ownCloud](https://owncloud.com) brand, launched on May 5, 2026, to steward the open source
ecosystem around ownCloud's products. The OSPO ensures transparent governance, license compliance,
community health, and sustainable collaboration between the open source community and
[Kiteworks](https://www.kiteworks.com), which acquired ownCloud in 2023.

- **OSPO Home**: <https://kiteworks.com/opensource>
- **GitHub**: <https://github.com/owncloud>
- **ownCloud**: <https://owncloud.com>

For questions about the OSPO or licensing, contact ospo@kiteworks.com.

### License Migration to Apache 2.0

The OSPO is driving a strategic relicensing of ownCloud repositories toward the
[Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0), following
the [Apache Software Foundation's third-party license policy](https://www.apache.org/legal/resolved.html).

Individual repositories will migrate as their audit is completed. The LICENSE file
in each repo reflects its **current** license status (not the target).

**Current license: GPL-2.0** (Category X per Apache policy -- cannot be included in Apache-2.0 works).

Migration prerequisites for this repository:

- **CLA/DCO coverage**: All past contributors must have signed agreements permitting relicensing
- **Copyleft dependency audit**: All GPL dependencies must be replaced or isolated
- **KDE heritage review**: Any code with KDE-era copyrights requires legal analysis
- **Complete relicensing**: GPL-2.0 is a strong copyleft license; migration requires full relicensing of all files
