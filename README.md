# GitLite — A Minimal Git in C++

GitLite is a small C++ implementation of core Git plumbing. It initializes a repository and creates real Git objects (blobs, trees, commits) using the same storage layout and compression Git uses. It supports a subset of commands for working with the object database and the current working directory.

## Overview

- Stores objects under `.git/objects/<sha[:2]>/<sha[2:]>`.
- Uses OpenSSL SHA‑1 to hash object payloads.
- Uses zlib to compress/decompress object data.
- Encodes object payloads exactly like Git: `"<type> <size>\0<payload>"`.

## Supported Commands

These are implemented in `src/main.cpp`:

- `init`: Create a new `.git` directory with `objects`, `refs`, and `HEAD`.
- `hash-object -w <file>`: Create and store a blob from a file; prints the blob SHA‑1.
- `cat-file -p <sha>`: Decompress and print a blob’s content.
- `write-tree`: Snapshot the current directory (skipping `.git`) into a tree object; prints the tree SHA‑1.
- `ls-tree --name-only <tree_sha>`: List entry names stored in a tree object.
- `commit-tree <tree_sha> -p <parent_sha> -m <message>`: Create a commit object; prints the commit SHA‑1.

## Build & Run

This project uses CMake and vcpkg for dependencies (OpenSSL, zlib). The included script compiles and runs the binary.

### Prerequisites

- CMake 3.13+
- A C++23 compiler (e.g., Clang on macOS)
- vcpkg with OpenSSL and zlib; `VCPKG_ROOT` environment variable set

### Quick Start (Recommended: use a temp directory)

Testing writes to `.git/` in your current directory. To avoid damaging an existing repository, use a clean sandbox path.

```sh
mkdir -p /tmp/gitlite && cd /tmp/gitlite
/path/to/this/repo/your_program.sh init
```

You can add a convenience alias:

```sh
alias gitlite=/path/to/this/repo/your_program.sh
mkdir -p /tmp/gitlite && cd /tmp/gitlite
gitlite init
```

### Manual Build (optional)

```sh
cd /path/to/this/repo
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build ./build
./build/git init
```

## Usage Examples

Initialize a repository:

```sh
gitlite init
```

Create a blob from a file and print its SHA‑1:

```sh
echo "hello" > hello.txt
gitlite hash-object -w hello.txt
# → prints a 40‑char SHA‑1
```

Print blob content from the object database:

```sh
gitlite cat-file -p <blob_sha>
```

Snapshot the working directory as a tree:

```sh
gitlite write-tree
# → prints a tree SHA‑1
```

List names in a tree object:

```sh
gitlite ls-tree --name-only <tree_sha>
```

Create a commit object:

```sh
gitlite commit-tree <tree_sha> -p <parent_commit_sha> -m "My message"
# → prints a commit SHA‑1
```

## How It Works

- **Blob (`src/blob.cpp`)**: Reads file bytes and builds `blob <size>\0<data>`, hashes with SHA‑1, compresses with zlib, and writes to `.git/objects/`.
- **Tree (`src/tree.cpp`)**: Recursively scans the working directory (skips `.git`), turns files/subdirs into entries with modes (`100644`, `100755`, `120000`, `40000`), encodes entries as `mode SP name NUL <20‑byte raw hash>`, then wraps with `tree <size>\0<content>`, hashes, compresses, and stores.
- **Commit (`src/commit.cpp`)**: Builds a commit body with `tree`, optional `parent`, `author`, `committer` (name, email, timestamp, timezone), and message; wraps as `commit <size>\0<body>`, hashes, compresses, and stores.
- **Helpers (`src/helper.cpp`, `src/git_object.h`)**: SHA‑1, zlib (de)compression, hex↔byte conversions, and a tiny `GitObject` helper for locating object files.

## Notes & Limitations

- Focuses on core object plumbing; network protocols and index staging are out of scope.
- Blob printing via `cat-file -p` is supported; other object pretty‑printing is minimal.
- Executable detection (`100755`) uses the file’s executable bit; symlinks are encoded as `120000`.
- Sorting in trees is by entry name to match Git’s canonical ordering.

## Roadmap

- Cloning a public repository (planned)
- More `cat-file` modes and object introspection
- Basic branch refs and updating `HEAD`

## Troubleshooting

- If build fails, verify `VCPKG_ROOT` is set and vcpkg has OpenSSL and zlib.
- Always test in a throwaway directory to avoid modifying a real repo’s `.git`.
