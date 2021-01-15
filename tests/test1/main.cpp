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

#define CATCH_CONFIG_MAIN
#if __has_include(<catch2/catch.hpp>)
#include <catch2/catch.hpp>
#else
#include <catch.hpp>
#endif

SCENARIO("Single thread", "[Channels]")
{
    GIVEN("a single slot channel")
    {
        channels::channel<int, 1, channels::full_policy::overwrite_last> chan;
        WHEN("Nothing is inside")
        {
            THEN("Size should be 0") { REQUIRE(std::size(chan) == 0); }
        }
        WHEN("adding one item")
        {
            chan << -1;
            THEN("Size should be 1 and item can be retreived")
            {
                REQUIRE(std::size(chan) == 1);
                REQUIRE(*chan.take() == -1);
                REQUIRE(std::size(chan) == 0);
            }
        }
        WHEN("adding two items")
        {
            chan << -1 << -2;
            THEN("Size should be still 1 and last item can be retreived")
            {
                REQUIRE(std::size(chan) == 1);
                REQUIRE(*chan.take() == -2);
                REQUIRE(std::size(chan) == 0);
            }
        }
    }

    GIVEN("a buffered channel")
    {
        channels::channel<int, 10, channels::full_policy::overwrite_last> chan;
        WHEN("Nothing is inside")
        {
            THEN("Size should be 0") { REQUIRE(std::size(chan) == 0); }
        }
        WHEN("adding one item")
        {
            chan << -1;
            THEN("Size should be 1 and item can be retreived")
            {
                REQUIRE(std::size(chan) == 1);
                REQUIRE(*chan.take() == -1);
                REQUIRE(std::size(chan) == 0);
            }
        }
        WHEN("adding two items")
        {
            chan << -10 << -2;
            THEN("Size should be 2 and both two items can be retreived")
            {
                REQUIRE(std::size(chan) == 2);
                REQUIRE(*chan.take() == -10);
                REQUIRE(std::size(chan) == 1);
                REQUIRE(*chan.take() == -2);
                REQUIRE(std::size(chan) == 0);
            }
        }
        WHEN("copying it")
        {
            auto copy = chan;
            THEN("Both chan and cpoy should point to the same queue")
            {
                REQUIRE(std::size(chan) == std::size(copy));
                chan << 5;
                REQUIRE(std::size(chan) == 1);
                REQUIRE(std::size(chan) == std::size(copy));
                copy << 10;
                REQUIRE(std::size(chan) == 2);
                REQUIRE(std::size(chan) == std::size(copy));
            }
        }
        WHEN("moving it")
        {
            chan << 1 << 2;
            const auto size = std::size(chan);
            auto moved = std::move(chan);
            THEN("moved should have size")
            {
                REQUIRE(std::size(moved) == size);
            }
        }
    }
}
