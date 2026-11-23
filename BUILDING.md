# Building the ownCloud Desktop Client

This guide builds the client from source for development and testing, on Linux x86_64, Windows x86_64,
macOS arm64 and macOS Intel. It does not cover release packaging (AppImage, MSI, DMG).

The client is a CMake project. Its third-party dependencies (Qt, OpenSSL, qtkeychain, SQLite, Sparkle on
macOS, and the rest) come from [Conan 2](https://docs.conan.io/2/). There are two ways to get them:

- **[Prebuilt from ghcr.io](#a-prebuilt-dependencies-from-ghcrio)**: download the archive that CI uses.
  This takes a few minutes, and is the way to go unless you change the dependencies.
- **[From source](#b-building-the-dependencies-from-source)**: Conan builds every dependency on your machine.
  This takes an hour or more, mostly for Qt. You need it when your branch changes the Conan inputs (see below),
  or when there is no prebuilt archive for them.

Both end the same way: you [build and test the client](#building-and-testing-the-client) against the
dependencies.

## How the dependencies are defined

These files are the only source of truth for the dependencies, and together they are the **Conan inputs**:

- `conanfile.py`: the requirements, their options, and the exact Conan version (`required_conan_version`).
- `conan.lock`: the exact version and recipe revision of every package, for all four platforms. Change it only
  with [`conan/update-lock.sh`](conan/update-lock.sh).
- `conan/profiles/`: one profile per platform, with the compiler the packages are built with.
- `conan/recipes-commit`: the commit of [owncloud/conan-center-index](https://github.com/owncloud/conan-center-index)
  the recipes come from. Conan never contacts ConanCenter.

| Profile          | Platform                | Compiler                        | Build it on                 |
|------------------|-------------------------|---------------------------------|-----------------------------|
| `linux-x86_64`   | Linux x86_64            | gcc 13                          | Ubuntu 24.04 x86_64         |
| `windows-x86_64` | Windows x86_64          | MSVC 19.4x (Visual Studio 2022) | Windows x86_64              |
| `macos-armv8`    | macOS 13 or newer, arm64 | Apple clang 21                 | an Apple silicon Mac        |
| `macos-x86_64`   | macOS 13 or newer, Intel | Apple clang 21                 | an Apple silicon Mac, with Rosetta |

The macOS Intel dependencies are cross-built on an arm64 Mac, with `macos-armv8` as the build profile. Their
build tools are arm64 programs, so an Intel Mac cannot use them. The Intel client and its tests run under
Rosetta.

The compiler on your machine has to match the profile. Conan identifies a prebuilt package by its profile
settings, and the client links against the C++ libraries in them.

## Prerequisites

### All platforms

- Git, and a clone of this repository.
- Python 3.12 or newer, for Conan and `conan/fingerprint.py`.
- Ninja, as the CMake generator.
- CMake comes from Conan (the `cmake` tool requirement in `conanfile.py`), so you don't need your own.

### Linux

Ubuntu 24.04 on x86_64, with its default gcc 13. The profile checks for the system packages of Conan's
`xorg/system`, `opengl/system`, `egl/system` and `xkeyboard-config/system` recipes, but does not install them.
Install them, with Ninja and the libraries Qt and the tests need:

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential git python3 python3-venv ninja-build pkgconf \
  libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev libxau-dev libxaw7-dev \
  libxcomposite-dev libxcursor-dev libxdamage-dev libxdmcp-dev libxext-dev libxfixes-dev \
  libxi-dev libxinerama-dev libxkbfile-dev libxmu-dev libxmuu-dev libxpm-dev libxrandr-dev \
  libxrender-dev libxres-dev libxss-dev libxt-dev libxtst-dev libxv-dev libxxf86vm-dev \
  libxcb-glx0-dev libxcb-render0-dev libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev \
  libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-shape0-dev libxcb-sync-dev \
  libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-dri3-dev uuid-dev libxcb-cursor-dev \
  libxcb-dri2-0-dev libxcb-present-dev libxcb-composite0-dev libxcb-ewmh-dev libxcb-res0-dev \
  libxcb-util-dev libgl-dev libegl-dev xkb-data libopengl0
```

`libopengl0` provides `libOpenGL.so.0`, which the prebuilt Qt links. Without it, the client and its tests do
not start.

On another distribution you need gcc 13 and the same libraries under that distribution's package names. The
prebuilt archive's tools also need glibc 2.28 or newer.

### macOS

- An Xcode whose compiler is **Apple clang 21**, the `compiler.version=21` in the macOS profiles. Check the
  selected one with `xcrun clang --version`. If you have several, select one with:

  ```bash
  sudo xcode-select -s /Applications/Xcode-<version>.app/Contents/Developer
  ```

- Ninja, e.g. `brew install ninja`.
- For `macos-x86_64`: an Apple silicon Mac with Rosetta. Install it with `softwareupdate --install-rosetta`.
  `arch -x86_64 /usr/bin/true` succeeds once it works.

### Windows

- **Visual Studio 2022** (17.10 or newer), or its Build Tools, with the "Desktop development with C++" workload.
  Its MSVC 19.4x is the profile's `compiler.version=194`. Visual Studio 2026 ships MSVC 19.5x, which does not
  match, unless you add the VS 2022 (v143) toolset.
- Ninja, e.g. `winget install Ninja-build.Ninja`.
- Git for Windows and Python from [python.org](https://www.python.org/downloads/windows/).
- PowerShell 7 (`pwsh`). The Windows commands below are for `pwsh`.

Qt's build paths are long, and Windows still has a 260-character limit in places. Keep the Conan cache at a
short path, such as `C:\ch` (see `CONAN_HOME` below), and check out this repository at a short path too.

## Installing Conan

Install the exact Conan version `conanfile.py` pins into a virtual environment. Use a dedicated Conan home
(`CONAN_HOME`) for this checkout, so that nothing here mixes with any other Conan cache or configuration you
have. The steps below remove all Conan remotes, so they must not run in a Conan home you use elsewhere.

Linux and macOS, in the repository root:

```bash
version="$(sed -nE 's/^required_conan_version = "=([0-9]+\.[0-9]+\.[0-9]+)"$/\1/p' conanfile.py)"
python3 -m venv ~/.venvs/conan-owncloud
~/.venvs/conan-owncloud/bin/pip install "conan==$version"

export PATH="$HOME/.venvs/conan-owncloud/bin:$PATH"
export CONAN_HOME="$HOME/.conan-owncloud"
conan --version
```

Windows (`pwsh`), in the repository root:

```powershell
$version = (Select-String -Path conanfile.py -Pattern '^required_conan_version = "=([0-9.]+)"$').Matches[0].Groups[1].Value
py -3 -m venv C:\venvs\conan-owncloud
C:\venvs\conan-owncloud\Scripts\pip install "conan==$version"

$env:PATH = "C:\venvs\conan-owncloud\Scripts;$env:PATH"
$env:CONAN_HOME = 'C:\ch'
conan --version
```

Set `PATH` and `CONAN_HOME` like this in every new shell you build in. `python` is then the venv's Python. `PROFILE` below is the profile you build
for: `linux-x86_64`, `windows-x86_64`, `macos-armv8` or `macos-x86_64`.

## A: Prebuilt dependencies from ghcr.io

CI builds and tests the client against an archive of all the dependency packages, which our dependency
builder publishes for the Conan inputs of `master`: `ghcr.io/owncloud/client-deps:<profile>-<fingerprint>`. The fingerprint is a hash of the
Conan inputs listed above, so every combination of them has its own archive, about 150 MB. The archives are
public, and pulling them needs no login.

You need [ORAS](https://oras.land/docs/installation) to pull them. CI uses version 1.3.6. On macOS
`brew install oras` works; on Linux and Windows, download it from the
[ORAS releases](https://github.com/oras-project/oras/releases).

1. Pull the archive for your checkout's Conan inputs.

   Linux and macOS:

   ```bash
   PROFILE=macos-armv8   # or linux-x86_64, macos-x86_64
   fp="$(python conan/fingerprint.py --profile "$PROFILE" --root .)"
   oras pull --output /tmp/client-deps "ghcr.io/owncloud/client-deps:$PROFILE-$fp"
   ```

   Windows:

   ```powershell
   $PROFILE_NAME = 'windows-x86_64'
   $fp = (python conan/fingerprint.py --profile $PROFILE_NAME --root .).Trim()
   oras pull --output "$env:TEMP\client-deps" "ghcr.io/owncloud/client-deps:$PROFILE_NAME-$fp"
   ```

   If `oras` says `not found`, there is no archive for these Conan inputs. Either your branch changed any of
   them (then [build the dependencies from source](#b-building-the-dependencies-from-source)), or the archive
   for a newer `master` is not published yet. In the second case, use the `master` your branch is based on,
   or wait.

2. Restore it into your Conan cache, with no remotes. Everything has to come from the archive.

   ```bash
   conan remote remove "*"
   conan cache restore /tmp/client-deps/cache.tgz
   ```

   On Windows, restore `"$env:TEMP\client-deps\cache.tgz"` instead.

3. Install the dependencies and generate the CMake files into `build/`. `--build=never` makes Conan fail
   rather than build anything that the archive doesn't hold.

   Linux and macOS arm64:

   ```bash
   conan install . --lockfile=conan.lock -pr:a "conan/profiles/$PROFILE" --build=never \
     -g CMakeDeps -g CMakeToolchain --output-folder build \
     -c tools.cmake.cmaketoolchain:generator=Ninja
   ```

   Windows, with Conan's PowerShell launchers:

   ```powershell
   conan install . --lockfile=conan.lock -pr:a "conan/profiles/$PROFILE_NAME" --build=never `
     -g CMakeDeps -g CMakeToolchain --output-folder build `
     -c tools.cmake.cmaketoolchain:generator=Ninja `
     -c tools.env.virtualenv:powershell=pwsh
   ```

   macOS Intel, with the build profile its archive was built with:

   ```bash
   conan install . --lockfile=conan.lock \
     -pr:h conan/profiles/macos-x86_64 -pr:b conan/profiles/macos-armv8 --build=never \
     -g CMakeDeps -g CMakeToolchain --output-folder build \
     -c tools.cmake.cmaketoolchain:generator=Ninja \
     -c "&:tools.build.cross_building:cross_build=False" \
     -c "&:tools.cmake.cmaketoolchain:system_name="
   ```

   The last two lines build the client itself natively for x86_64, as Qt is (see the comment in
   `conan/profiles/macos-x86_64`). CMake then does not see a cross build, and `CMAKE_OSX_ARCHITECTURES` still
   selects x86_64.

Then continue with [Building and testing the client](#building-and-testing-the-client).

## B: Building the dependencies from source

Conan builds every package from the recipes at `conan/recipes-commit`. Expect an hour or more and tens of GB of
disk space, most of both for Qt.

1. Clone the recipes at the pinned commit, next to this repository. The checkout must stay clean, because
   local changes would give different recipe revisions than the ones in `conan.lock`.

   ```bash
   git clone https://github.com/owncloud/conan-center-index ../conan-center-index
   git -C ../conan-center-index checkout "$(cat conan/recipes-commit)"
   ```

   Windows: `git -C ..\conan-center-index checkout (Get-Content conan/recipes-commit)`.

2. Make that checkout the only remote:

   ```bash
   conan remote remove "*"
   conan remote add oc-recipes ../conan-center-index --type=local-recipes-index
   ```

3. Run the `conan install` from [step 3 of A](#a-prebuilt-dependencies-from-ghcrio) for your platform, with
   `--build=missing -r oc-recipes` instead of `--build=never`. For example, on Linux:

   ```bash
   conan install . --lockfile=conan.lock -pr:a "conan/profiles/$PROFILE" --build=missing -r oc-recipes \
     -g CMakeDeps -g CMakeToolchain --output-folder build \
     -c tools.cmake.cmaketoolchain:generator=Ninja
   ```

   Conan builds with all CPUs. Add `-c:a tools.build:jobs=4` to use fewer, e.g. when memory runs short.

The packages stay in your Conan cache, so the next `conan install` with the same Conan inputs builds nothing.

Notes:

- **macOS and Homebrew.** Qt's build looks for optional libraries and also finds them in `/opt/homebrew`.
  With Homebrew's webp, libtiff or jasper installed, Qt's image plugins link them from there. That client
  works on your Mac, but not on one without those Homebrew packages. The archives on ghcr.io are built on a
  Mac without Homebrew packages.
- **macOS Intel** builds Qt natively for x86_64 and runs its tools under Rosetta. Use Xcode 27.2 or newer:
  the linker of Xcode 26.4.1 fails on OpenSSL's x86_64 assembly in Qt's TLS plugin.
- **Windows**: keep `CONAN_HOME` short (see [Prerequisites](#windows)).

### Changing the dependencies

To add, remove or bump a dependency, change `conanfile.py` and regenerate the lock against the pinned recipes:

```bash
OC_RECIPES_DIR=../conan-center-index conan/update-lock.sh 'openssl/*'
```

See the header of [`conan/update-lock.sh`](conan/update-lock.sh) for its options. Commit `conanfile.py` and
`conan.lock` together. CI checks that the lock is what the script produces.

Any change to the Conan inputs needs new prebuilt archives, which the maintainers have to get built for the
pull request. Until then, the Conan build on your pull request fails with "no dependency binaries for this commit's
Conan inputs". Say in the pull request that it changes the dependencies.

## Building and testing the client

`conan install` wrote the CMake toolchain, a CMake preset `conan-release` and two environment scripts into
`build/`. `conanbuild` puts Conan's CMake on `PATH`. `conanrun` adds the shared libraries from the Conan cache,
Qt among them, which the client and the tests need at runtime.

Linux and macOS:

```bash
source build/conanbuild.sh
cmake --preset conan-release -DBUILD_TESTING=ON
cmake --build --preset conan-release

source build/conanrun.sh
ctest --preset conan-release --output-on-failure
```

Windows (`conanbuild.ps1` also sets up the MSVC environment):

```powershell
. build\conanbuild.ps1
cmake --preset conan-release -DBUILD_TESTING=ON
cmake --build --preset conan-release

. build\conanrun.ps1
ctest --preset conan-release --output-on-failure -j1
```

Useful CMake options: `-DWITH_AUTO_UPDATER=ON` builds the updater (Sparkle on macOS), as CI does;
`-DFORCE_ASSERTS=ON` keeps assertions on in the release build.

To run the client you built, source `conanrun` first, in the same shell:

- Linux: `build/bin/owncloud`
- macOS: `build/bin/owncloud.app/Contents/MacOS/owncloud`
- Windows: `build\bin\owncloud.exe`

Notes on the tests:

- On Linux, `AppConfigTest` writes `/etc/ownCloud/ownCloud.ini`. Give yourself that directory first:
  `sudo install -d -o "$(id -u)" -g "$(id -g)" /etc/ownCloud`.
- On Linux, `CredentialManagerTest` uses the Secret Service through D-Bus. With a session bus but no Secret
  Service running (e.g. no GNOME Keyring or KWallet), the keychain reports an error.
- On macOS, `CredentialManagerTest` uses your login keychain and `UtilityTest` registers a login item. Both
  need your normal desktop session, and can fail from a shell without one, e.g. over SSH.
- On Windows, CI runs the tests one at a time (`-j1`), as above.

## Troubleshooting

- **`Missing prebuilt package` with `--build=never`**: your profile settings don't produce the package IDs in
  the archive. Use the profile file unchanged and don't pass extra `-s` settings or options. If you pulled an
  archive for another checkout, pull again for this one.
- **`conan` complains about the version**: the venv has another Conan than `conanfile.py` pins. Install again
  as above.
- **A system package check fails on Linux** (`... is not installed` from a `*/system` recipe): install the apt
  packages listed under [Prerequisites](#linux).
- **The compiler doesn't match**: `xcrun clang --version` must say Apple clang 21. On Windows,
  `cl` (after `. build\conanbuild.ps1`) must print `Version 19.4x`.
- **Switching profiles or Conan inputs**: delete `build/` and run `conan install` again. A preset from another
  profile stays in `build/` otherwise.
