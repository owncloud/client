# AI Agent Guidelines for ownCloud Desktop Client

This file provides context for AI coding agents (Claude Code, GitHub Copilot, Cursor, etc.) working in this repository.

## Repository Overview
- **Product family:** Desktop Client
- **Primary language(s):** C++
- **Build system:** CMake
- **Test framework:** Qt Test, CTest
- **CI system:** GitHub Actions

## Architecture & Key Paths
- `src/` - Main source code (GUI, sync engine, networking)
- `test/` - Test suites
- `shell_integration/` - File manager shell integration modules
- `cmake/` - CMake modules and helpers
- `admin/` - Administrative scripts and packaging
- `translations/` - Translation files
- `CMakeLists.txt` - Root CMake configuration
- `VERSION.cmake` - Version definition
- `OWNCLOUD.cmake` - ownCloud-specific CMake variables
- `THEME.cmake` - Theming configuration
- `CONTRIBUTING.md` - Contribution guidelines
- `CHANGELOG.md` - Release history
- `sync-exclude.lst` - Default sync exclusion patterns

## Development Conventions
- **Branching:** master
- **Commit messages:** DCO sign-off required (`git commit -s`)
- **Code style:** EditorConfig (.editorconfig)
- **PR process:** Open a PR against master. All CI checks must pass.

## Build & Test Commands
```bash
# Build (using CMake)
mkdir build && cd build
cmake ..
cmake --build .

# Test
cd build && ctest

# Meta build system (recommended)
# See https://community.kde.org/Craft for KDE Craft instructions
```

## Important Constraints
- All code contributions must be compatible with the **GPL-2.0** license
- Do not introduce new **copyleft-licensed dependencies** (GPL, AGPL, LGPL, MPL) without explicit discussion in an issue first. This is especially important for repos migrating to Apache 2.0.
- Do not introduce new dependencies without discussion in an issue first
- C++17 standard is required (CMAKE_CXX_STANDARD 17)
- KDE heritage code exists - be aware of KDE-era copyrights
- Documentation is separately licensed under COPYING.documentation


## OSPO Policy Constraints

### GitHub Actions
- **Only** use actions owned by `owncloud`, created by GitHub (`actions/*`), or verified on the GitHub Marketplace.
- Pin all actions to their full commit SHA (not tags): `uses: actions/checkout@<SHA> # vX.Y.Z`
- Never introduce actions from unverified third parties.

### Dependency Management
- Dependabot is configured for automated dependency updates.
- Review and merge Dependabot PRs as part of regular maintenance.
- Do not introduce new dependencies without discussion in an issue first.

### Git Workflow
- **Rebase policy**: Always rebase; never create merge commits. Use `git pull --rebase` and `git rebase` before pushing.
- **Signed commits**: All commits **must** be PGP/GPG signed (`git commit -S -s`).
- **DCO sign-off**: Every commit needs a `Signed-off-by` line (`git commit -s`).
- **Conventional Commits**: Use the [Conventional Commits](https://www.conventionalcommits.org/) format where the repository enforces it.

## Context for AI Agents
- Match existing code style
- Do not refactor unrelated code in the same PR
- Write tests for new functionality
- Keep PRs focused and atomic
- Follow Qt/C++ conventions used in the existing codebase
- Shell integrations (Dolphin, Nautilus) are in separate repositories
