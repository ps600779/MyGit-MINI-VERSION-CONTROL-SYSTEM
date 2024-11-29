# MyGit

MyGit is a simple implementation of a version control system in C++. It provides basic functionalities similar to Git, such as initializing a repository, hashing objects, committing changes, logging history, and checking out specific commits. This project is designed for educational purposes to understand the underlying mechanics of version control systems.

## Table of Contents

- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
  - [Commands](#commands)
- [File Structure](#file-structure)
- [Dependencies](#dependencies)

## Features

- Initialize a new repository
- Hash files and store them as objects
- Commit changes with messages
- Log commit history
- Checkout previous commits
- Support for adding files and directories
- View and retrieve file and tree objects

## Installation

To use MyGit, you need to have a C++ compiler and the following libraries installed on your system:

- **CommonCrypto** (for SHA-1 hashing)
- **zlib** (for compression)

### Steps to Install

1. Clone the repository or download the source code.
2. Navigate to the project directory.
3. Compile the code using a C++ compiler (e.g., g++ or clang++):
   ```bash
   g++ -o mygit main.cpp -lCommonCrypto -lz
4. Run the compiled executable.
Usage
To use MyGit, run the executable followed by the desired command and options. The basic syntax is:

```
./mygit <command> [options]
```
## Commands
- ### init

- Initializes a new MyGit repository in the current directory.
```
./mygit init
```
- ### hash-object

- Computes the SHA-1 hash of a file. Optionally write the object to the object store.
```
./mygit hash-object <file> [-w]
```
- ### cat-file

- Displays information about an object.
```
./mygit cat-file <flag> <file_sha>
```
- Flags:

- -p: Print the contents of the object.
- -s: Show the size of the object.
- -t: Show the type of the object.
- ### write-tree

Writes the current directory tree to the object store.
```
./mygit write-tree
```
- ### ls-tree

- Lists the contents of a tree object.
```
./mygit ls-tree <tree_sha> [--name-only]
```
- ### add

- Stages files for the next commit. Supports adding directories recursively.
```
./mygit add <file_or_directory>
```
- ### commit

- Commits the staged changes with an optional commit message.
```
./mygit commit [-m <message>]
```
- ### log

- Displays the commit history.
```
./mygit log
```
- ### checkout

- Checks out a specific commit.
```
./mygit checkout <commit_sha>
```
File Structure
```
.mygit/
├── HEAD            # Points to the current commit
├── objects/        # Stores object files (blobs and trees)
└── commits/        # Stores commit information
```
## Dependencies
- CommonCrypto: For SHA-1 hashing. (Available on macOS)
- zlib: For data compression. (Install via package manager or source)
