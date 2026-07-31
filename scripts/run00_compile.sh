#!/bin/bash

g++ -O3 -march=native -std=c++11 -Wall -Wextra -Wconversion -pedantic ../src/test_lis.cpp -pthread -o test_lis