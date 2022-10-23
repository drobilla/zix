Zix
===

Zix is a lightweight C library of portability wrappers and data structures.

Components
----------

  * `ZixAllocator`: A customizable allocator.
    * `ZixBumpAllocator`: A simple realtime-safe "bump-pointer" allocator.
  * `ZixBitset`: A packed set of bits of arbitrary length.
  * `ZixBTree`: A page-allocated B-tree.
  * `ZixHash`: An open-addressing hash table.
  * `ZixRing`: A lock-free realtime-safe ring buffer.
  * `ZixSem`: A portable semaphore wrapper.
  * `ZixThread`: A portable thread wrapper.
  * `ZixTree`: A binary search tree.

  * `zix/digest.h`: Digest functions suitable for hashing arbitrary data.
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

Public interfaces are well-documented in the [headers](include/zix/).  There is
no external documentation at this time.

  * [Installation Instructions](INSTALL.md)

 -- David Robillard <d@drobilla.net>
