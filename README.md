Zix
===

Zix is a lightweight C library of portability wrappers and data structures.

Dependencies
------------

None, except the C standard library.

Building
--------

Zix is a straightforward collection of C headers and implementation files which
should be easy to build or incorporate into a project.

A [Meson][] build definition is included which can be used to do a proper
system installation with a `pkg-config` file, generate IDE projects, run the
tests, and so on.  For example, the library and tests can be built and run like
so:

    meson setup build
    cd build
    ninja test

See the [Meson documentation][] for more details on using Meson.

Usage
-----

The [headers](include/zix/) are reasonably well documented.  There is no
external documentation at this time.

 -- David Robillard <d@drobilla.net>

[Meson]: https://mesonbuild.com/

[Meson documentation]: https://mesonbuild.com/Quick-guide.html
