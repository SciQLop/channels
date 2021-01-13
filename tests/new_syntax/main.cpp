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
#include <functional>
#include <thread>

#define CATCH_CONFIG_MAIN
#if __has_include(<catch2/catch.hpp>)
#include <catch2/catch.hpp>
#else
#include <catch.hpp>
#endif

template <int k, typename T>
T coef(T value)
{
    return value * k;
}

template <int b, typename T>
T offset(T value)
{
    return value - b;
}

SCENARIO("new_syntax", "[Channels]")
{
    using namespace std::placeholders;
    using chan_t = channels::channel<int, 128, channels::full_policy::wait_for_space>;

    auto pipeline = chan_t {} >> [](int value) { return value * 10; };

    auto pipeline2
        = chan_t {} >> [](int value) { return value * 10; } >> [](int value) { return value - 1; };

    auto pipeline3 = chan_t {} >> coef<10, int> >> offset<1, int>;

    auto pipeline4 = chan_t {} >> std::bind(std::multiplies<int> {}, 10, _1)
        >> std::bind(std::plus<int> {}, -1, _1);
}
