#!/bin/sh

./build/fuzz/btree_fuzz_gen 1 100 fuzzin/btree_1_100
./build/fuzz/btree_fuzz_gen 2 500 fuzzin/btree_2_500
./build/fuzz/btree_fuzz_gen 3 1000 fuzzin/btree_3_1000
./build/fuzz/btree_fuzz_gen 4 1500 fuzzin/btree_4_1500
