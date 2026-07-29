#!/bin/bash

echo -e "\nRunning tests for Range pattern..."

./test_lis -size 100000000 -trials 5 -pattern range -upper_limit 100000000 -algo all