#include <vector>
#include <iostream>
#include "schensted.hpp"

int main()
{
    std::vector<int> data = { 10, 22, 9, 33, 21, 50, 41, 60, 80 };

    size_t len1 = Schensted::LLIS_1T( data.begin(), data.end() );
    std::cout << "LLIS 1T: " << len1 << "\n";

    size_t len2 = Schensted::LLIS_2T( data.begin(), data.end() );
    std::cout << "LLIS 2T: " << len2 << "\n";


    std::vector<std::string> data_str = { "10", "22", "9", "33", "21", "50", "41", "60", "80" };

    len1 = Schensted::LLIS_1T( data_str.begin(), data_str.end() );
    std::cout << "LLIS 1T: " << len1 << "\n";

    len2 = Schensted::LLIS_2T( data_str.begin(), data_str.end() );
    std::cout << "LLIS 2T: " << len2 << "\n";

    return 0;
}
