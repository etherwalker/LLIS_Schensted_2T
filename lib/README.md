# Schensted LIS C++ Header-Only Library

A lightweight, zero-dependency, header-only C++ library implementing Schensted's algorithm for Longest Increasing Subsequence (LIS) in both sequential (1T) and parallel 2-thread (2T) modes.

## Features
- Header-only (`schensted.hpp`): Simply include and compile with C++11 or later.
- Generic STL Iterator Interface: Works with any random-access iterator and comparable type.
- Parallel Speedup: `LLIS_2T` leverages a symmetric $O(k)$ lock-free merge step to achieve nearly linear speedup on dual-core/dual-thread execution.

## Minimal Example

```cpp
#include <vector>
#include <string>
#include <iostream>
#include "schensted.hpp"

int main()
{
    // Example with integers
    std::vector<int> data = { 10, 22, 9, 33, 21, 50, 41, 60, 80 };

    size_t len1 = Schensted::LLIS_1T( data.begin(), data.end() );
    std::cout << "LLIS 1T: " << len1 << "\n";

    size_t len2 = Schensted::LLIS_2T( data.begin(), data.end() );
    std::cout << "LLIS 2T: " << len2 << "\n";

    // Example with strings
    std::vector<std::string> data_str = { "10", "22", "9", "33", "21", "50", "41", "60", "80" };

    len1 = Schensted::LLIS_1T( data_str.begin(), data_str.end() );
    std::cout << "LLIS 1T: " << len1 << "\n";

    len2 = Schensted::LLIS_2T( data_str.begin(), data_str.end() );
    std::cout << "LLIS 2T: " << len2 << "\n";

    return 0;
}
```

## Compilation and execution
```bash
g++ -O3 -std=c++11 example.cpp -pthread -o example
./example
```