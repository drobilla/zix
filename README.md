Zix
===

Zix is a lightweight C library of portability wrappers and data structures.

Components
----------

  * `ZixAllocator`: A customizable allocator.
    * `ZixBumpAllocator`: A simple realtime-safe "bump-pointer" allocator.
  * `ZixBTree`: A page-allocated B-tree.
  * `ZixHash`: An open-addressing hash table.
  * `ZixRing`: A lock-free realtime-safe ring buffer.
  * `ZixSem`: A portable semaphore wrapper.
  * `ZixThread`: A portable thread wrapper.
  * `ZixTree`: A binary search tree.

  * `zix/digest.h`: Digest functions suitable for hashing arbitrary data.
  * `zix/filesystem.h`: Functions for working with filesystems.
  * `zix/path.h`: Functions for working with filesystem paths lexically.

Platforms
---------

Zix is continually tested on:

  * GNU/Linux (x86, arm32, and arm64)
  * FreeBSD (x64)
  * MacOS (x64)
  * Node (as wasm via emscripten)

Dependencies
------------

None, except the C standard library.

Documentation
-------------

  * [Installation Instructions](INSTALL.md)
  * [API reference (single page)](https://drobilla.gitlab.io/zix/doc/singlehtml)
  * [API reference (paginated)](https://drobilla.gitlab.io/zix/doc/html)

 -- David Robillard <d@drobilla.net>
