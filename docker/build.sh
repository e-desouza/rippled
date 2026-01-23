#!/bin/bash
# Helper script to build rippled in Docker
# 
# Usage:
#   ./docker/build.sh          # Release build
#   ./docker/build.sh debug    # Debug build
#   ./docker/build.sh test     # Run tests (requires prior build)
#   ./docker/build.sh shell    # Start interactive shell
#   ./docker/build.sh clean    # Clean build artifacts

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

# Parse command
COMMAND="${1:-release}"

case "$COMMAND" in
    release|build)
        echo "=== Building rippled (Release) ==="
        docker-compose -f docker/docker-compose.yml run --rm build
        ;;
    
    debug)
        echo "=== Building rippled (Debug) ==="
        docker-compose -f docker/docker-compose.yml run --rm build-debug
        ;;
    
    test)
        echo "=== Running unit tests ==="
        docker-compose -f docker/docker-compose.yml run --rm test
        ;;
    
    shell|dev)
        echo "=== Starting interactive development shell ==="
        docker-compose -f docker/docker-compose.yml run --rm dev
        ;;
    
    up)
        echo "=== Starting development environment in background ==="
        docker-compose -f docker/docker-compose.yml up -d dev
        echo "Container started. Connect with: docker exec -it rippled-dev bash"
        ;;
    
    down)
        echo "=== Stopping development environment ==="
        docker-compose -f docker/docker-compose.yml down
        ;;
    
    clean)
        echo "=== Cleaning build artifacts ==="
        rm -rf build .build
        echo "Build artifacts removed."
        ;;
    
    rebuild-image)
        echo "=== Rebuilding Docker image (no cache) ==="
        docker-compose -f docker/docker-compose.yml build --no-cache
        ;;
    
    *)
        echo "Usage: $0 {release|debug|test|shell|up|down|clean|rebuild-image}"
        echo ""
        echo "Commands:"
        echo "  release        Build rippled in Release mode (default)"
        echo "  debug          Build rippled in Debug mode"
        echo "  test           Run unit tests (requires prior build)"
        echo "  shell          Start interactive development shell"
        echo "  up             Start development container in background"
        echo "  down           Stop development container"
        echo "  clean          Remove build artifacts"
        echo "  rebuild-image  Rebuild Docker image without cache"
        exit 1
        ;;
esac

