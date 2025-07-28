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
#include <channels/concepts.hpp>
#include <channels/pipelines.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Channels concepts", "[concepts]")
{
    using namespace std::placeholders;
    using namespace channels;
    using namespace channels::operators;
    using namespace channels::traits;

    static_assert(ChannelConcept<channel<int>>);
    THEN("ChannelConcept should be satisfied for channel<int>")
    {
        REQUIRE(ChannelConcept<channel<int>>);
    }
    THEN("ChannelConcept should be satisfied for channel<int, 10> whatever the policy")
    {
        REQUIRE(ChannelConcept<channel<int, 10, full_policy::overwrite_last>>);
        REQUIRE(ChannelConcept<channel<int, 10, full_policy::wait_for_space>>);
    }
    THEN("ChannelInput should be satisfied for a pipeline that starts with a channel")
    {
        using pipeline_t = decltype(channel<int> {} >> std::bind(std::multiplies<int> {}, 10, _1)
            >> std::bind(std::plus<int> {}, -1, _1));
        REQUIRE(ChannelInput<pipeline_t>);
    }

    /*THEN("ChannelOutput should be satisfied for a pipeline that ends with a channel")
    {
        using pipeline_t = decltype(channel<int> {}>>std::bind(std::multiplies<int> {}, 10, _1)
            >> std::bind(std::plus<int> {}, -1, _1) >> channel<int> {});
        REQUIRE(ChannelOutput<pipeline_t>);
    }
    THEN("ChannelConcept should be satisfied for pipelines with channels on both ends")
    {
        using pipeline_t = decltype(channel<int> {} >> std::bind(std::multiplies<int> {}, 10, _1));
        REQUIRE(ChannelConcept<pipeline_t>);
    }*/


    THEN("ChannelConcept should not be satisfied for non-channel types")
    {
        REQUIRE(!ChannelConcept<int>);
        REQUIRE(!ChannelConcept<std::function<void()>>);
    }
}
