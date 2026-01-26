#!/bin/bash

# Usage: generate_fast.sh
# Optimized version of generate.sh using awk for faster processing.
# This script takes no parameters, reads no environment variables,
# and can be run from any directory, as long as it is in the expected
# location in the repo.

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd "$SCRIPT_DIR"

if [[ -n "${PS1:-}" ]]; then
  # if the shell is interactive, clean up any flotsam before analyzing
  git clean -ix
fi

# Ensure all sorting is ASCII-order consistently across platforms.
export LANG=C

rm -rf results
mkdir -p results/includes results/included_by

REPO_ROOT="$SCRIPT_DIR/../../.."
INCLUDES_FILE="$SCRIPT_DIR/results/rawincludes.txt"
PATHS_FILE="$SCRIPT_DIR/results/paths.txt"
LOOPS_FILE="$SCRIPT_DIR/results/loops.txt"
ORDERING_FILE="$SCRIPT_DIR/results/ordering.txt"

echo "Extracting raw includes..."
grep -r '^[ ]*#include.*/.*\.h' "$REPO_ROOT/include" "$REPO_ROOT/src" 2>/dev/null | \
    grep -v boost > "$INCLUDES_FILE"

echo "Building levelization paths with awk..."
awk -F: '
{
    # Extract the file path (before the colon)
    file = $1
    # Extract the include directive (after the colon)
    include = $2

    # Find the module from file path - look for include/ or src/ marker
    level = ""
    n = split(file, parts, "/")
    for (i = 1; i <= n; i++) {
        if (parts[i] == "include" || parts[i] == "src") {
            # Next two components are the module
            if (i + 2 <= n) {
                level = parts[i+1] "/" parts[i+2]
            } else if (i + 1 <= n) {
                level = parts[i+1] "/toplevel"
            }
            break
        }
    }
    if (level == "") next

    # If level ends with a file extension, use parent dir
    if (match(level, /\.[^\/]+$/)) {
        sub(/\/[^\/]+$/, "/toplevel", level)
    }
    gsub("/", ".", level)

    # Extract include path from the include directive
    # Remove everything before < or " and after > or "
    gsub(/.*["<]/, "", include)
    gsub(/[">].*/, "", include)

    # Get first two path components of include
    m = split(include, iparts, "/")
    if (m >= 2) {
        includelevel = iparts[1] "/" iparts[2]
    } else if (m >= 1) {
        includelevel = iparts[1] "/toplevel"
    } else {
        next
    }

    # If includelevel ends with a file extension, use parent dir
    if (match(includelevel, /\.[^\/]+$/)) {
        sub(/\/[^\/]+$/, "/toplevel", includelevel)
    }
    gsub("/", ".", includelevel)

    # Only output if different modules
    if (level != includelevel) {
        print level, includelevel
    }
}
' "$INCLUDES_FILE" | sort | uniq -c | sort -rn > "$PATHS_FILE"

echo "Building flat-file database..."
awk '
{
    count = $1
    level = $2
    include = $3
    
    # Write to includes/<level>
    print include, count >> ("results/includes/" level)
    
    # Write to included_by/<include>
    print level, count >> ("results/included_by/" include)
}
' "$PATHS_FILE"

echo "Searching for loops..."
# Use awk for O(n) cycle detection instead of O(n²) nested grep
awk '
BEGIN {
    # First pass: build adjacency map
}
{
    count = $1
    from = $2
    to = $3
    
    # Store edge: from -> to with count
    edges[from, to] = count
    
    # Track all modules
    modules[from] = 1
    modules[to] = 1
}
END {
    # Check for bidirectional edges (cycles)
    for (edge in edges) {
        split(edge, parts, SUBSEP)
        from = parts[1]
        to = parts[2]
        
        # Check if reverse edge exists
        if ((to, from) in edges) {
            # Avoid duplicate reporting (only report if from < to alphabetically)
            if (from < to) {
                fromCount = edges[from, to]
                toCount = edges[to, from]
                
                print "Loop:", from, to
                
                # Determine which direction is stronger
                diff = fromCount - toCount
                if (diff > 3) {
                    print "  " from " > " to "\n"
                } else if (diff < -3) {
                    print "  " to " > " from "\n"
                } else if (diff == 0) {
                    print "  " from " == " to "\n"
                } else {
                    print "  " from " ~= " to "\n"
                }
            }
        } else {
            # No cycle - record ordering
            print from " > " to >> "'"$ORDERING_FILE"'"
        }
    }
}
' "$PATHS_FILE" > "$LOOPS_FILE"

echo ""
echo "=== Ordering ==="
cat "$ORDERING_FILE" 2>/dev/null || true

echo ""
echo "=== Loops ==="
cat "$LOOPS_FILE"

echo ""
echo "Done. Results in $SCRIPT_DIR/results/"

