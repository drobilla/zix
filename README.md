Zix
===

Zix is a lightweight C library of portability wrappers and data structures.

Components
----------

* Allocation

  * `ZixAllocator`: A customizable allocator.
    * `ZixBumpAllocator`: A simple realtime-safe "bump-pointer" allocator.

* Algorithms

  * `zix/digest.h`: Digest functions suitable for hashing arbitrary data.

* Data Structures

  * `ZixBTree`: A page-allocated B-tree.
  * `ZixHash`: An open-addressing hash table.
  * `ZixRing`: A lock-free realtime-safe ring buffer.
  * `ZixTree`: A binary search tree.

* Threading

  * `ZixSem`: A portable semaphore wrapper.
  * `ZixThread`: A portable thread wrapper.

* File System

  * `zix/filesystem.h`: Functions for working with filesystems.
  * `zix/path.h`: Functions for working with filesystem paths lexically.

* Environment

  * `zix/environment.h`: Function to expand shell-style variables in a string.

Platforms
---------

Zix is continually tested on:

  * Debian 12 (x86, x64, arm32, and arm64)
  * Fedora 43 (x64)
  * FreeBSD 15 (x64)
  * MacOS 15 (M2)
  * Node 20 (as wasm via emscripten 3.1.69)
  * Windows 10 (x64)

Dependencies
------------

None, except the C standard library, and some POSIX and platform-specific APIs
where necessary.

Documentation
-------------

  * [Installation Instructions](INSTALL.md)
  * [API reference](https://drobilla.codeberg.page/zix/doc/html/)
    * [Single page](https://drobilla.codeberg.page/zix/doc/singlehtml/)
  * [Test Coverage](https://drobilla.codeberg.page/zix/coverage/)

 -- David Robillard <d@drobilla.net>
