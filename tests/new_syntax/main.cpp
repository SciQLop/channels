/*------------------------------------------------------------------------------
-- This file is a part of the SciQLop Software
-- Copyright (C) 2020, Plasma Physics Laboratory - CNRS
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
#include <thread>

#include <catch2/catch_test_macros.hpp>

template <int k, typename T>
T coef(T value)
{
    return value * k;
}

template <int b, typename T>
T offset(T value)
{
    return value + b;
}

SCENARIO("new_syntax", "[Channels]")
{
    using namespace std::placeholders;
    using namespace channels::operators;

    GIVEN("a channel type")
    {

    using chan_t = channels::channel<int, 128, channels::full_policy::wait_for_space>;

    THEN("We can create a simple pipeline with a function")
    {
        auto pipeline = chan_t {} >> coef<10, int>;
        REQUIRE(pipeline.closed() == false);
        pipeline.add(10);
        REQUIRE(*pipeline.take() == 100);
    }

    THEN("We can create a pipeline with multiple functions")
    {
        auto pipeline2 = chan_t {} >> [](int value) { return value * 10; }
            >> [](int value) { return value - 1; };
        REQUIRE(pipeline2.closed() == false);
        pipeline2.add(10);
        REQUIRE(*pipeline2.take() == 99);
    }

    THEN("We can create a pipeline with a function and an offset")
    {
        auto pipeline3 = chan_t {} >> coef<10, int> >> offset<-1, int>;
        REQUIRE(pipeline3.closed() == false);
        pipeline3.add(10);
        REQUIRE(*pipeline3.take() == 99);
    }

    THEN("We can create a pipeline with std::bind")
    {
        auto pipeline4 = chan_t {} >> std::bind(std::multiplies<int> {}, 10, _1)
            >> std::bind(std::plus<int> {}, -1, _1);
        REQUIRE(pipeline4.closed() == false);
        pipeline4.add(10);
        REQUIRE(*pipeline4.take() == 99);
    }

    // WHEN("Adding values to the pipelines")
    // auto pipeline = chan_t {} >> coef<10, int>;

    // auto pipeline2
    //     = chan_t {} >> [](int value) { return value * 10; } >> [](int value) { return value - 1; };

    // auto pipeline3 = chan_t {} >> coef<10, int> >> offset<-1, int>;

    // auto pipeline4 = chan_t {} >> std::bind(std::multiplies<int> {}, 10, _1)
    //     >> std::bind(std::plus<int> {}, -1, _1);

    // pipeline.add(10);
    // REQUIRE(*pipeline.take() == 100);
    // pipeline2 << 10;
    // REQUIRE(*pipeline2.take() == 99);
    // pipeline3 << 10;
    // REQUIRE(*pipeline3.take() == 99);
    // pipeline4 << 10;
    // REQUIRE(*pipeline4.take() == 99);
    }
}
