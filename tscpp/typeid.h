/* Copyright (c) 2022 Skyward Experimental Rocketry
 * Author: Davide Mor
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <cstdlib>
#include <cstdint>

namespace tscpp
{

/**
 * Compile time enabled version of memcmp.
 */
constexpr bool comptime_memcmp(const char *a, const char *b, size_t len) {
    while(len--) {
        if(*a++ != *b++)
            return false;
    }

    return true;
}

/**
 * Compile time enabled hashing function (Jenkin's One-at-a-Time hash).
 * 
 * https://stackoverflow.com/questions/114085/fast-string-hashing-algorithm-with-low-collision-rates-with-32-bit-integer
 */
constexpr uint32_t comptime_hash(const char *a, size_t len) {
    uint32_t hash = 0;

    while(len--) {
        hash += *a++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/**
 * Obtain a unique ID at compile time, which is also stable across compilations.
 */
template<typename T>
constexpr uint32_t type_id() {
    
    // We define compiler dependent strings, THIS IS NOT PORTABLE!
#if defined(__GNUC__) && !defined(__clang__)
    #define PRETTY_FUNCTION __PRETTY_FUNCTION__
    #define PRETTY_FUNCTION_PRE "constexpr uint32_t tscpp::type_id() [with T = "
    #define PRETTY_FUNCTION_POST "; uint32_t = unsigned int]"
#else
    #error "tscpp: Your compiler is not supported!"
#endif

    constexpr size_t PRETTY_FUNCTION_SIZEOF = sizeof(PRETTY_FUNCTION) - 1;
    constexpr size_t PRETTY_FUNCTION_PRE_SIZEOF = sizeof(PRETTY_FUNCTION_PRE) - 1;
    constexpr size_t PRETTY_FUNCTION_POST_SIZEOF = sizeof(PRETTY_FUNCTION_POST) - 1;

    // Assert that PRETTY_FUNCTION is sane.
    // The prefix and postfix must be correct, otherwise HORRIBLE things can happen.
    static_assert(
        PRETTY_FUNCTION_PRE_SIZEOF < PRETTY_FUNCTION_SIZEOF && 
        comptime_memcmp(
            PRETTY_FUNCTION, 
            PRETTY_FUNCTION_PRE,
            PRETTY_FUNCTION_PRE_SIZEOF
        ), 
    "PRETTY_FUNCTION is not sane! (prefix mismatch)");
    
    static_assert(
        PRETTY_FUNCTION_POST_SIZEOF < PRETTY_FUNCTION_SIZEOF && 
        comptime_memcmp(
            PRETTY_FUNCTION + PRETTY_FUNCTION_SIZEOF - PRETTY_FUNCTION_POST_SIZEOF,
            PRETTY_FUNCTION_POST,
            PRETTY_FUNCTION_POST_SIZEOF
        ), 
    "PRETTY_FUNCTION is not sane! (suffix mismatch)");

    // Ok now that we know that it is sane we compute the hash,
    // stripping away any prefix/postfix, and leave just the type. 
    // This should reduce variance between compilers and hash collisions.

    // enum forces const evaluation, no runtime overhead!
    enum {Hash = comptime_hash(
        PRETTY_FUNCTION + PRETTY_FUNCTION_PRE_SIZEOF,
        PRETTY_FUNCTION_SIZEOF - PRETTY_FUNCTION_PRE_SIZEOF - PRETTY_FUNCTION_POST_SIZEOF
    )};

    return Hash;

    #undef PRETTY_FUNCTION
    #undef PRETTY_FUNCTION_PRE
    #undef PRETTY_FUNCTION_POST
}

}