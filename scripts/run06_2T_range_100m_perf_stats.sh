#!/bin/bash

echo -e "\nRunning tests for Schensted 2T and Range pattern with perf stats..."

perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000000 -trials 5 -pattern range -upper_limit 100000000 -algo 2T