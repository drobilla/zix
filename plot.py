#!/usr/bin/env python

import sys
import os

from matplotlib import pyplot
from matplotlib.pyplot import *

for i in range(len(sys.argv) - 1):
    filename = sys.argv[i+1]
    file = open(filename, 'r')

    pyplot.subplot(2, 1, i + 1)
    pyplot.xlabel('Number of Elements')
    pyplot.ylabel('Time (s)')

    ns = []
    zix_times = []
    glib_times = []
    for line in file:
        if line[0] == '#':
            continue;
        (n, zix, glib) = line.split()
        ns.append(int(n))
        zix_times.append(float(zix))
        glib_times.append(float(glib))
    file.close()

    matplotlib.pyplot.plot(ns, zix_times, '-o', label='ZixTree')
    matplotlib.pyplot.plot(ns, glib_times, '-x', label='GSequence')
    pyplot.legend(loc='upper left')
    pyplot.title(os.path.splitext(os.path.basename(filename))[0].title())

matplotlib.pyplot.show()
