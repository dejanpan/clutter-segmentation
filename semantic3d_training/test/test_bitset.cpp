/**
 * Author: Julius Adorf
 */

#include <gtest/gtest.h>
#include <boost/dynamic_bitset.hpp>

TEST(Bitset, Create66Bitset) {
    size_t size = 66;
    boost::dynamic_bitset<> bs(size);
    bs.clear();
    bs.set(0);
}

