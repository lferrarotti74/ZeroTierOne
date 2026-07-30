# Central Controller Builds

NOTE: for ZeroTier, Inc Internal use only.  We do not support these builds for external use, nor do we guarantee this will work for anyone but us.

## Prerequisites

The controller builds with **CMake**. Most dependencies come from your platform's package
manager; the two version-sensitive libraries that aren't packaged consistently anywhere
(`opentelemetry-cpp` 1.27 and `google-cloud-cpp` 2.38) are pinned and built from source by
`scripts/bootstrap-deps.sh`. (Conda is no longer used.)

### macOS

```bash
brew bundle                       # deps from the Brewfile (cmake, ninja, protobuf, grpc, abseil, libpqxx, hiredis, ...)
rustup-init -y && rustup default stable
ZT_CONTROLLER_DEPS=1 scripts/bootstrap-deps.sh   # builds opentelemetry-cpp + google-cloud-cpp into ./.deps
```

### Linux (Debian trixie or newer)

```bash
sudo apt-get install -y \
    build-essential cmake ninja-build git pkg-config ca-certificates curl \
    libpqxx-dev libpq-dev libhiredis-dev libjemalloc-dev nlohmann-json3-dev \
    libabsl-dev libprotobuf-dev protobuf-compiler protobuf-compiler-grpc libgrpc++-dev \
    libgtest-dev libgmock-dev libssl-dev libcurl4-openssl-dev
# Rust via https://rustup.rs (pinned: rustup default 1.89.0), then:
ZT_CONTROLLER_DEPS=1 scripts/bootstrap-deps.sh   # builds opentelemetry-cpp + google-cloud-cpp into ./.deps
```

`scripts/bootstrap-deps.sh` prints the exact `-DCMAKE_PREFIX_PATH` to use when it finishes.

## Build the Central Controller Binary

The simplest path is the CMake presets (`cmake --list-presets` shows the ones for your OS):

```bash
cmake --preset macos-controller-release          # or: linux-controller-release
cmake --build --preset macos-controller-release
```

The presets set `-DZT1_CENTRAL_CONTROLLER=1`, the `.deps` prefix, and (on macOS) the Homebrew/
keg-only paths for you. For the daemon drop `-controller` (`macos-release` / `linux-release`), and use
the `-debug` variants for Debug builds. (macOS/Linux presets use Unix Makefiles, which are
single-config, so the build type is baked into the preset name; Windows uses VS multi-config.)
Equivalent manual invocation:

```bash
# macOS: include the Homebrew prefix and the keg-only libpq prefix.
PREFIX="$PWD/.deps;$(brew --prefix);$(brew --prefix libpq)"
# Linux: PREFIX="$PWD/.deps"

cmake -DCMAKE_BUILD_TYPE=Release -DZT1_CENTRAL_CONTROLLER=1 \
      -DCMAKE_PREFIX_PATH="$PREFIX" -S . -B build
cmake --build build -j8
```

The binary is `build/zerotier-one` (manual) or `build-<preset>/zerotier-one` (preset; single-config,
so no `Release/` subfolder on macOS/Linux — that subfolder only exists for the Windows VS presets).

## Packaging via Docker

`ext/central-controller-docker/Dockerfile` builds a conda-free image on Debian trixie
(apt deps + `scripts/bootstrap-deps.sh` + the CMake build, in a multi-stage builder/runtime):

```bash
docker build -f ext/central-controller-docker/Dockerfile -t ztcentral-controller .
```

Official multi-arch images are produced by GitHub Actions.

## Configuration

Central Controller has new configuration options outside of the normal "settings" block of `local.conf`.

```json
{
  "settings": { 
    ...standard zt1 local.conf settings... 
  },
  "controller": {
    "listenMode": (pgsql|redis|pubsub),
    "statusMode": (pgsql|redis|bigtable),
    "redis": {
      "hostname": ...,
      "port": 6379,
      "clusterMode": true
    },
    "pubsub": {
      "project_id": <gcp-project-id>
    },
    "bigtable": {
      "project_id": <gcp-project-id>,
      "instance_id": <bigtable-instance-id>,
      "table_id": <bigtable-table-id>
    }
  }
}
```

Configuration checks for invalid configurations like `listenMode = "pubsub"`, but without a `"pubsub"` config block.
