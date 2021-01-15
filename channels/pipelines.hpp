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
#pragma once
#include "channels.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>

namespace channels
{

namespace details
{

    template <typename _input_channel_t, typename _function_t>
    struct _pipeline
    {
        using pipeline_tag = struct pipeline_tag;
        using in_value_type = typename _input_channel_t::in_value_type;
        using full_policy_t = typename _input_channel_t::full_policy_t;
        static inline constexpr std::size_t max_size = _input_channel_t::max_size;
        using function_t = _function_t;
        using input_channel_t = _input_channel_t;
        using output_channel_t = channel<decltype(std::declval<_function_t>()(
                                             *std::declval<_input_channel_t>().take())),
            max_size, full_policy_t>;
        using out_value_type = typename output_channel_t::out_value_type;

        inline _pipeline& operator<<(in_value_type&& item)
        {
            add(std::move(item));
            return *this;
        }

        inline _pipeline& operator<<(const in_value_type& item)
        {
            add(item);
            return *this;
        }

        inline std::optional<out_value_type> take() { return output_chan.take(); }
        inline void add(in_value_type&& item) { input_chan.add(std::move(item)); }
        inline void add(const in_value_type& item) { input_chan.add(item); }

        inline bool closed() { return output_chan.closed(); }
        inline void close() { output_chan.close(); }

        inline std::size_t size() const noexcept { return std::size(output_chan); }

        _pipeline(_input_channel_t&& input_chan, _function_t f)
                : input_chan { std::move(input_chan) }
                , output_chan {}
                , f { f }
                , thread { &_pipeline::loop, this }
        {
        }

        _pipeline(const _input_channel_t& input_chan, _function_t f)
                : input_chan { input_chan }
                , output_chan {}
                , f { f }
                , thread { &_pipeline::loop, this }
        {
        }

        _pipeline(_pipeline&& other) = delete;

        ~_pipeline()
        {
            if (!input_chan.closed())
                input_chan.close();
            if (!output_chan.closed())
                output_chan.close();
            if (thread.joinable())
                thread.join();
        }

    private:
        void loop()
        {

            while (!input_chan.closed() && !output_chan.closed())
            {
                if (auto value = input_chan.take(); value)
                {
                    if constexpr(std::is_pointer_v<decltype (*value)>)
                        output_chan.add(f(*value));
                    else
                        output_chan.add(f(std::move(*value)));
                }
            }
        }
        input_channel_t input_chan;
        output_channel_t output_chan;
        std::function<typename output_channel_t::in_value_type(
            typename input_channel_t::out_value_type)>
            f;
        std::thread thread;
    };

}


template <typename _input_channel_t, typename _function_t>
struct pipeline_t
{
    using channel_tag = struct channel_tag;
    using pipeline_tag = struct pipeline_tag;
    using in_value_type = typename _input_channel_t::in_value_type;
    using full_policy_t = typename _input_channel_t::full_policy_t;
    static inline constexpr std::size_t max_size = _input_channel_t::max_size;
    using function_t = _function_t;
    using input_channel_t = _input_channel_t;
    using output_channel_t = channels::channel<decltype(std::declval<_function_t>()(
                                                   *std::declval<_input_channel_t>().take())),
        max_size, full_policy_t>;
    using out_value_type = typename output_channel_t::out_value_type;

    using impl_t = details::_pipeline<_input_channel_t, _function_t>;


    inline pipeline_t& operator<<(in_value_type&& item)
    {
        add(std::move(item));
        return *this;
    }

    inline pipeline_t& operator<<(const in_value_type& item)
    {
        add(item);
        return *this;
    }

    inline std::optional<out_value_type> take() { return m_pipeline->take(); }
    inline void add(in_value_type&& item) { m_pipeline->add(std::move(item)); }
    inline void add(const in_value_type& item) { m_pipeline->add(item); }

    inline bool closed() { return m_pipeline->closed(); }
    inline void close() { m_pipeline->close(); }

    inline std::size_t size() const noexcept { return std::size(m_pipeline); }

    pipeline_t(_input_channel_t&& input_chan, _function_t f)
            : m_pipeline { new impl_t { std::move(input_chan), f } }
    {
    }

    pipeline_t(const _input_channel_t& input_chan, _function_t f)
            : m_pipeline { new impl_t { input_chan, f } }
    {
    }

    pipeline_t(pipeline_t&& other) : m_pipeline { std::move(other.m_pipeline) } { }

    ~pipeline_t() { }

private:
    std::shared_ptr<impl_t> m_pipeline;
};

template <typename input_chan_t, typename function_t>
using pipeline_helper_t
    = decltype(channels::pipeline_t { std::declval<input_chan_t>(), std::declval<function_t>() });

template <typename T, typename = void>
constexpr bool is_channel_like_v = false;

template <typename T>
constexpr bool is_channel_like_v<T, decltype (std::declval<typename T::channel_tag>(), void()) > = true;

namespace operators {
template <typename input_channel_t, typename function_t,
    std::enable_if_t<is_channel_like_v<input_channel_t>, int> = 0>
auto operator>>(const input_channel_t& pipeline, function_t f)
{
    return channels::pipeline_t { pipeline, f };
}

template <typename input_channel_t, typename function_t,
    std::enable_if_t<is_channel_like_v<input_channel_t>, int> = 0>
auto operator>>(input_channel_t&& pipeline, function_t f)
{
    return channels::pipeline_t { std::move(pipeline), f };
}
}
}

