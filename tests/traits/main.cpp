/*------------------------------------------------------------------------------
-- This file is a part of the SciQLop Software
-- Copyright (C) 2021, Plasma Physics Laboratory - CNRS
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
-------------------------------------------------------------------------------*/
/*-- Author : Alexis Jeandet
-- Mail : alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#include <channels/channels.hpp>
#include <channels/pipelines.hpp>
#include <functional>

#include <catch2/catch_test_macros.hpp>


template <std::size_t size, typename T>
struct test_functor
{
    static inline T static_source() { return T {}; };
    T source() { return T {}; }

    static inline T static_filter(const T& v) { return v; };
    T source(const T& v) { return v; }

    static inline void static_sink(const T& v) {};
    void sink(const T& v) { }
};

TEST_CASE("Channels utils", "[type traits]")
{
    using namespace std::placeholders;
    using namespace channels;
    using namespace channels::operators;
    using namespace channels::traits;

    using test_functor_t = test_functor<256, double>;

    auto test_source_lambda = []() { return 10; };
    auto test_sink_lambda = [](int a) { (void)a; };
    auto test_filter_const_ref_lambda = [](const int& a) { return a + 1; };
    auto test_filter_lambda = [](int a) { return a + 1; };


    REQUIRE(is_channel_like_v<channel<int>>);
    REQUIRE(has_output_chan_interface_v<channel<int>>);

    REQUIRE(
        is_channel_like_v<decltype(channel<int> {} >> std::bind(std::multiplies<int> {}, 10, _1))>);
    REQUIRE(!is_channel_like_v<decltype(std::bind(std::multiplies<int> {}, 10, _1))>);

    REQUIRE(is_source_function_v<int(void)>);
    REQUIRE(is_source_function_v<decltype(test_source_lambda)>);
    REQUIRE(!is_source_function_v<void>);
    REQUIRE(!is_source_function_v<int(int)>);
    REQUIRE(!is_source_function_v<void(int)>);
    REQUIRE(!is_source_function_v<void(void)>);

    REQUIRE(!takes_void_v<decltype(test_sink_lambda)>);
    REQUIRE(returns_void_v<decltype(test_sink_lambda)>);
    REQUIRE(is_sink_function_v<decltype(test_sink_lambda)>);

    REQUIRE(std::is_same_v<return_type_t<decltype(test_source_lambda)>, int>);
    REQUIRE(std::is_same_v<int, arg_type_t<decltype(test_filter_lambda)>>);
    REQUIRE(std::is_same_v<const int&, arg_type_t<decltype(test_filter_const_ref_lambda)>>);
    REQUIRE(std::is_same_v<return_type_t<decltype(test_source_lambda)>,
        std::remove_cv_t<std::remove_reference_t<arg_type_t<decltype(test_filter_lambda)>>>>);
    REQUIRE(std::is_same_v<int, return_type_t<decltype(std::bind(std::plus<int>(),1))>>);


    REQUIRE(is_callable<double(double const&)&>::value);
    REQUIRE(is_callable<decltype(test_source_lambda)>::value);
    REQUIRE(is_callable<decltype(test_filter_lambda)>::value);
    REQUIRE(is_callable<std::plus<int>>::value);
    REQUIRE(is_callable<decltype (std::bind(std::plus<int>(),1))>::value);
    REQUIRE(is_callable<decltype(test_functor_t::static_source)>::value);
    REQUIRE(is_callable<decltype(test_functor_t::static_sink)>::value);
    REQUIRE(is_callable<decltype(test_functor_t::static_filter)>::value);

    REQUIRE(are_compatible_v<decltype(test_functor_t::static_source),
        decltype(test_functor_t::static_filter)>);
    REQUIRE(are_compatible_v<decltype(test_functor_t::static_source),
        decltype(test_functor_t::static_sink)>);
    REQUIRE(are_compatible_v<decltype(test_functor_t::static_filter),
        decltype(test_functor_t::static_sink)>);
    REQUIRE(are_compatible_v<double(void)&, double(const double&)&>);


    REQUIRE(are_compatible_v<decltype(test_source_lambda), decltype(test_filter_lambda)>);
    REQUIRE(are_compatible_v<decltype(test_source_lambda), decltype(test_filter_const_ref_lambda)>);
}
