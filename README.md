# Schensted’s Algorithm for LIS on Two Threads
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Language](https://img.shields.io/badge/C%2B%2B-11%2B-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%2F%20x86__64-lightgrey.svg)
![Architecture](https://img.shields.io/badge/Architecture-Lock--Free-success.svg)
[![DOI](https://zenodo.org/badge/1315429849.svg)](https://doi.org/10.5281/zenodo.21681413)

## Quick Start

```bash
git clone git@github.com:etherwalker/LLIS_Schensted_2T.git
#git clone https://github.com/etherwalker/LLIS_Schensted_2T.git
cd LLIS_Schensted_2T/scripts
chmod +x *.sh
./run00_compile.sh
./run01_enable_performance_mode.sh
./run03_range_100m.sh
./run08_line_100m_offset10k.sh
cat input_data_one_integer_per_line.txt | ./test_lis -size 100000 -trials 10 -pattern stdin

```

## Overview

The parallelization of the Longest Increasing Subsequence (LIS) problem has remained a significant challenge for decades due to:
* Inherent data dependencies that create a highly dense dependency graph.
* The existence of [Schensted's highly optimized, textbook sequential algorithm](https://link.springer.com/chapter/10.1007/978-0-8176-4842-8_21), [as presented by Fredman and Knuth](https://www.sciencedirect.com/science/article/pii/0012365X7590103X?via%3Dihub), bounded by $O(n \log k)$ work.

The modern parallel state-of-the-art (SOTA) approach by [Gu et al. (2023)](https://doi.org/10.1145/3558481.3591069) [(GitHub)](https://github.com/ucrparlay/Parallel-LIS):
* Achieves up to 16x speedup by employing large-scale parallelism across tens of threads (tested on a 192-thread / 96-core computer).
* Requires 8 to 16 cores to offset architectural overhead and become faster than Schensted's sequential algorithm.
* Encounters parallelism limitations on data with bounded disorder: for large LIS lengths ($k$), their span of $O(k)$ reduces available parallelism, making their 192-thread execution slower than the single-threaded sequential baseline.
* According to the authors, prior to their work, they were not aware of any parallel LIS algorithm implementation competitive with the highly optimized Schensted’s algorithm in practice.

In this work, we diverge from large-scale parallelism models and evaluate the performance of a "Lean Engineering" dual-thread implementation of Schensted's algorithm (Schensted 2T).
By leveraging a symmetric architecture and a completely lock-free $O(k)$ merge step, we experimentally evaluated the algorithm on the exact benchmarking standards (range and line data patterns) established by Gu et al. 

Measurements demonstrate that Schensted 2T:
* Achieves up to linear speedup utilizing just two threads.
* Remains faster than the classic Schensted's algorithm even for extremely large LIS lengths—the scenarios where current parallel SOTA approaches experience scaling bottlenecks.
* Achieves a substantial reduction in the Energy-Delay Product (EDP)—by up to 62.7%—compared to the classic Schensted's algorithm.
* Delivers these results entirely on consumer-grade, energy-constrained commodity hardware (e.g., AMD Ryzen Mobile 15W).

Perf stats reveal that the efficiency of Schensted 2T is grounded on specific micro-architectural utilization:

* Thread-Level to Memory-Level Parallelism (TLP & MLP): The architecture utilizes TLP to issue independent search queries, which in turn unlocks Memory-Level Parallelism (MLP). Even in patterns where total frontend stalled cycles remain constant or slightly increase due to merge-step overhead (e.g., Line pattern with high offset), the absolute execution time is nearly halved. This proves that independent memory stalls are effectively overlapped (latency hiding), accelerating an inherently latency-bound problem without requiring an increase in total memory bandwidth.
* L1 Cache Pressure Mitigation: Slicing the search space per thread significantly reduces the working set size, leading to a measurable ~35% drop in `L1-dcache-load-misses` (as verified by perf stats on the Range pattern).

In the following sections, we present:
* The Schensted 2T implementation and its symmetric $O(k)$ merge logic.
* The experimental setup.
* The performance measurements against the fully optimized 1T baseline.
* Micro-architectural and energy metrics provided by hardware perf stats.
* Reproducibility instructions.

---

## Core Algorithm Implementation (C++)

The core idea of Schensted 2T relies on a single-split partitioning of the input sequence into two halves, processed concurrently by two independent threads.

Let the input sequence $A$ of length $n$ be partitioned into two contiguous halves, $A_{left}$ and $A_{right}$. The first thread processes $A_{left}$ and computes the standard `min_tails` array of Schensted's algorithm. Simultaneously, the second thread processes $A_{right}$ in reverse to compute an analogous `max_heads` array. While `min_tails[i]` stores the minimum possible tail for an increasing subsequence of length $i$ within the first half $A_{left}$, the `max_heads[j]` symmetrically stores the maximum possible starting element (head) for a valid increasing subsequence of length $j$ within the second half $A_{right}$.

After the parallel execution finishes, we possess two arrays: `min_tails` (strictly increasing) from the left half and `max_heads` (strictly decreasing) from the right half. 

Consider any optimal global LIS of the entire sequence `A`. There are three possible spatial distributions for this subsequence:

1. It is entirely contained within the first half, `A_left`.
2. It is entirely contained within the second half, `A_right`.
3. It spans across both halves, meaning a prefix of the LIS lies in `A_left` and the remaining suffix lies in `A_right`.

The maximum lengths of the first two cases are trivially given by the sizes of the `min_tails` and `max_heads` arrays, respectively. The non-trivial scenario is the third one, for which the global LIS is determined by finding the maximum total length $k = i + j$, such that the $i$-th element of the first half precedes the $j$-th element of the second half:

$$k = \max \\{ i + j \mid \text{min}\\_\\text{tails}[i] < \text{max}\\_\\text{heads}[j] \\}$$

This can be achieved elegantly and efficiently via a merge-like process.

The following code snippet demonstrates the core logic of Schensted's 2T $O(k)$ merge process. The full implementation, including the data generation step, can be found in `test_lis.cpp`.

```cpp
if ( min_tails[left_lis_len] < max_heads[right_lis_len] )
{
  lis_len = left_lis_len + right_lis_len;
}
else
{
  lis_len = max( left_lis_len, right_lis_len );

  i = 1;
  j = right_lis_len;
  while ( i <= left_lis_len && j >= 1 )
  {
    if ( min_tails[i] < max_heads[j] )
      i++;
    else
    {
      lis_len = max( i-1 + j, lis_len );
      j--;
    }
  }

  lis_len = max( i-1 + j, lis_len );
}
```

The provided implementation and performance benchmarks focus on computing the exact length of the LIS. However, the Schensted 2T algorithm can be trivially extended to reconstruct the actual sequence elements. By maintaining a standard predecessor (parent-pointer) array during the independent thread executions and tracking the optimal crossover indices during the lock-free $O(k)$ merge step, the global LIS can be fully recovered via a simple linear backtracking pass.

---

## Experimental Setup

### System Specifications (Hardware & Software)

All micro-architectural benchmarks and energy metrics (EDP) presented in this repository were conducted on a strict power-constrained mobile SoC.

| CPU |  |
| :--- | :--- |
| Number of cores | 4 (max 4) |
| Number of threads |	8 (max 8) |
| Name | AMD Ryzen 3 Mobile 5300U |
| Codename | Lucienne |
| Specification | AMD Ryzen 3 5300U with Radeon Graphics @ 2.60GHz |
| Package | Socket FP6 |
| Technology | 7 nm |
| Instructions sets |	MMX (+), SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, SSE4A, x86-64, AES, AVX, AVX2, FMA3, SHA |
| Turbo Mode | supported, enabled |
| Max turbo frequency | 3.80 MHz |

| Motherboard & System | |
| :--- | :--- |
| System Model | Lenovo ThinkPad E15 Gen 3 |
| Motherboard Model | Lenovo 20YHS0GD00 |
| Chipset | AMD Ryzen SOC |
| BIOS Vendor | LENOVO |
| BIOS Version | R1OET35W (1.14) |
| BIOS Date | 11/30/2022 |

| Cache |  |
| :--- | :--- |
| L1 Data cache | 4 x 32 KB (8-way, 64-byte line) |
| L1 Instruction cache | 4 x 32 KB (8-way, 64-byte line) |
| L2 cache | 4 x 512 KB (8-way, 64-byte line) |
| L3 cache | 4 MB (16-way, 64-byte line) |

| RAM |  |
| :--- | :--- |
| Memory Type |	DDR4 |
| Memory Size | 8 GBytes (6.75 for use) |
| Channels | Single |
| Memory Frequency | 1596.1 MHz (1:16) |
| CAS# latency (CL) | 22.0 |
| RAS# to CAS# delay (tRCD) | 22 |
| RAS# Precharge (tRP) | 22 |
| Cycle Time (tRAS) | 52 |
| Bank Cycle Time (tRC) | 74 |

| Operating system and compiler |  |
| :--- | :--- |
| OS | Ubuntu 22.04 |
| Language | c++ 11 |
| Compiler | g++ (Ubuntu 13.1.0-8ubuntu1~22.04) |
| Compilation flags | -O3 -march=native -Wall -Wextra -Wconversion -pedantic |
| Data type | long long


### Input Datasets & Benchmarking Protocol

To establish a direct and reproducible benchmark against current SOTA massive-parallelism frameworks, we incorporated the exact synthetic data generators proposed by Gu et al., specifically evaluating the algorithms against their Range Pattern and Line Pattern distributions. All elements within the generated sequences are strictly represented as 64-bit integers (long long). For each data sequence, five consecutive trials were performed to eliminate potential Operating System noise and ensure statistical significance. The execution times are  both printed to the standard output and systematically logged into an output file.

---


## Workload Characterization, Benchmarks & Micro-architectural Analysis

### High Entropy Workloads (Range Pattern)

#### 1. Pattern Description
To benchmark our proposed 2-thread architecture against the current SOTA in a highly controlled environment, we employ the *Range Pattern* synthetic data generator introduced by Gu *et al.* Their full source code is available on GitHub. The $i$-th element of the sequence of length $n$ is deterministically generated using the formula:

$$A[i] = \text{hash64}(i + \text{seed} \times n) \pmod{\mathtt{upper\_limit}}$$

The application of the modulo operator mathematically bounds the alphabet size of the generated sequence to the interval $[0,$ `upper_limit` $- 1]$. Since a valid LIS must consist of *strictly* increasing elements, it inherently cannot contain duplicate values. Consequently, the length of the optimal solution, $k$, is tightly constrained by the available alphabet, yielding $k \le$ `upper_limit`.

#### 2. Performance Graph
<p align="left">
  <img src="/results/ryzen3_5300U/plots/range_pattern_100000000_100000000.jpg" alt="Performance Comparison: Range Pattern" width="600"/>
  <br>
  <em><b>Figure 1:</b> Execution time and speedup comparison for the Range pattern (N=10<sup>8</sup>).</em>
</p>

#### 3. Hardware Performance Counters
Methodological note on Hardware Profiling:
The hardware performance counters (`perf stats`) and energy consumption metrics presented in the table below reflect the execution of the *entire program binary* (e.g., `./test_lis`), rather than isolating strictly the Region-of-Interest (ROI) of the LLIS computation algorithms. Consequently, the reported metrics—such as total cycles, instructions, cache misses, and Energy-Delay Product (EDP)—inclusively encompass the computational and memory overhead associated with the synthetic data generation and initial array allocation phases.

**Table 1: Hardware Performance Counters and Energy Metrics for the Sequential Schensted (1T) and the Schensted (2T) executing the Range pattern ($n=10^8$) on the AMD Ryzen 3 5300U (Ubuntu) platform.**

| Micro-architectural Metric | Schensted 1T | Schensted 2T |
| :--- | ---: | ---: |
| **Cycles** | 818,008,807,286 | 790,777,943,039 |
| **Instructions** | 528,615,232,134 | 517,944,709,108 |
| **Stalled Cycles (Frontend)** | 431,053,663,929 | 422,829,739,842 |
| **Branches** | 125,855,357,727 | 123,030,759,623 |
| **Branch Misses** | 24,168,918,751 | 23,452,994,275 |
| **Cache References** | 34,172,584,058 | 23,373,841,130 |
| **Cache Misses** | 69,718,002 | 70,773,450 |
| **L1 D-Cache Load Misses** | 24,995,242,376 | 16,281,426,364 |
| **Execution Time (s)** | 211.88 | 107.50 |
| **Energy Consumed (J)** | 919.76 | 675.35 |
| **Energy-Delay Product (J·s)** | **194,876.27** | **72,596.98** |

#### 4. Micro-architectural Conclusion: Latency Hiding, MLP, and L1 Cache Pressure Mitigation

In scenarios with maximum data entropy (e.g., the Range pattern), the binary search mechanism performs unpredictable, non-sequential memory jumps that continuously overflow the L1 Data Cache capacity, rendering the algorithm inherently latency-bound rather than compute-bound. The efficiency of the Schensted 2T architecture is grounded on two synergistic micro-architectural utilizations:

*   Thread-Level to Memory-Level Parallelism (TLP & MLP): The architecture utilizes TLP to issue independent search queries, which in turn unlocks Memory-Level Parallelism (MLP). By effectively overlapping independent memory stalls (latency hiding), the dual-thread execution accelerates this latency-bound problem, achieving almost linear speedup without requiring an increase in total memory bandwidth.
*   L1 Cache Pressure Mitigation: Slicing the search space per thread significantly reduces the active working set size per core. As verified by the hardware performance counters, this partitioning leads to a massive ~34.8% drop in `L1-dcache-load-misses` (dropping from 24.99 billion down to 16.28 billion).

This synergistic combination of MLP (stall overlap) and L1 Cache Pressure Mitigation mathematically explains the architecture's almost linear speedup. Consequently, this profound architectural efficiency allows the processor to complete the workload rapidly and transition into deep idle C-states ("Race to Sleep"). This directly yields the dramatic 62.7% reduction in the Energy-Delay Product (EDP) when processing high-entropy input patterns.


### Bounded Disorder Workloads (Line Pattern)

#### 1. Pattern Description
To evaluate the algorithm's behavior under conditions of bounded disorder—which accurately simulate real-world data distributions such as time-series or sequential logging systems—we utilize the *Line Pattern* generator, as defined by Gu *et al.* The sequence of length $n$ is constructed using a monotonically increasing linear function that is deliberately perturbed by random noise. Formally, the $i$-th element of the sequence is generated according to the following formula:

$$A[i] = t \times i + b_i$$

In this equation, $t$ represents the slope of the increasing line, and $b_i$ is an independent random variable drawn from a uniform distribution bounded by a specified noise parameter (`offset`). By adjusting the slope $t$ and the distribution bounds of the noise $b_i$, we can precisely control the rank (the optimal LIS length, $k$) of the input data. The Line pattern forces the LIS length $k$ to scale proportionally with the input size $n$, thereby creating massive active working sets while simultaneously preserving a high degree of spatial locality.

#### 2. Performance Graph
<p align="left">
  <img src="/results/ryzen3_5300U/plots/line_pattern_100000000_10000.jpg" alt="Performance Comparison: Line Pattern" width="600"/>
  <br>
  <em><b>Figure 2:</b> Execution time and speedup comparison for the Line pattern (N=10<sup>8</sup>, offset 10000).</em>
</p>

#### 3. Hardware Performance Counters
Methodological note on Hardware Profiling:
The hardware performance counters (`perf stats`) and energy consumption metrics presented in the table below reflect the execution of the *entire program binary* (e.g., `./test_lis`), rather than isolating strictly the Region-of-Interest (ROI) of the LLIS computation algorithms. Consequently, the reported metrics—such as total cycles, instructions, cache misses, and Energy-Delay Product (EDP)—inclusively encompass the computational and memory overhead associated with the synthetic data generation and initial array allocation phases.

**Table 2: Hardware Performance Counters and Energy Metrics for the Sequential Schensted (1T) and the Schensted (2T) executing the Line pattern ($n=10^8$, offset 10000) on the AMD Ryzen platform.**

| Micro-architectural Metric | Schensted 1T | Schensted 2T |
| :--- | ---: | ---: |
| **Cycles** | 871,617,474,592 | 860,018,309,698 |
| **Instructions** | 677,406,576,250 | 657,668,688,790 |
| **Stalled Cycles (Frontend)** | 443,922,201,622 | 447,419,849,760 |
| **Branches** | 155,462,697,811 | 151,540,159,385 |
| **Branch Misses** | 25,603,695,620 | 25,210,646,474 |
| **Cache References** | 1,770,454,702 | 1,779,105,213 |
| **Cache Misses** | 84,606,669 | 96,736,378 |
| **L1 D-Cache Load Misses** | 841,705,720 | 842,101,485 |
| **Execution Time (s)** | 225.79 | 117.30 |
| **Energy Consumed (J)** | 1,027.72 | 731.00 |
| **Energy-Delay Product (J·s)** | **232,052.78** | **85,747.73** |

#### 4. Micro-architectural Conclusion: Latency Hiding and Memory-Level Parallelism (MLP)

In scenarios characterized by limited entropy or bounded disorder (e.g., the Line pattern), the memory access pattern inherently exhibits high spatial locality. The active search space natively fits within the L1/L2 cache levels, preventing capacity misses. Consequently, the `L1-dcache-load-misses` remain practically identical (~841-842 million) for both executions. 

However, the algorithm is not compute-bound. The strict data dependencies of the binary search enforce a constant wait for L1/L2 cache hits (which cost ~4-15 cycles each), explaining why the processor pipeline remains severely stalled for ~51-52% of the execution time in both threads (as proven by the massive >440 billion `stalled-cycles-frontend`). The efficiency of the 2T architecture is driven by the following micro-architectural mechanisms:

*   Thread-Level to Memory-Level Parallelism (TLP & MLP): The architecture utilizes TLP to issue independent search queries, unlocking Memory-Level Parallelism (MLP). While total `stalled-cycles-frontend` slightly increase in the 2T execution (from 443.9 billion to 447.4 billion) due to the $O(k)$ merge-step overhead and shared L3 cache contention, the absolute execution time is nearly halved. This indicates that the independent memory stalls of the two cores are effectively overlapped (latency hiding) in wall-clock time.
*   Race to Sleep & EDP Reduction: By overlapping the inherent L1/L2 latencies across two threads, the 2T architecture rapidly completes the workload and transitions the processor into deep idle C-states. This profound efficiency yields a 63.0% reduction in the Energy-Delay Product (EDP), proving that the speedup is achieved without an unproportional energy trade-off.

---

## Compilation & Execution Guide for reproducing the Benchmarks

### Grant execution permissions to all shell scripts
chmod +x *.sh

### 00 Compilation

Run:

```bash
./run00_compile.sh
```

Under the hood, this script executes the following command, ensuring maximum compiler optimizations (-O3) and host-specific architecture tuning (-march=native):

```bash
g++ -O3 -march=native -std=c++11 -Wall -Wextra -Wconversion -pedantic ../src/test_lis.cpp -pthread -o test_lis
```

### 01 Enable perfomance mode

Run:

```bash
./run01_enable_performance_mode.sh
```

Under the hood, this script executes the following command:

```bash
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### 02 Allow perf stats

Run:

```bash
./run02_allow_perf_stats.sh
```

Under the hood, this script executes the following commands:

```bash
sudo sysctl -w kernel.perf_event_paranoid=-1
sudo sysctl -w kernel.kptr_restrict=0
```

### 03 Run range pattern tests

Run:

```bash
./run03_range_100m.sh
```

Under the hood, this script executes the following command:

```bash
./test_lis -size 100000000 -trials 5 -pattern range -upper_limit 100000000 -algo all
```

### 04 Run range pattern perf stats for 1T

Run:

```bash
./run04_1T_range_100m_perf_stats.sh
```

Under the hood, this script executes the following command:

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000000 -trials 5 -pattern range -upper_limit 100000000 -algo 1T
```

### 05 Run range pattern perf energy measurement for 1T

Run:

```bash
./run05_1T_range_100m_energy.sh
```

Under the hood, this script executes the following command:

```bash
perf stat -e energy-pkg ./test_lis -size 100000000 -trials 5 -pattern range -upper_limit 100000000 -algo 1T
```

### 06 Run range pattern perf stats for 2T

Run:

```bash
./run06_2T_range_100m_perf_stats.sh
```

Under the hood, this script executes the following command:

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000000 -trials 5 -pattern range -upper_limit 100000000 -algo 2T
```

### 07 Run range pattern perf energy measurement for 2T

Run:

```bash
./run07_2T_range_100m_energy.sh
```

Under the hood, this script executes the following command:

```bash
perf stat -e energy-pkg ./test_lis -size 100000000 -trials 5 -pattern range -upper_limit 100000000 -algo 2T
```

### 08 Run line pattern tests

Run:

```bash
./run08_line_100m_offset10k.sh
```

Under the hood, this script executes the following command:

```bash
./test_lis -size 100000000 -trials 5 -pattern line -offset 10000 -algo all
```

### 09 Run line pattern perf stats for 1T

Run:

```bash
run09_1T_line_100m_offset10k_perf_stats.sh
```

Under the hood, this script executes the following command:

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000000 -trials 5 -pattern line -offset 10000 -algo 1T
```

### 10 Run line pattern perf energy measurement for 1T

Run:

```bash
run10_1T_line_100m_offset10k_energy.sh
```

Under the hood, this script executes the following command:

```bash
perf stat -e energy-pkg ./test_lis -size 100000000 -trials 5 -pattern line -offset 10000 -algo 1T
```

### 11 Run line pattern perf stats for 2T

Run:

```bash
run11_2T_line_100m_offset10k_perf_stats.sh
```

Under the hood, this script executes the following command:

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000000 -trials 5 -pattern line -offset 10000 -algo 2T
```

### 12 Run line pattern perf energy measurement for 2T

Run:

```bash
run12_2T_line_100m_offset10k_energy.sh
```

Under the hood, this script executes the following command:

```bash
perf stat -e energy-pkg ./test_lis -size 100000000 -trials 5 -pattern line -offset 10000 -algo 2T
```

### 13 Enable balanced mode

Run:

```bash
run13_enable_balanced_mode.sh
```

Under the hood, this script executes the following commands:

```bash
for gov in schedutil ondemand powersave; do echo $gov | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor 2>/dev/null && break; done
```

---


## Run on your own data

Given a text file `input_data_one_integer_per_line.txt` with integers, each in a different line, you can compute the length of the LIS of the first 100000 of them by running the following command:

```bash
./test_lis -size 100000 -trials 10 -pattern stdin < input_data_one_integer_per_line.txt
```
or alternatively:

```bash
cat input_data_one_integer_per_line.txt | ./test_lis -size 100000 -trials 10 -pattern stdin
```

In order to take perf stats for either 1T or 2T, you can run the following commands:

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000 -trials 10 -algo 1T -pattern stdin < input_data_one_integer_per_line.txt
```

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000 -trials 10 -algo 2T -pattern stdin < input_data_one_integer_per_line.txt
```

or alternatively:

```bash
cat input_data_one_integer_per_line.txt | perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000 -trials 10 -algo 1T -pattern stdin
```

```bash
cat input_data_one_integer_per_line.txt | perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses,branches,branch-misses,stalled-cycles-frontend ./test_lis -size 100000 -trials 10 -algo 2T -pattern stdin
```

In order to take perf energy measurements for either 1T or 2T, you can run the following commands:

```bash
perf stat -e energy-pkg ./test_lis -size 100000 -trials 10 -algo 1T -pattern stdin < input_data_one_integer_per_line.txt
```

```bash
perf stat -e energy-pkg ./test_lis -size 100000 -trials 10 -algo 2T -pattern stdin < input_data_one_integer_per_line.txt
```

or alternatively:

```bash
cat input_data_one_integer_per_line.txt | perf stat -e energy-pkg ./test_lis -size 100000 -trials 10 -algo 1T -pattern stdin
```

```bash
cat input_data_one_integer_per_line.txt | perf stat -e energy-pkg ./test_lis -size 100000 -trials 10 -algo 2T -pattern stdin
```

---