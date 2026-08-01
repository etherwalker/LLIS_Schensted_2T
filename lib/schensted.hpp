/*
 * schensted.hpp
 *
 * This file is part of the following repository:
 * https://github.com/etherwalker/LLIS_Schensted_2T
 *
 */


#ifndef SCHENSTED_HPP
#define SCHENSTED_HPP

#include <algorithm>
#include <future>
#include <iterator>
#include <memory>
#include <functional>
#include <utility>

namespace Schensted
{
    namespace
    {
        template <typename Iterator, typename ValueType, typename Compare>
        size_t compute_lis_buffer( Iterator first, Iterator last, ValueType* buf, Compare comp )
        {
            if ( first == last ) return 0;

            auto it = first;
            buf[1] = *it;
            size_t lis_len = 1;
            ++it;

            for ( ; it != last; ++it ) {
                if ( comp(buf[lis_len], *it) ) {
                    buf[++lis_len] = *it;
                }
                else {
                    auto pos = std::lower_bound( buf + 1, buf + lis_len + 1, *it, comp );
                    *pos = *it;
                }
            }

            return lis_len;
        }

        template <typename RandomAccessIterator, typename ValueType, typename Compare>
        size_t min_tails( RandomAccessIterator first, RandomAccessIterator last, ValueType* min_tails_buf, Compare comp )
        {
            return compute_lis_buffer( first, last, min_tails_buf, comp );
        }

        template <typename RandomAccessIterator, typename ValueType, typename Compare>
        size_t max_heads( RandomAccessIterator first, RandomAccessIterator last, ValueType* max_heads_buf, Compare comp )
        {
            return compute_lis_buffer(
                std::reverse_iterator<RandomAccessIterator>(last),
                std::reverse_iterator<RandomAccessIterator>(first),
                max_heads_buf,
                [&comp](const ValueType& a, const ValueType& b) {
                    return comp(b, a);
                }
            );
        }

    } // namespace


    template <typename RandomAccessIterator,
              typename Compare = std::less<typename std::iterator_traits<RandomAccessIterator>::value_type>>
    size_t LLIS_1T( RandomAccessIterator first, RandomAccessIterator last, Compare comp = Compare() )
    {
        size_t n = std::distance( first, last );
        if ( n <= 1 ) return n;

        using ValueType = typename std::iterator_traits<RandomAccessIterator>::value_type;

        std::unique_ptr<ValueType[]> min_tails_buf(new ValueType[n + 1]);

        return min_tails( first, last, min_tails_buf.get(), comp );
    }


    template <typename RandomAccessIterator,
              typename Compare = std::less<typename std::iterator_traits<RandomAccessIterator>::value_type>>
    size_t LLIS_2T( RandomAccessIterator first, RandomAccessIterator last, Compare comp = Compare() )
    {
        size_t n = std::distance( first, last );
        if ( n <= 1 ) return n;

        size_t mid     = n / 2;
        size_t left_n  = mid;
        size_t right_n = n - left_n;

        using ValueType = typename std::iterator_traits<RandomAccessIterator>::value_type;

        std::unique_ptr<ValueType[]> min_tails_buf(new ValueType[left_n + 1]);
        std::unique_ptr<ValueType[]> max_heads_buf(new ValueType[right_n + 1]);

        RandomAccessIterator mid_it = first + mid;

        auto right_lis_len_future = std::async( std::launch::async, [=, &max_heads_buf, &comp]() {
            return max_heads(mid_it, last, max_heads_buf.get(), comp);
        });

        size_t left_lis_len  = min_tails( first, mid_it, min_tails_buf.get(), comp );
        size_t right_lis_len = right_lis_len_future.get();

        size_t lis_len;

        if ( comp(min_tails_buf[left_lis_len], max_heads_buf[right_lis_len]) ) {
            lis_len = left_lis_len + right_lis_len;
        }
        else {
            lis_len = std::max( left_lis_len, right_lis_len );

            size_t i = 1;
            size_t j = right_lis_len;

            while ( i <= left_lis_len && j >= 1 ) {
                if ( comp(min_tails_buf[i], max_heads_buf[j]) ) {
                    i++;
                } else {
                    if ( i - 1 + j > lis_len ) {
                        lis_len = i - 1 + j;
                    }
                    j--;
                }
            }

            if ( i - 1 + j > lis_len ) {
                lis_len = i - 1 + j;
            }
        }

        return lis_len;
    }

}

#endif // SCHENSTED_HPP
