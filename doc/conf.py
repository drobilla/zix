# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Project information

project = "Zix"
copyright = "2011-2022, David Robillard"
author = "David Robillard"
release = "dev"
desc = "A lightweight C library of portability wrappers and data structures"

# General configuration

exclude_patterns = ["xml"]
language = "en"
nitpicky = True
pygments_style = "friendly"

# Ignore everything opaque or external for nitpicky mode
_opaque = [
    "FILE",
    "ZixAllocator",
    "ZixAllocatorImpl",
    "ZixBTree",
    "ZixBTreeImpl",
    "ZixBTreeNode",
    "ZixBTreeNodeImpl",
    "ZixHash",
    "ZixHashImpl",
    "ZixRing",
    "ZixRingImpl",
    "ZixSem",
    "ZixSemImpl",
    "ZixTree",
    "ZixTreeImpl",
    "ZixTreeIter",
    "ZixTreeNode",
    "ZixTreeNodeImpl",
    "int64_t",
    "pthread_t",
    "ptrdiff_t",
    "size_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
    "uint8_t",
]

_c_nitpick_ignore = map(lambda x: ("c:identifier", x), _opaque)
_cpp_nitpick_ignore = map(lambda x: ("cpp:identifier", x), _opaque)
nitpick_ignore = list(_c_nitpick_ignore) + list(_cpp_nitpick_ignore)

# HTML output

html_copy_source = False
html_short_title = "Zix"
html_theme = "sphinx_lv2_theme"

if tags.has("singlehtml"):
    html_sidebars = {
        "**": [
            "globaltoc.html",
        ]
    }

    html_theme_options = {
        "body_max_width": "48em",
        "body_min_width": "48em",
        "description": desc,
        "show_footer_version": True,
        "show_logo_version": True,
        "logo_name": True,
        "logo_width": "8em",
        "nosidebar": False,
        "page_width": "80em",
        "sidebar_width": "18em",
        "globaltoc_maxdepth": 3,
        "globaltoc_collapse": False,
    }

else:
    html_theme_options = {
        "body_max_width": "60em",
        "body_min_width": "40em",
        "description": desc,
        "show_footer_version": True,
        "show_logo_version": True,
        "logo_name": True,
        "logo_width": "8em",
        "nosidebar": True,
        "page_width": "60em",
        "sidebar_width": "14em",
        "globaltoc_maxdepth": 1,
        "globaltoc_collapse": True,
    }
