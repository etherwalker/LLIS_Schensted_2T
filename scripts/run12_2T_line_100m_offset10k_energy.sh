#!/bin/bash

echo -e "\nRunning tests for Schensted 2T and Line pattern with perf energy..."

perf stat -a -e power/energy-pkg/./test_lis -size 100000000 -trials 5 -pattern line -offset 10000 -algo 2T