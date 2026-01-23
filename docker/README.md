# Docker Development Environment for rippled

This directory contains Docker configuration files for building and developing rippled in a containerized environment.

## Prerequisites

- [Docker](https://docs.docker.com/get-docker/) (20.10 or later)
- [Docker Compose](https://docs.docker.com/compose/install/) (v2.0 or later)

## Quick Start

### Using the Helper Script

The simplest way to build rippled:

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

### Using Docker Compose Directly

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

## VS Code Dev Container

For the best development experience, open this project in VS Code with the Dev Containers extension:

1. Install the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
2. Open the rippled repository in VS Code
3. Click "Reopen in Container" when prompted (or use Command Palette: "Dev Containers: Reopen in Container")
4. Wait for the container to build (first time may take a while)

Once inside the container, run:

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

## Services

| Service | Description |
|---------|-------------|
| `dev` | Interactive development environment with pre-configured tools |
| `build` | One-shot Release build |
| `build-debug` | One-shot Debug build |
| `test` | Run unit tests (requires prior build) |

## Volumes

The Docker Compose configuration uses named volumes for Conan cache persistence:

- `conan-cache`: Conan cache for the developer user
- `conan-cache-root`: Conan cache for root user (build services)

This ensures dependencies don't need to be re-downloaded between container restarts.

## Troubleshooting

### Build Fails with Conan Remote Error

If you see errors about missing packages from the `xrplf` remote:

```bash
# Inside the container
conan remote add --index 0 xrplf https://conan.ripplex.io
```

### Slow First Build

The first build downloads and compiles all Conan dependencies, which can take 30+ minutes. Subsequent builds use cached dependencies and are much faster.

### Permission Issues

If you encounter permission issues with mounted volumes:

```bash
# Fix ownership of build directory
sudo chown -R $(id -u):$(id -g) build/
```

### Clean Rebuild

To start fresh:

```bash
# Remove build artifacts
./docker/build.sh clean

# Rebuild Docker image without cache
./docker/build.sh rebuild-image
```

## Architecture

The Dockerfile uses a multi-stage build:

1. **base**: System dependencies and Conan installation
2. **deps**: Pre-built Conan dependencies (cached layer)
3. **dev**: Development environment with non-root user
4. **builder**: CI/CD-ready build environment

This architecture allows for:
- Fast image rebuilds when only code changes
- Shared base layers between stages
- Separation of development and CI concerns

