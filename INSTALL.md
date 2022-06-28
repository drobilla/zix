Installation Instructions
=========================

Prerequisites
-------------

To build from source, you will need:

 * A relatively modern C compiler (GCC, Clang, and MSVC are known to work).

 * [Meson](http://mesonbuild.com/), which depends on
   [Python](http://python.org/).

This is a brief overview of building this project with meson.  See the meson
documentation for more detailed information.

Configuration
-------------

The build is configured with the `setup` command, which creates a new build
directory with the given name:

    meson setup build

Some environment variables are read during `setup` and stored with the
configuration:

  * `CC`: Path to C compiler.
  * `CFLAGS`: C compiler options.
  * `LDFLAGS`: Linker options.

However, it is better to use meson options for configuration.  All options can
be inspected with the `configure` command from within the build directory:

    cd build
    meson configure

Options can be set by passing C-style "define" options to `configure`:

    meson configure -Dc_args="-march=native" -Dprefix="/opt/mypackage/"

Building
--------

From within a configured build directory, everything can be built with the
`compile` command:

    meson compile

Similarly, tests can be run with the `test` command:

    meson test

Meson can also generate a project for several popular IDEs, see the `backend`
option for details.

Installation
------------

A compiled project can be installed with the `install` command:

    meson install

You may need to acquire root permissions to install to a system-wide prefix.
The `DESTDIR` environment can be set during this command to add a path to the
installation prefix (which is useful for packaging):

    DESTDIR=/tmp/mypackage/ meson install

The `--destdir` option can be used for the same purpose:

    meson install --destdir=/tmp/mypackage/

Compiler Configuration
----------------------

Several standard environment variables can be used to control how compilers are
invoked:

 * `CC`:       Path to C compiler
 * `CFLAGS`:   C compiler options
 * `LDFLAGS`:  Linker options

The value of these environment variables is recorded during `meson setup`,
they have no effect at any other stage.

Note that there are also meson options that do the same thing as most of these
environment variables, they are supported for convenience and compatibility
with the conventions of other build systems.
