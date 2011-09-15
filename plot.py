#!/usr/bin/env python

import math
import os
import sys

from matplotlib import pyplot
from matplotlib.pyplot import *

pyplot.subplots_adjust(wspace=0.2, hspace=0.2)

n_plots = len(sys.argv) - 1
for i in range(n_plots):
    filename = sys.argv[i+1]
    file = open(filename, 'r')

    math.ceil
    pyplot.subplot(math.ceil(math.sqrt(n_plots)),
                   math.ceil(math.sqrt(n_plots)),
                   i + 1)
    pyplot.xlabel('# Elements')
    pyplot.ylabel('Time (s)')

    ns = []
    zix_tree_times = []
    zix_sorted_array_times = []
    glib_times = []
    for line in file:
        if line[0] == '#':
            continue;
        (n, zix_tree, zix_sorted_array, glib) = line.split()
        ns.append(int(n))
        zix_tree_times.append(float(zix_tree))
        zix_sorted_array_times.append(float(zix_sorted_array))
        glib_times.append(float(glib))
    file.close()

    matplotlib.pyplot.plot(ns, zix_tree_times, '-o', label='ZixTree')
    matplotlib.pyplot.plot(ns, zix_sorted_array_times, '-o', label='ZixSortedArray')
    matplotlib.pyplot.plot(ns, glib_times, '-x', label='GSequence')
    pyplot.legend(loc='upper left')
    pyplot.title(os.path.splitext(os.path.basename(filename))[0].title())

matplotlib.pyplot.show()
