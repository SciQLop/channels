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
#include <thread>

#define CATCH_CONFIG_MAIN
#if __has_include(<catch2/catch.hpp>)
#include <catch2/catch.hpp>
#else
#include <catch.hpp>
#endif

SCENARIO("Multithreads", "[Channels]")
{
    GIVEN("a single slot channel")
    {
        channels::channel<int, 1, channels::full_policy::overwrite_last> in_chan, out_chan;
        std::thread t{[&in_chan,&out_chan]()
            {
                out_chan << *in_chan.take() * 10;
            }};
        in_chan << 1;
        t.join();
        REQUIRE(out_chan.take() == 10);
    }

    GIVEN("a buffered channel")
    {
        channels::channel<int, 5, channels::full_policy::wait_for_space> chan1, chan2,chan3;
        std::thread t{[&chan1,&chan2]()
            {
                while (!chan1.closed()) {
                    auto val=chan1.take();
                    if(val)
                        chan2 << *val * 10;
                }
            }};
        std::thread t2{[&chan2,&chan3]()
            {
                static int i=0;
                static int result = 0;
                while (i<10)
                {
                    result += *chan2.take();
                    i++;
                }
                chan3 << result;
            }};
        for(int i=0;i<10;i++)
            chan1 << i;
        t2.join();
        chan1.close();
        t.join();
        REQUIRE(*chan3.take() == 450);
    }
}
