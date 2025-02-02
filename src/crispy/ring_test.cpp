// SPDX-License-Identifier: Apache-2.0
#include <crispy/ring.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <format>
#include <iostream>

using crispy::fixed_size_ring;
using crispy::ring;
using std::generate_n;

namespace
{
template <typename T>
[[maybe_unused]] void dump(ring<T> const& r)
{
    std::cout << std::format("ring(@{}): {{", r.zero_index());
    for (size_t i = 0; i < r.size(); ++i)
    {
        if (i)
            std::cout << std::format(", ");
        std::cout << std::format("{}", r[i]);
    }
    std::cout << std::format("}}\n");
}
} // namespace

TEST_CASE("ring.init")
{
    ring<char> r(3, {});
    generate_n(r.begin(), 3, [c = 'a']() mutable { return c++; });
    REQUIRE(r[0] == 'a');
    REQUIRE(r[1] == 'b');
    REQUIRE(r[2] == 'c');
}

TEST_CASE("ring.push_back")
{
    ring<char> r;
    r.push_back('a');
    r.push_back('b');
    r.push_back('c');
    REQUIRE(r[0] == 'a');
    REQUIRE(r[1] == 'b');
    REQUIRE(r[2] == 'c');
}

TEST_CASE("ring.emplace_back")
{
    ring<char> r;
    r.emplace_back('a');
    r.emplace_back('b');
    r.emplace_back('c');
    REQUIRE(r[0] == 'a');
    REQUIRE(r[1] == 'b');
    REQUIRE(r[2] == 'c');
}

TEST_CASE("ring.rotate_right")
{
    ring<char> r(3, {});
    generate_n(r.begin(), 3, [c = 'a']() mutable { return c++; });
    r.rotate_right(1);
    REQUIRE(r[0] == 'c');
    REQUIRE(r[1] == 'a');
    REQUIRE(r[2] == 'b');
}

TEST_CASE("ring.rotate_right_2")
{
    ring<char> r(3, {});
    generate_n(r.begin(), 3, [c = 'a']() mutable { return c++; });
    r.rotate_right(2);
    REQUIRE(r[0] == 'b');
    REQUIRE(r[1] == 'c');
    REQUIRE(r[2] == 'a');
}

TEST_CASE("ring.rotate_left")
{
    ring<char> r(3, {});
    generate_n(r.begin(), 3, [c = 'a']() mutable { return c++; });
    r.rotate_left(1);
    REQUIRE(r[0] == 'b');
    REQUIRE(r[1] == 'c');
    REQUIRE(r[2] == 'a');
}

TEST_CASE("ring.rotate_left_2")
{
    ring<char> r(3, {});
    generate_n(r.begin(), 3, [c = 'a']() mutable { return c++; });
    r.rotate_left(2);
    REQUIRE(r[0] == 'c');
    REQUIRE(r[1] == 'a');
    REQUIRE(r[2] == 'b');
}

TEST_CASE("ring.rotate_left_3")
{
    ring<char> r(3, {});
    generate_n(r.begin(), 3, [c = 'a']() mutable { return c++; });
    r.rotate_left(3);
    REQUIRE(r[0] == 'a');
    REQUIRE(r[1] == 'b');
    REQUIRE(r[2] == 'c');
}

TEST_CASE("ring.rezero")
{
    ring<char> r(6, {});
    generate_n(r.begin(), r.size(), [c = 'a']() mutable { return c++; });

    r.rotate_right(2);
    r.rezero();
    REQUIRE(r[0] == 'e');
    REQUIRE(r[1] == 'f');
    REQUIRE(r[2] == 'a');
    REQUIRE(r[3] == 'b');
    REQUIRE(r[4] == 'c');
    REQUIRE(r[5] == 'd');
}

TEST_CASE("ring.rezero.iterator")
{
    ring<char> r(6);
    generate_n(r.begin(), r.size(), [c = 'a']() mutable { return c++; });
    r.rezero(std::next(r.begin(), 2));
    REQUIRE(r[0] == 'c');
    REQUIRE(r[1] == 'd');
    REQUIRE(r[2] == 'e');
    REQUIRE(r[3] == 'f');
    REQUIRE(r[4] == 'a');
    REQUIRE(r[5] == 'b');
}

TEST_CASE("ring.fixed_size")
{
    fixed_size_ring<char, 6> r;
    generate_n(r.begin(), r.size(), [c = 'a']() mutable { return c++; });
    REQUIRE(r[0] == 'a');
    REQUIRE(r[1] == 'b');
    REQUIRE(r[2] == 'c');
    REQUIRE(r[3] == 'd');
    REQUIRE(r[4] == 'e');
    REQUIRE(r[5] == 'f');
}

TEST_CASE("ring.offset_negative")
{
    ring<char> r;
    r.emplace_back('a');
    r.emplace_back('b');
    r.emplace_back('c');

    REQUIRE(r[0] == 'a');
    REQUIRE(r[1] == 'b');
    REQUIRE(r[2] == 'c');
    REQUIRE(r[-1] == 'c');
    REQUIRE(r[-2] == 'b');
    REQUIRE(r[-3] == 'a');
}
