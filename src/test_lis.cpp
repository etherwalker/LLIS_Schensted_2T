/*
 * test_lis.cpp
 *
 * This file is part of the following repository:
 * https://github.com/etherwalker/LLIS_Schensted_2T
 *
 */


#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <future>
#include <iomanip>
#include <assert.h>


using data_type = long long;


size_t LLIS_Schensted_2T( data_type arr[], size_t n );
size_t LLIS_Schensted_1T( data_type arr[], size_t n );
size_t Schensted_min_tails( data_type arr[], size_t n, data_type* min_tails );
size_t Schensted_max_heads( data_type arr[], size_t from, size_t to, data_type* max_heads );


size_t LLIS_Schensted_2T( data_type arr[], size_t n )
{
    std::cout << "Running LLIS_Schensted_2T" << std::endl;

    if ( n <= 1)
    {
        return n;
    }

    size_t mid     = n/2;
    size_t left_n  = mid;
    size_t right_n = n-left_n;

    data_type *min_tails = new data_type[left_n+1];
    data_type *max_heads = new data_type[right_n+1];

    std::future<size_t> right_lis_len_future = std::async( std::launch::async, Schensted_max_heads, arr, mid, n-1, max_heads );

    size_t left_lis_len  = Schensted_min_tails( arr, mid, min_tails );
    size_t right_lis_len = right_lis_len_future.get();

    size_t lis_len;

    if ( min_tails[left_lis_len] < max_heads[right_lis_len] )
        lis_len = left_lis_len + right_lis_len;
    else  {
        lis_len = std::max( left_lis_len, right_lis_len );

        size_t i = 1;
        size_t j = right_lis_len;

        while ( i <= left_lis_len && j >= 1 ) {
            if ( min_tails[i] < max_heads[j] )
                i++;
            else {
                if ( i-1 + j > lis_len )
                    lis_len = i-1 + j;

                j--;
            }
        }

        if ( i-1 + j > lis_len )
            lis_len = i-1 + j;
    }

    delete[] min_tails;
    delete[] max_heads;

    return lis_len;
}


size_t LLIS_Schensted_1T( data_type arr[], size_t n )
{
    std::cout << "Running LLIS_Schensted_1T" << std::endl;

    if ( n <= 1)
    {
        return n;
    }

    data_type *min_tails = new data_type[n+1];

    size_t lis_len = Schensted_min_tails( arr, n, min_tails );

    delete[] min_tails;

    return lis_len;
}


size_t Schensted_min_tails( data_type arr[], size_t n, data_type* min_tails )
{
    if ( n == 0 )
    {
        return 0;
    }

    min_tails[1]   = arr[0];
    size_t lis_len = 1;

    for ( size_t i = 1; i < n; i++ )
    {
        if ( arr[i] > min_tails[lis_len] )
        {
            min_tails[++lis_len] = arr[i];
        }
        else
        {
            auto it = std::lower_bound(min_tails + 1, min_tails + lis_len + 1, arr[i]);
            size_t lo = std::distance(min_tails, it);
            min_tails[lo] = arr[i];
        }
    }

    return lis_len;
}


size_t Schensted_max_heads( data_type arr[], size_t from, size_t to, data_type* max_heads )
{
    long long n = (long long) to - (long long) from + 1;

    if ( n <= 0 )
    {
        return 0;
    }

    max_heads[1]   = arr[to];
    size_t lis_len = 1;

    for ( size_t i = to-1; i >= from; i-- )
    {
        if ( arr[i] < max_heads[lis_len] )
        {
            max_heads[++lis_len] = arr[i];
        }
        else
        {
            auto it = std::lower_bound(max_heads + 1, max_heads + lis_len + 1, arr[i], std::greater<data_type>());
            size_t lo = std::distance(max_heads, it);
            max_heads[lo] = arr[i];
        }
    }

    return lis_len;
}


/*
 * Hash64 utility function (Adapted)
 * Original Source: https://github.com/cmuparlay/parlaylib/blob/master/include/parlay/utilities.h
 * Used and adapted under the terms of the MIT License.
 *
 */

inline uint64_t hash64(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >> 4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v << 5;
  return v;
}


/*
 * Synthetic Data Generator (Adapted)
 * Based on the methodology by Gu et al., "Parallel Longest Increasing Subsequence and van Emde Boas Trees" (SPAA '23).
 * Paper DOI: https://doi.org/10.1145/3558481.3591069
 * Original Source: https://github.com/ucrparlay/Parallel-LIS/blob/main/init_seq.cpp
 * function: initializeLineArray
 * Used and adapted under the terms of the MIT License.
 *
 */

std::string create_test_data_gu_et_al_line_pattern( long long arr[], size_t n, size_t upper_limit, size_t lis_len, size_t seed = 0, long long offset = 10 )
{
    assert( n >= lis_len );

    std::stringstream ss;
    ss << "gu_et_all line pattern " << "param upper_limit: " << upper_limit << " param offset: " << offset << " param lis_len: " << lis_len;
    std::string test_name = ss.str();

    std::cout << "data pattern " << ss.str() << std::endl;

    size_t base=0;
    if( upper_limit < lis_len + offset ) base = 0;
    else base = upper_limit - lis_len - offset;
    float gain = (float)(lis_len)/(float) n;

    if( offset != 0 )
    {
        for ( size_t i = 0; i < n; i++ )
        {
          arr[i] = (long long) ((float) (hash64( i + seed*n ) % (offset) + base) + gain*(float)i);
        }
    }
    else
    {
        for ( size_t i = 0; i < n; i++ )
        {
          arr[i] = (long long)((float)base + gain*(float)i);
        }
    }

    return test_name;
}


/*
 * Synthetic Data Generator (Adapted)
 * Based on the methodology by Gu et al., "Parallel Longest Increasing Subsequence and van Emde Boas Trees" (SPAA '23).
 * Paper DOI: https://doi.org/10.1145/3558481.3591069
 * Original Source: https://github.com/ucrparlay/Parallel-LIS/blob/main/init_seq.cpp
 * function: initializePureRandomArray
 * Used and adapted under the terms of the MIT License.
 *
 */

std::string create_test_data_gu_et_al_range_pattern( long long arr[], size_t n, size_t upper_limit, size_t lis_len, size_t seed = 0 )
{
    assert( n >= lis_len );

    std::stringstream ss;
    ss << "gu_et_all range pattern " << "param upper_limit: " << upper_limit << " param lis_len: " << lis_len;
    std::string test_name = ss.str();

    std::cout << "data pattern " << ss.str() << std::endl;

    for ( size_t i = 0; i < n; i++ )
    {
      arr[i] = hash64( i + seed*n ) % ( upper_limit );
    }

    return test_name;
}


// function for fast reading integer values

#ifdef _WIN32
#define getchar_unlocked _getchar_nolock
#endif
long long read_int()
{
    bool is_negative = false;
    long long a = 0;

    int c = getchar_unlocked();

    while (c != '-' && (c < '0' || c > '9')) {
        if (c == EOF) return 0;
        c = getchar_unlocked();
    }

    if (c == '-') {
        is_negative = true;
        c = getchar_unlocked();
    }

    while (c >= '0' && c <= '9') {
        a = a * 10 + (c - '0');
        c = getchar_unlocked();
    }

    return is_negative ? -a : a;
}


std::string create_test_data_stdin( long long arr[], size_t n )
{
    std::stringstream ss;
    ss << "reading " << n << " values from stdin";
    std::string test_name = ss.str();

    std::cout << "data pattern " << ss.str() << std::endl;

    for ( size_t i = 0; i < n; i++ )
    {
        arr[i] = read_int();
    }

    return test_name;
}


struct Tests_Results
{
    size_t n;
    std::string algo_name;
    std::vector<size_t> test_ids;
    std::vector<std::string> test_names;
    std::vector<std::vector<long long>> test_times;
    std::vector<size_t> test_result;
};


std::vector<Tests_Results> run_tests( data_type *arr, size_t n, size_t num_of_trials, std::vector<size_t>& algos )
{
    size_t num_of_algos = algos.size();

    std::vector<Tests_Results> trs;
    Tests_Results t_r;
    t_r.n = n;
    for( size_t algo = 1; algo <= num_of_algos; algo++ )
    {
        trs.push_back( t_r );
    }

    std::cout << std::endl << "creating test data..." << std::endl;

    std::string test_name = create_test_data_stdin( arr, n );

    std::cout << "done" << std::endl;

    size_t lis_len;

    std::vector<long long> times;

    size_t algo = 0;
    for ( size_t alg : algos )
    {
        trs[algo].test_ids.push_back( 0 );
        trs[algo].test_names.push_back( test_name );

        std::cout << std::endl << "Running test " << 0 << std::endl << std::endl;
        for ( size_t trial = 0; trial < num_of_trials; trial++ )
        {
            std::cout << std::endl << "Running trial " << trial << std::endl << std::endl;

            auto start_time = std::chrono::high_resolution_clock::now();

            if ( alg == 0 )
            {
                trs[algo].algo_name = "LLIS_Schensted_1T";
                lis_len = LLIS_Schensted_1T( arr, n );
            }
            else // if ( alg == 1 )
            {
                trs[algo].algo_name = "LLIS_Schensted_2T";
                lis_len = LLIS_Schensted_2T( arr, n );
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            std::cout << "lis_len:    " << lis_len << std::endl;
            std::cout << std::fixed << std::setprecision(3) << "Duration:   " << (double) duration.count()/1000. << std::endl;
            times.push_back( duration.count() );
        }

        trs[algo].test_result.push_back( lis_len );

        trs[algo].test_times.push_back( times );
        times.clear();

        algo++;
    }

    return trs;
}


std::vector<Tests_Results> run_tests_Gu_et_al_line_pattern( data_type *arr, size_t n, size_t num_of_trials, std::vector<size_t>& algos, size_t upper_limit, size_t offset, size_t lis_len_from, size_t lis_len_to )
{
    size_t num_of_tests = 0;
    for ( size_t lis_len = lis_len_from; lis_len <= lis_len_to; lis_len *= 10 )
    {
        num_of_tests++;
    }

    size_t num_of_algos = algos.size();

    std::vector<Tests_Results> trs;
    Tests_Results t_r;
    t_r.n = n;
    for( size_t algo = 1; algo <= num_of_algos; algo++ )
    {
        trs.push_back( t_r );
    }

    size_t t = 0;
    for ( size_t lis_len_param = lis_len_from; lis_len_param <= lis_len_to; lis_len_param *= 10 )
    {
        std::cout << std::endl << "creating test data..." << std::endl;

        size_t upper_limit_param = std::max( upper_limit, lis_len_param );

        std::string test_name;

        test_name = create_test_data_gu_et_al_line_pattern( arr, n, upper_limit_param, lis_len_param, 0, offset );

        std::cout << "done" << std::endl;

        size_t lis_len;

        std::vector<long long> times;

        size_t algo = 0;
        for ( size_t alg : algos )
        {
            trs[algo].test_ids.push_back( t );
            trs[algo].test_names.push_back( test_name );

            std::cout << std::endl << "Running test " << t << std::endl << std::endl;
            for ( size_t trial = 0; trial < num_of_trials; trial++ )
            {
                std::cout << std::endl << "Running trial " << trial << std::endl << std::endl;

                auto start_time = std::chrono::high_resolution_clock::now();

                if ( alg == 0 )
                {
                    trs[algo].algo_name = "LLIS_Schensted_1T";
                    lis_len = LLIS_Schensted_1T( arr, n );
                }
                else // if ( alg == 1 )
                {
                    trs[algo].algo_name = "LLIS_Schensted_2T";
                    lis_len = LLIS_Schensted_2T( arr, n );
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                std::cout << "lis_len:    " << lis_len << std::endl;
                std::cout << std::fixed << std::setprecision(3) << "Duration:   " << (double) duration.count()/1000. << std::endl;
                times.push_back( duration.count() );
            }

            trs[algo].test_result.push_back( lis_len );

            trs[algo].test_times.push_back( times );
            times.clear();

            algo++;
        }

        t++;
    }

    return trs;
}


std::vector<Tests_Results> run_tests_Gu_et_al_range_pattern( data_type *arr, size_t n, size_t num_of_trials, std::vector<size_t>& algos, size_t upper_limit )
{
    size_t num_of_tests = 0;
    for ( size_t u = 1; u <= upper_limit; u *= 10 )
    {
        num_of_tests++;
    }

    size_t num_of_algos = algos.size();

    std::vector<Tests_Results> trs;
    Tests_Results t_r;
    t_r.n = n;
    for( size_t algo = 1; algo <= num_of_algos; algo++ )
    {
        trs.push_back( t_r );
    }

    size_t t = 0;

    for ( size_t upper_limit_param = 1; upper_limit_param <= upper_limit; upper_limit_param *= 10 )
    {
        std::cout << std::endl << "creating test data..." << std::endl;

        std::string test_name;

        test_name = create_test_data_gu_et_al_range_pattern( arr, n, upper_limit_param, upper_limit_param, 0 );

        std::cout << "done" << std::endl;

        size_t lis_len;

        std::vector<long long> times;

        size_t algo = 0;
        for ( size_t alg : algos )
        {
            trs[algo].test_ids.push_back( t );
            trs[algo].test_names.push_back( test_name );

            std::cout << std::endl << "Running test " << t << std::endl << std::endl;
            for ( size_t trial = 0; trial < num_of_trials; trial++ )
            {
                std::cout << std::endl << "Running trial " << trial << std::endl << std::endl;

                auto start_time = std::chrono::high_resolution_clock::now();

                if ( alg == 0 )
                {
                    trs[algo].algo_name = "LLIS_Schensted_1T";
                    lis_len = LLIS_Schensted_1T( arr, n );
                }
                else // if ( alg == 1 )
                {
                    trs[algo].algo_name = "LLIS_Schensted_2T";
                    lis_len = LLIS_Schensted_2T( arr, n );
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                std::cout << "lis_len:    " << lis_len << std::endl;
                std::cout << std::fixed << std::setprecision(3) << "Duration:   " << (double) duration.count()/1000. << std::endl;
                times.push_back( duration.count() );
            }


            trs[algo].test_result.push_back( lis_len );

            trs[algo].test_times.push_back( times );
            times.clear();

            algo++;
        }

        t++;
    }

    return trs;
}


void print_usage()
{
    std::cout << "Usage: ./program [options]\n"
              << "Options:\n"
              << "  -size <size_t>              Default: 100000000\n"
              << "  -trials <size_t>            Default: 5\n"
              << "  -algo <1T|2T|all>           Default: all\n"
              << "  -pattern <range|line|stdin> Default: range\n"
              << "  -offset <size_t>            Only for 'line'. Default: 1000\n"
              << "  -lis_len_from <size_t>      Only for 'line'. Default: 1\n"
              << "  -lis_len_to <size_t>        Only for 'line'. Default: size\n"
              << "  -upper_limit <size_t>       Only for 'range'. Default: 100000000\n"
              ;
}


struct Config
{
    size_t size         = 100000000;
    size_t trials       = 5;
    std::string algo    = "all";
    std::string pattern = "range";
    size_t offset       = 1000;
    size_t lis_len_from = 1;
    size_t lis_len_to   = 100000000;
    size_t upper_limit  = 100000000;

    bool provided_offset       = false;
    bool provided_lis_len_from = false;
    bool provided_lis_len_to   = false;
    bool provided_upper_limit  = false;
};


int main( int argc, char** argv )
{

    Config cfg;

    for ( int i = 1; i < argc; ++i )
    {
        std::string arg = argv[i];

        try {
            if ( arg == "-size" && i + 1 < argc )
            {
                cfg.size = std::stoul( argv[++i] );
            }
            else if ( arg == "-trials" && i + 1 < argc )
            {
                cfg.trials = std::stoul( argv[++i] );
            }
            else if ( arg == "-algo" && i + 1 < argc )
            {
                cfg.algo = argv[++i];
            }
            else if ( arg == "-pattern" && i + 1 < argc )
            {
                cfg.pattern = argv[++i];
            }
            else if ( arg == "-offset" && i + 1 < argc )
            {
                cfg.offset = std::stoul( argv[++i] );
                cfg.provided_offset = true;
            }
            else if ( arg == "-lis_len_from" && i + 1 < argc )
            {
                cfg.lis_len_from = std::stoul( argv[++i] );
                cfg.provided_lis_len_from = true;
            }
            else if ( arg == "-lis_len_to" && i + 1 < argc )
            {
                cfg.lis_len_to = std::stoul( argv[++i] );
                cfg.provided_lis_len_to = true;
            }
            else if ( arg == "-upper_limit" && i + 1 < argc )
            {
                cfg.upper_limit = std::stoul( argv[++i] );
                cfg.provided_upper_limit = true;
            }
            else
            {
                std::cerr << "Error: Unknown or incomplete argument: " << arg << std::endl;
                print_usage();
                return 1;
            }
        } catch  (const std::exception& e )
        {
            std::cerr << "Error: Invalid value for argument " << arg << " (" << e.what() << ")" << std::endl;
            return 1;
        }
    }


    if ( cfg.provided_offset && cfg.pattern != "line" )
    {
        std::cerr << "Error: -offset is only allowed when -pattern is 'line'.\n";
        return 1;
    }

    if ( cfg.provided_lis_len_from && cfg.pattern != "line" )
    {
        std::cerr << "Error: -lis_len_from is only allowed when -pattern is 'line'.\n";
        return 1;
    }

    if ( cfg.provided_lis_len_to && cfg.pattern != "line" )
    {
        std::cerr << "Error: -lis_len_to is only allowed when -pattern is 'line'.\n";
        return 1;
    }

    if ( cfg.provided_upper_limit && cfg.pattern != "range" )
    {
        std::cerr << "Error: -upper_limit is only allowed when -pattern is 'range'.\n";
        return 1;
    }

    if ( cfg.algo != "1T" && cfg.algo != "2T" && cfg.algo != "all" )
    {
        std::cerr << "Error: Invalid value for -algo. Accepted values: 1T, 2T, all.\n";
        return 1;
    }

    if ( cfg.pattern != "range" && cfg.pattern != "line" && cfg.pattern != "stdin" )
    {
        std::cerr << "Error: Invalid value for -pattern. Accepted values: range, line, stdin.\n";
        return 1;
    }

    if ( !cfg.provided_lis_len_from && cfg.pattern == "line" )
    {
        cfg.provided_lis_len_from = 1;
    }

    if ( !cfg.provided_lis_len_to && cfg.pattern == "line" )
    {
        cfg.provided_lis_len_to = cfg.size;
    }

    if ( cfg.pattern == "line" )
    {
        if ( cfg.lis_len_from < 1 )
        {
            std::cerr << "Error: lis_len_from must be an integer between 1 and lis_len_to.\n";
            return 1;
        }

        if ( cfg.lis_len_to > cfg.size )
        {
            std::cerr << "Error: lis_len_to must be an integer between lis_len_from and size.\n";
            return 1;
        }

        if ( cfg.lis_len_from > cfg.lis_len_to )
        {
            std::cerr << "Error: lis_len_from must be less than or equal to lis_len_to.\n";
            return 1;
        }
    }


    std::cout << "Configuration successfully loaded:\n"
              << "Size:              " << cfg.size << "\n"
              << "Trials:            " << cfg.trials << "\n"
              << "Algorithm:         " << cfg.algo << "\n"
              << "Pattern:           " << cfg.pattern << "\n";

    if ( cfg.pattern == "line" )  std::cout << "Offset:            " << cfg.offset << "\n";
    if ( cfg.pattern == "line" )  std::cout << "Lis len from:      " << cfg.lis_len_from << "\n";
    if ( cfg.pattern == "line" )  std::cout << "Lis len to:        " << cfg.lis_len_to << "\n";
    if ( cfg.pattern == "range" ) std::cout << "Upper Limit:       " << cfg.upper_limit << "\n";

    size_t n = cfg.size;

    //unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    //std::mt19937_64 dre( cfg.seed );

    std::vector<size_t> algos;
    size_t num_of_algos = 2;
    if ( cfg.algo != "all" )
    {
        num_of_algos = 1;
        size_t algo;
        if ( cfg.algo == "1T" )
        {
            algo=0;
        }
        else // if ( cfg.algo == "2T" )
        {
            algo = 1;
        }

        algos.push_back( algo );
    }
    else
    {
        for( size_t algo = 0; algo < num_of_algos; algo++ )
        {
            algos.push_back( algo );
        }
    }

    data_type *arr = new data_type[n];

    std::vector<Tests_Results> trs;

    if ( cfg.pattern == "line" )
    {
        trs = run_tests_Gu_et_al_line_pattern( arr, n, cfg.trials, algos, n, cfg.offset, cfg.lis_len_from, cfg.lis_len_to );
    }
    else if ( cfg.pattern == "range" )
    {
        trs = run_tests_Gu_et_al_range_pattern( arr, n, cfg.trials, algos, cfg.upper_limit );
    }
    else
    {
        trs = run_tests( arr, n, cfg.trials, algos );
    }

    std::stringstream ss;

    for ( auto t_r : trs )
    {
        ss << std::endl << t_r.algo_name << " n: " <<  t_r.n << std::endl;
        size_t num_of_tests  = t_r.test_names.size();
        for ( size_t i = 0; i < num_of_tests; i++ )
        {
            ss << t_r.test_ids[i] << " " << t_r.test_names[i] << " times: ";

            size_t num_of_trials = t_r.test_times[i].size();
            for ( size_t j = 0; j < num_of_trials; j++ )
            {
                ss << std::fixed << std::setprecision(3) << (double) t_r.test_times[i][j]/1000. << " ";
            }

            ss << " lis_len: " << t_r.test_result[i];
            ss << std::endl;
        }
    }

    std::cout << ss.str();

    delete[] arr;

    time_t startTime = clock();
    time (&startTime);
    struct tm * timeinfo;
    char buffer [80];
    timeinfo = localtime (&startTime);
    strftime (buffer, 80,"%Y_%m_%d_%H_%M_%S", timeinfo);

    std::string fname = "";
    if ( cfg.pattern == "line" )
    {
        fname = "lis_new_results_gu_et_al_line_pattern_" + std::to_string( n ) + "_" + std::to_string( cfg.offset ) + "_" + std::string( buffer ) + ".txt";
    }
    else if ( cfg.pattern == "range" )
    {
        fname = "lis_new_results_gu_et_al_range_pattern_" + std::to_string( n ) + "_" + std::to_string( cfg.upper_limit ) + "_" + std::string( buffer ) + ".txt";
    }
    else
    {
        fname = "lis_new_results_" + std::to_string( n ) + "_" + std::string( buffer ) + ".txt";
    }

    std::ofstream results_file( fname );
    results_file << ss.str();
    results_file.close();

    return 0;
}
