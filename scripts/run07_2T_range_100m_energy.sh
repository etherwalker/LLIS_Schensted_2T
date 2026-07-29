#!/bin/bash

echo -e "\nRunning tests for Schensted 2T and Range pattern with perf energy..."

perf stat -a -e power/energy-pkg/ ./test_lis -size 100000000 -trials 5 -pattern range -upper_limit 100000000 -algo 2T