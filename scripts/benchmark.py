#!/usr/bin/env python

"""
Benchmark Zix data structures.
"""

import os
import random
import string
import subprocess

os.chdir("build")

# Benchmark trees

subprocess.call(["benchmark/tree_bench", "40000", "640000"])
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
CHARACTERS = string.ascii_letters + string.digits


def random_word():
    """Return a random word with ASCII letters and digits"""

    wordlen = random.randrange(1, 64)
    word = ""
    for _ in range(wordlen):
        word += random.choice(CHARACTERS)

    return word


if not os.path.exists(FILENAME):
    print("Generating random text %s" % FILENAME)

    with open(FILENAME, "w") as out:
        for i in range(1 << 20):
            out.write(random_word() + "\n")

subprocess.call(["benchmark/dict_bench", "gibberish.txt"])
subprocess.call(
    [
        "../scripts/plot.py",
        "dict_bench.svg",
        "dict_insert.txt",
        "dict_search.txt",
    ]
)
