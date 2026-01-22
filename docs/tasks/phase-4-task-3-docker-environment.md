# Phase 4 Task 4.3: Docker Development Environment

## Overview

### Problem Statement
The initial build of rippled takes **>30 minutes** due to:
1. Conan dependency downloads (~40+ packages from Conan Center and xrplf remote)
2. Compilation of dependencies from source (boost, grpc, protobuf, rocksdb, etc.)
3. Compilation of rippled itself (~15-20 minutes on typical hardware)

This creates a significant barrier for new developer onboarding and slows down development workflows.

### Success Criteria
- [ ] New developers can build rippled in **<5 minutes** with Docker
- [ ] Single command to start development environment
- [ ] VS Code Dev Container integration for seamless IDE experience
- [ ] Docker build works on both Linux and macOS
- [ ] Pre-cached Conan dependencies eliminate download/compile time

### Target Experience
```bash
# New developer workflow (goal: <5 minutes to first build)
git clone https://github.com/XRPLF/rippled.git
cd rippled
docker-compose up -d dev
docker exec -it rippled-dev cmake --build build
```

---

## Deep Code Analysis

### Current Build Process
From `BUILD.md` and `docs/build/environment.md`:

1. **Prerequisites Installation**
   - Python 3.11+
   - Conan 2.17+
   - CMake 3.22+
   - GCC 12+ / Clang 16+ / Apple Clang 16+

2. **Conan Setup**
   ```bash
   conan config install conan/profiles/ -tf $(conan config home)/profiles/
   conan remote add --index 0 xrplf https://conan.ripplex.io
   ```

3. **Build Steps**
   ```bash
   mkdir .build && cd .build
   conan install .. --output-folder . --build missing --settings build_type=Release
   cmake -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake \
         -DCMAKE_BUILD_TYPE=Release -Dxrpld=ON -Dtests=ON ..
   cmake --build .
   ```

### Complete Dependency List (from conanfile.py)

#### Direct Requirements
| Package | Version | Purpose |
|---------|---------|---------|
| boost | 1.88.0 | Core utilities, ASIO, Beast, JSON |
| openssl | 3.5.4 | TLS/Crypto |
| grpc | 1.72.0 | gRPC communication |
| protobuf | 6.32.1 | Protocol buffers |
| rocksdb | 10.5.1 | Database backend |
| date | 3.0.4 | Date/time handling |
| soci | 4.0.3 | Database abstraction |
| sqlite3 | 3.49.1 | SQLite database |
| lz4 | 1.10.0 | Compression |
| libarchive | 3.8.1 | Archive handling |
| secp256k1 | 0.7.0 | Cryptographic signatures |
| ed25519 | 2015.03 | EdDSA signatures |
| nudb | 2.0.9 | Key-value store |
| xxhash | 0.8.3 | Fast hashing |
| zlib | 1.3.1 | Compression |

#### Test Requirements
| Package | Version | Purpose |
|---------|---------|---------|
| gtest | 1.17.0 | Unit testing |

#### Transitive Dependencies (from conan.lock)
- snappy/1.1.10 (compression for RocksDB)
- re2/20230301 (regex for gRPC)
- abseil (C++ utilities for gRPC)
- c-ares (async DNS for gRPC)
- Many more (~40+ total packages)

### Compiler Requirements
| Compiler | Minimum Version |
|----------|-----------------|
| GCC | 12 |
| Clang | 16 |
| Apple Clang | 16 |
| MSVC | 19.44 |

### Existing Build Infrastructure
- **Conan Profiles**: `conan/profiles/default`, `conan/profiles/ci`, `conan/profiles/sanitizers`
- **GitHub Actions**: `.github/workflows/reusable-build-test-config.yml` - uses container images
- **Patched Recipes Remote**: `https://conan.ripplex.io` (xrplf remote)
- **No existing Docker/DevContainer configuration**

---

## Design Considerations

### Option A: Full Dockerfile with Pre-built Dependencies
**Pros:**
- Single image with everything pre-compiled
- Fastest developer experience after pull
- Simple to use

**Cons:**
- Large image size (estimated 8-15GB)
- Rebuild entire image for dependency updates
- Longer CI build times for image updates

### Option B: Docker Compose with Multi-stage Build
**Pros:**
- Layered approach - base + deps + dev environment
- Better cache utilization
- Can share base layers

**Cons:**
- More complex setup
- Still large total size

### Option C: VS Code Dev Container (.devcontainer/)
**Pros:**
- Best IDE integration
- Standard dev container format
- Works with GitHub Codespaces
- Can mount local source for live editing

**Cons:**
- Requires VS Code or compatible IDE
- May need separate solution for CLI users

### Recommended Approach: Option C + Option A Hybrid
1. **Primary**: VS Code Dev Container for IDE users
2. **Secondary**: Standalone Dockerfile for CLI/CI users
3. **Shared**: Multi-stage Dockerfile with cached Conan layer

---

## Implementation Plan

### Step 1: Create Base Dockerfile with Build Tools
**File**: `Dockerfile.dev`

```dockerfile
# Multi-stage build for rippled development
FROM debian:bookworm-slim AS base

# Install system dependencies
RUN apt-get update && apt-get install -y \
    gcc-12 g++-12 \
    python3-pip python-is-python3 python3-venv python3-dev \
    curl wget ca-certificates git \
    build-essential cmake ninja-build libc6-dev \
    && rm -rf /var/lib/apt/lists/*

# Set up compiler alternatives
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-12 999 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-12

# Install Conan
RUN pip install --break-system-packages conan

# Set up Conan
RUN conan profile detect
```

### Step 2: Add Conan Dependency Layer
**Strategy**: Pre-install all dependencies in a cached layer

```dockerfile
FROM base AS deps

WORKDIR /rippled-deps

# Copy only files needed for dependency resolution
COPY conanfile.py conan.lock ./
COPY conan/ ./conan/

# Configure Conan profiles and remotes
RUN conan config install conan/profiles/ -tf $(conan config home)/profiles/ && \
    conan remote add --index 0 xrplf https://conan.ripplex.io

# Pre-install all dependencies (cached layer)
RUN conan install . --output-folder build --build missing \
    --settings build_type=Release \
    --options='&:tests=True' \
    --options='&:xrpld=True'

# Also build Debug dependencies
RUN conan install . --output-folder build --build missing \
    --settings build_type=Debug \
    --options='&:tests=True' \
    --options='&:xrpld=True'
```

### Step 3: Create VS Code Dev Container
**File**: `.devcontainer/devcontainer.json`

```json
{
  "name": "rippled Development",
  "build": {
    "dockerfile": "../Dockerfile.dev",
    "target": "dev"
  },
  "customizations": {
    "vscode": {
      "extensions": [
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools",
        "twxs.cmake",
        "llvm-vs-code-extensions.vscode-clangd"
      ],
      "settings": {
        "cmake.configureOnOpen": true,
        "cmake.buildDirectory": "${workspaceFolder}/build",
        "cmake.configureSettings": {
          "CMAKE_TOOLCHAIN_FILE": "build/generators/conan_toolchain.cmake",
          "CMAKE_BUILD_TYPE": "Release",
          "xrpld": "ON",
          "tests": "ON"
        }
      }
    }
  },
  "workspaceMount": "source=${localWorkspaceFolder},target=/workspace,type=bind",
  "workspaceFolder": "/workspace",
  "postCreateCommand": "conan install . --output-folder build --build missing",
  "remoteUser": "vscode"
}
```

### Step 4: Create docker-compose.yml
**File**: `docker-compose.yml`

```yaml
version: '3.8'

services:
  dev:
    build:
      context: .
      dockerfile: Dockerfile.dev
      target: dev
    container_name: rippled-dev
    volumes:
      - .:/workspace:cached
      - conan-cache:/root/.conan2
    working_dir: /workspace
    stdin_open: true
    tty: true
    command: /bin/bash

  build:
    build:
      context: .
      dockerfile: Dockerfile.dev
      target: builder
    volumes:
      - .:/workspace:cached
      - conan-cache:/root/.conan2
    working_dir: /workspace
    command: |
      bash -c "
        conan install . --output-folder build --build missing --settings build_type=Release &&
        cmake -B build -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake \
              -DCMAKE_BUILD_TYPE=Release -Dxrpld=ON -Dtests=ON &&
        cmake --build build --parallel
      "

volumes:
  conan-cache:
```

### Step 5: Add Usage Documentation
**File**: `docs/DOCKER_DEVELOPMENT.md`

Contents should include:
- Prerequisites (Docker, Docker Compose)
- Quick start commands
- VS Code Dev Container usage
- Building with docker-compose
- Troubleshooting common issues

### Step 6: CI Integration (Optional)
- Consider using the Docker image in GitHub Actions
- Pre-build and push image to GitHub Container Registry
- Update `.github/workflows/reusable-build-test-config.yml` to use custom image

---

## Files to Create

| File | Purpose | Priority |
|------|---------|----------|
| `Dockerfile.dev` | Multi-stage development container | High |
| `.devcontainer/devcontainer.json` | VS Code Dev Container config | High |
| `docker-compose.yml` | Container orchestration | Medium |
| `docs/DOCKER_DEVELOPMENT.md` | Usage documentation | Medium |
| `.devcontainer/Dockerfile` | Optional separate devcontainer Dockerfile | Low |

---

## Risk Assessment

### Medium Risk: Docker Image Size
- **Issue**: Pre-compiled dependencies will result in large image (8-15GB)
- **Mitigation**:
  - Use multi-stage builds to separate build artifacts
  - Consider slim base images
  - Document that this is a trade-off for build speed

### Low Risk: Platform Compatibility
- **Issue**: Docker behavior differences between Linux and macOS
- **Mitigation**:
  - Test on both platforms
  - Use platform-agnostic base image (Debian)
  - Document any platform-specific considerations

### Low Risk: Conan Cache Invalidation
- **Issue**: Dependency version updates require image rebuild
- **Mitigation**:
  - Use lockfile (`conan.lock`) in Docker build
  - Implement automated image rebuild on dependency changes
  - Use volume mount for Conan cache during development

### Low Risk: Conan Remote Availability
- **Issue**: `https://conan.ripplex.io` must be accessible during build
- **Mitigation**:
  - Document fallback to export patched recipes manually
  - Consider caching recipes in the image

---

## Validation Criteria

### Developer Experience Tests
- [ ] New developer can build rippled in <10 minutes (including image pull)
- [ ] `docker-compose up dev` starts development environment
- [ ] VS Code Dev Container opens and IntelliSense works
- [ ] Incremental builds work correctly with mounted source

### Platform Tests
- [ ] Docker build succeeds on Linux (Ubuntu 22.04+)
- [ ] Docker build succeeds on macOS (Docker Desktop)
- [ ] Built rippled binary passes unit tests

### Integration Tests
- [ ] Conan dependencies install without errors
- [ ] CMake configuration succeeds
- [ ] Full build completes without errors
- [ ] `./xrpld --unittest` passes

---

## Estimated Effort

| Task | Estimate |
|------|----------|
| Base Dockerfile creation | 2-3 hours |
| Conan layer optimization | 3-4 hours |
| VS Code Dev Container setup | 1-2 hours |
| Docker Compose configuration | 1 hour |
| Documentation | 2 hours |
| Testing on Linux/macOS | 2-3 hours |
| **Total** | **11-15 hours** |

---

## References

- [BUILD.md](../../BUILD.md) - Current build instructions
- [docs/build/environment.md](../build/environment.md) - Environment setup guide
- [docs/build/conan.md](../build/conan.md) - Conan crash course
- [conanfile.py](../../conanfile.py) - Dependency definitions
- [VS Code Dev Containers](https://code.visualstudio.com/docs/devcontainers/containers)
- [GitHub Codespaces](https://docs.github.com/en/codespaces)

