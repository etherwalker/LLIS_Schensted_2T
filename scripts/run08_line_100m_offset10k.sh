#!/bin/bash

echo -e "\nRunning tests for Line pattern..."

./test_lis -size 100000000 -trials 5 -pattern line -offset 10000 -algo all