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
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>

namespace channels
{
namespace full_policy
{
    struct wait_for_space
    {
    };
    struct overwrite_last
    {
    };
}
namespace details
{

    template <typename ref_type, typename... types>
    inline constexpr bool is_any_of_v = (std::is_same_v<ref_type, types> || ...);

    template <typename T, std::size_t max_size, typename full_policy_t>
    struct _fixed_size_queue
    {
        static_assert(
            is_any_of_v<full_policy_t, full_policy::overwrite_last, full_policy::wait_for_space>,
            "Unknown policy");

        inline void add(const T& item)
        {
            T copy { item };
            add(std::move(copy));
        }

        inline void add(T&& item)
        {
            using namespace full_policy;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if constexpr (std::is_same_v<full_policy_t, wait_for_space>)
                {
                    m_cv.wait(lock, [this]() { return !full() || m_closed.load(); });
                }
                else if constexpr (std::is_same_v<full_policy_t, overwrite_last>)
                {
                    if (full())
                        m_queue.pop_back();
                }
            }
            if (!closed())
            {
                m_queue.push_back(std::move(item));
                m_cv.notify_one();
            }
        }

        inline std::optional<T> take()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return !empty() || m_closed.load(); });
            if (m_closed.load())
                return std::nullopt; // maybe optional!
            T item = m_queue.front();
            m_queue.pop_front();
            lock.unlock();
            m_cv.notify_one();
            return std::optional<T> { item };
        }

        inline bool full() const noexcept { return size() >= (max_size); }
        inline bool empty() const noexcept { return size() == 0U; }
        inline std::size_t size() const noexcept { return std::size(m_queue); }

        inline bool closed() { return m_closed.load(); }
        inline void close()
        {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_closed.store(true);
            }
            m_cv.notify_all();
        }

    private:
        std::deque<T> m_queue;
        std::mutex m_mutex;
        std::atomic<bool> m_closed { false };
        std::condition_variable m_cv;
    };
}


template <typename _value_type, std::size_t _max_size,
    typename _full_policy_t = full_policy::wait_for_space>
struct channel
{
    using channel_tag = struct channel_tag;
    using in_value_type = _value_type;
    using out_value_type = _value_type;
    using full_policy_t = _full_policy_t;
    static inline constexpr std::size_t max_size = _max_size;
    using queue_t = details::_fixed_size_queue<_value_type, _max_size, _full_policy_t>;

    channel() : m_queue { new queue_t } {};
    channel(const channel& other) : m_queue { other.m_queue } {};
    channel(channel&& other) : m_queue { std::move(other.m_queue) } {};
    channel& operator=(const channel&) = delete;
    channel& operator=(channel&&) = delete;

    inline channel& operator<<(in_value_type&& item)
    {
        add(std::move(item));
        return *this;
    }

    inline channel& operator<<(const in_value_type& item)
    {
        add(item);
        return *this;
    }

    inline std::optional<out_value_type> take() { return m_queue->take(); }
    inline void add(in_value_type&& item) { m_queue->add(std::move(item)); }
    inline void add(const in_value_type& item) { m_queue->add(item); }

    inline bool closed() { return m_queue->closed(); };
    inline void close() { m_queue->close(); }

    inline std::size_t size() const noexcept { return std::size(*m_queue); }

private:
    std::shared_ptr<queue_t> m_queue;
};
}
/*
namespace details
{

    template <typename _input_channel_t, typename _function_t>
    struct _pipeline
    {
        using pipeline_tag = struct pipeline_tag;
        using in_value_type = typename _input_channel_t::out_value_type;
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
            this->start_thread();
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
                    output_chan.add(f(std::move(*value)));
            }
        }
        input_channel_t input_chan;
        output_channel_t output_chan;
        std::function<typename input_channel_t::out_value_type(
            typename output_channel_t::in_value_type)>
            f;
        std::thread thread;
    };

}


template <typename _input_channel_t, typename _function_t>
struct pipeline_t
{
    using channel_tag = struct channel_tag;
    using pipeline_tag = struct pipeline_tag;
    using in_value_type = typename _input_channel_t::out_value_type;
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
}

template <typename input_chan_t, typename function_t>
using pipeline_helper_t
    = decltype(channels::pipeline_t { std::declval<input_chan_t>(), std::declval<function_t>() });

template <typename T, typename = void>
constexpr bool is_channel_like_v = false;

template <typename T>
constexpr bool is_channel_like_v<T, decltype (std::declval<typename T::channel_tag>(), void()) > = true;


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
*/
