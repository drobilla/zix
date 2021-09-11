#!/usr/bin/env python

# Copyright 2011-2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

"""
Benchmark Zix data structures.
"""

import os
import random
import string
import subprocess

# Benchmark trees

subprocess.call(["./tree_bench", "40000", "640000"])
subprocess.call(
    [
        "../scripts/plot.py",
        "tree_bench.svg",
        "tree_insert.txt",
        "tree_search.txt",
        "tree_iterate.txt",
        "tree_delete.txt",
    ]
)

# Benchmark dictionaries

FILENAME = "gibberish.txt"
CHARACTERS = string.ascii_letters + string.digits + " '."
WORDS = open("/usr/share/dict/words", "r").readlines()
SCHEMES = ["http", "https"]
DOMAINS = ["example.org", "drobilla.net", "lv2plug.in", "gitlab.com"]
NAMES = ["ns", "foo", "bar", "stuff", "things"]


def random_word():
    """Return a random word with ASCII letters and digits"""

    if random.randint(0, 1):
        # Generate a URI
        result = "%s://%s/" % (random.choice(SCHEMES), random.choice(DOMAINS))
        for _ in range(random.randrange(1, 6)):
            result += random.choice(NAMES) + "/"
    else:
        result = ""
        for _ in range(random.randrange(1, 7)):
            result += random.choice(WORDS).strip() + " "

        result += random.choice(WORDS).strip()

    return result


if not os.path.exists(FILENAME):
    print("Generating random text %s" % FILENAME)

    with open(FILENAME, "w") as out:
        for i in range(1 << 20):
            out.write(random_word())
            out.write("\n")

subprocess.call(["./dict_bench", "gibberish.txt"])
subprocess.call(
    [
        "../scripts/plot.py",
        "dict_bench.svg",
        "dict_insert.txt",
        "dict_search.txt",
    ]
)
