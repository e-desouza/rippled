# Docker Development Environment

This document provides a quick reference for using Docker to build and develop rippled.
For complete documentation, see [`docker/README.md`](../docker/README.md).

## Prerequisites

- [Docker](https://docs.docker.com/get-docker/) (20.10 or later)
- [Docker Compose](https://docs.docker.com/compose/install/) (v2.0 or later)

## Quick Start

### Option 1: Using the Helper Script (Recommended)

```bash
# Release build
./docker/build.sh

# Debug build
./docker/build.sh debug

# Run tests (after building)
./docker/build.sh test

# Start interactive shell
./docker/build.sh shell
```

### Option 2: Using Docker Compose

```bash
# Build rippled (Release)
docker-compose -f docker/docker-compose.yml run --rm build

# Build rippled (Debug)
docker-compose -f docker/docker-compose.yml run --rm build-debug

# Run unit tests
docker-compose -f docker/docker-compose.yml run --rm test

# Start interactive development environment
docker-compose -f docker/docker-compose.yml run --rm dev
```

### Option 3: VS Code Dev Container

1. Install the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
2. Open the rippled repository in VS Code
3. Click "Reopen in Container" when prompted
4. Wait for the container to build (first time may take a while)

Once inside the container, build with:

```bash
# Install Conan dependencies
conan install . --output-folder build --build missing \
  --settings build_type=Release \
  --options='&:tests=True' \
  --options='&:xrpld=True'

# Configure CMake
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -Dxrpld=ON -Dtests=ON

# Build
cmake --build build --parallel

# Run tests
./build/xrpld --unittest --unittest-jobs 2
```

## Docker Services

| Service | Description |
|---------|-------------|
| `dev` | Interactive development environment with pre-configured tools |
| `build` | One-shot Release build |
| `build-debug` | One-shot Debug build |
| `test` | Run unit tests (requires prior build) |

## Architecture Overview

The Docker setup uses a multi-stage build strategy:

1. **base**: System dependencies and Conan installation
2. **deps**: Pre-built Conan dependencies (cached layer)
3. **dev**: Development environment with non-root user
4. **builder**: CI/CD-ready build environment

This architecture provides:
- Fast image rebuilds when only code changes
- Shared base layers between stages
- Separation of development and CI concerns

## Key Files

| File | Purpose |
|------|---------|
| `docker/Dockerfile.dev` | Multi-stage development container |
| `docker/docker-compose.yml` | Container orchestration |
| `docker/build.sh` | Helper script for common operations |
| `docker/README.md` | Complete Docker documentation |
| `.devcontainer/devcontainer.json` | VS Code Dev Container configuration |

## Troubleshooting

### Slow First Build

The first build downloads and compiles all Conan dependencies, which can take 30+ minutes. Subsequent builds use cached dependencies and are much faster.

### Permission Issues

If you encounter permission issues with mounted volumes:

```bash
sudo chown -R $(id -u):$(id -g) build/
```

### Clean Rebuild

```bash
# Remove build artifacts
./docker/build.sh clean

# Rebuild Docker image without cache
./docker/build.sh rebuild-image
```

For more troubleshooting tips, see [`docker/README.md`](../docker/README.md).

## Related Documentation

- [BUILD.md](../BUILD.md) - Standard build instructions
- [docs/build/environment.md](build/environment.md) - Environment setup guide
- [docs/build/conan.md](build/conan.md) - Conan crash course

