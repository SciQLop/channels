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
#include "tags.hpp"
#include "traits.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>

namespace channels
{

namespace details
{

    template <typename _output_channel_t>
    struct _output_pipeline_interface
    {
        using tag = source_pipeline_tag;
        using output_channel_t = _output_channel_t;
        using out_value_type = typename output_channel_t::out_value_type;
        output_channel_t output_chan;

        inline std::optional<out_value_type> take() { return output_chan.take(); }
        virtual bool closed() { return output_chan.closed(); }
        virtual void close() { output_chan.close(); }
        inline std::size_t size() const noexcept { return std::size(output_chan); }

        _output_pipeline_interface() = default;
        _output_pipeline_interface(_output_channel_t&& output_chan)
                : output_chan { std::move(output_chan) }
        {
        }
        _output_pipeline_interface(_output_channel_t& output_chan) : output_chan { output_chan } { }

        virtual ~_output_pipeline_interface()
        {
            if (!output_chan.closed())
                output_chan.close();
        }
    };

    template <typename _input_channel_t>
    struct _input_pipeline_interface
    {
        using tag = source_pipeline_tag;
        using input_channel_t = _input_channel_t;
        using in_value_type = typename input_channel_t::out_value_type;

        input_channel_t input_chan;

        inline _input_pipeline_interface& operator<<(in_value_type&& item)
        {
            add(std::move(item));
            return *this;
        }

        inline _input_pipeline_interface& operator<<(const in_value_type& item)
        {
            add(item);
            return *this;
        }

        inline void add(in_value_type&& item) { input_chan.add(std::move(item)); }
        inline void add(const in_value_type& item) { input_chan.add(item); }

        virtual bool closed() { return input_chan.closed(); }

        virtual void close() { input_chan.close(); }


        _input_pipeline_interface(_input_channel_t&& input_chan)
                : input_chan { std::move(input_chan) }
        {
        }

        _input_pipeline_interface(_input_channel_t& input_chan) : input_chan { input_chan } { }

        virtual ~_input_pipeline_interface()
        {
            if (!input_chan.closed())
                input_chan.close();
        }
    };

    template <typename _input_channel_t, typename _function_t>
    struct filter_impl
            : _input_pipeline_interface<_input_channel_t>,
              _output_pipeline_interface<channel<decltype(std::declval<_function_t>()(
                                                     *std::declval<_input_channel_t>().take())),
                  _input_channel_t::max_size, typename _input_channel_t::full_policy_t>>
    {
        using tag = pipeline_tag;
        using input_pipeline_interface_t = _input_pipeline_interface<_input_channel_t>;

        using output_pipeline_interface_t = _output_pipeline_interface<
            channel<decltype(std::declval<_function_t>()(*std::declval<_input_channel_t>().take())),
                _input_channel_t::max_size, typename _input_channel_t::full_policy_t>>;

        using typename input_pipeline_interface_t::in_value_type;

        using input_channel_t = _input_channel_t;

        using output_channel_t = typename output_pipeline_interface_t::output_channel_t;
        using out_value_type = typename output_channel_t::out_value_type;

        virtual bool closed() final
        {
            return input_pipeline_interface_t::closed() or output_pipeline_interface_t::closed();
        }

        virtual void close() final
        {
            input_pipeline_interface_t::close();
            output_pipeline_interface_t::close();
        }


        filter_impl(_input_channel_t&& input_chan, _function_t f)
                : input_pipeline_interface_t { std::move(input_chan) }
                , output_pipeline_interface_t {}
                , f { f }
                , thread { &filter_impl::loop, this }
        {
        }

        filter_impl(const _input_channel_t& input_chan, _function_t f)
                : input_pipeline_interface_t { input_chan }
                , output_pipeline_interface_t {}
                , f { f }
                , thread { &filter_impl::loop, this }
        {
        }

        filter_impl(filter_impl&& other) = delete;

        ~filter_impl()
        {
            if (!this->input_chan.closed())
                this->input_chan.close();
            if (!this->output_chan.closed())
                this->output_chan.close();
            if (thread.joinable())
                thread.join();
        }

    private:
        void loop()
        {

            while (!this->input_chan.closed() && !this->output_chan.closed())
            {
                if (auto value = this->input_chan.take(); value)
                {
                    if constexpr (std::is_pointer_v<decltype(*value)>)
                        this->output_chan.add(f(*value));
                    else
                        this->output_chan.add(f(std::move(*value)));
                }
            }
        }

        std::function<typename output_channel_t::in_value_type(
            typename input_channel_t::out_value_type)>
            f;
        std::thread thread;
    };

    struct dummy_source
    {
        inline bool closed() const { return false; }
    };

    template <typename T, typename U = void>
    struct in_value_type;

    template <typename T>
    struct in_value_type<T, std::enable_if_t<traits::is_callable<T>::value, void>>
    {
        using type = traits::return_type_t<T>;
    };

    template <typename T>
    struct in_value_type<T, std::enable_if_t<!traits::is_callable<T>::value, void>>
    {
        using type = typename T::in_value_type;
    };

    template <typename _function_t, typename _source_t = dummy_source>
    struct source_impl
            : _output_pipeline_interface<channel<typename in_value_type<_function_t>::type>>
    {
        //        using out_value_type = decltype(std::declval<_function_t>()());

        template <typename Dummy = void,
            typename = std::enable_if_t<std::is_same_v<_source_t, dummy_source>, Dummy>>
        source_impl(_function_t f) : f { f }, thread { &source_impl::loop<_source_t>, this }
        {
        }

        template <typename Dummy = void,
            typename = std::enable_if_t<!std::is_same_v<_source_t, dummy_source>, Dummy>>
        source_impl(_source_t source, _function_t f)
                : f { f }, thread { &source_impl::loop<_source_t>, this }, source { source }
        {
        }


    private:
        template <typename source_t = _source_t>
        typename std::enable_if_t<std::is_same_v<source_t, dummy_source>, void> loop()
        {
            while (!this->output_chan.closed())
            {
                this->output_chan.add(f());
            }
        }

        template <typename source_t = _source_t>
        typename std::enable_if_t<!std::is_same_v<source_t, dummy_source>, void> loop()
        {
            while (!this->output_chan.closed() && !source.closed())
            {
                if (auto value = this->source.take(); value)
                {
                    if constexpr (std::is_pointer_v<decltype(*value)>)
                        this->output_chan.add(f(*value));
                    else
                        this->output_chan.add(f(std::move(*value)));
                }
            }
        }

        _function_t f;
        std::thread thread;
        _source_t source;
    };

    template <typename _source_t, typename _sink_t>
    struct full_pipeline_impl
    {
        full_pipeline_impl(_source_t _source, _sink_t _sink)
                : m_source { _source }, m_sink { _sink }, thread { &full_pipeline_impl::loop, this }
        {
        }


    private:
        void loop()
        {
            while (!m_source.closed() and !m_sink.closed())
            {
                if (auto value = m_source.take(); value)
                {
                    if constexpr (std::is_pointer_v<decltype(*value)>)
                        m_sink.add(*value);
                    else
                        m_sink.add(std::move(*value));
                }
            }
        }

        _source_t m_source;
        _sink_t m_sink;
        std::thread thread;
    };


    template <typename _function_t>
    struct sink_impl : _input_pipeline_interface<channel<traits::arg_type_t<_function_t>>>
    {
        using in_value_type = traits::arg_type_t<_function_t>;
        sink_impl(_function_t f) : f { f }, thread { &sink_impl::loop, this } { }

    private:
        void loop()
        {
            while (!this->input_chan.closed())
            {
                f(this->input_chan.take());
            }
        }
        _function_t f;
        std::thread thread;
    };

}


template <typename _input_channel_t, typename _function_t>
struct filter
{
    using tag = pipeline_tag;
    using in_value_type = typename _input_channel_t::in_value_type;
    using full_policy_t = typename _input_channel_t::full_policy_t;
    static inline constexpr std::size_t max_size = _input_channel_t::max_size;
    using function_t = _function_t;
    using input_channel_t = _input_channel_t;
    using output_channel_t = channels::channel<decltype(std::declval<_function_t>()(
                                                   *std::declval<_input_channel_t>().take())),
        max_size, full_policy_t>;
    using out_value_type = typename output_channel_t::out_value_type;

    using impl_t = details::filter_impl<_input_channel_t, _function_t>;


    inline filter& operator<<(in_value_type&& item)
    {
        add(std::move(item));
        return *this;
    }

    inline filter& operator<<(const in_value_type& item)
    {
        add(item);
        return *this;
    }

    inline std::optional<out_value_type> take() { return m_filter->take(); }
    inline void add(in_value_type&& item) { m_filter->add(std::move(item)); }
    inline void add(const in_value_type& item) { m_filter->add(item); }

    inline bool closed() { return m_filter->closed(); }
    inline void close() { m_filter->close(); }

    inline std::size_t size() const noexcept { return std::size(m_filter); }

    filter(_input_channel_t&& input_chan, _function_t f)
            : m_filter { new impl_t { std::move(input_chan), f } }
    {
    }

    filter(const _input_channel_t& input_chan, _function_t f)
            : m_filter { new impl_t { input_chan, f } }
    {
    }

    filter(filter&& other) : m_filter { std::move(other.m_filter) } { }

    ~filter() { }

private:
    std::shared_ptr<impl_t> m_filter;
};

template <typename input_chan_t, typename function_t>
using pipeline_helper_t
    = decltype(channels::filter { std::declval<input_chan_t>(), std::declval<function_t>() });


template <typename function_t, typename source_t = details::dummy_source>
struct source
{
    using tag
        = std::conditional_t<traits::is_sink_function_v<function_t>, full_pipeline_tag, source_tag>;
    static constexpr bool is_full_pipeline = traits::is_sink_function_v<function_t>;
    // using out_value_type = decltype(std::declval<function_t>()());
    using impl_t = details::source_impl<function_t, source_t>;

    template <typename Dummy = void,
        typename = std::enable_if_t<std::is_same_v<source_t, details::dummy_source>, Dummy>>
    source(function_t f) : m_source { new impl_t { f } }
    {
    }
    template <typename Dummy = void,
        typename = std::enable_if_t<std::is_same_v<source_t, details::dummy_source>, Dummy>>
    source(source&& other) : m_source { std::move(other.m_source) }
    {
    }

    template <typename Dummy = void,
        typename = std::enable_if_t<!std::is_same_v<source_t, details::dummy_source>, Dummy>>
    source(source_t other, function_t f) : m_source { new impl_t { other, f } }
    {
    }

    template <bool en = is_full_pipeline>
    inline std::enable_if_t<!en, decltype(std::declval<impl_t>().take())> take()
    {
        return m_source->take();
    }

    inline bool closed() { return m_source->closed(); }
    inline void close() { m_source->close(); }

private:
    std::shared_ptr<impl_t> m_source;
};

template <typename function_t>
struct sink
{
    static_assert(traits::is_sink_function_v<function_t>,
        "You must pass a function with this signature void(Args...)");
    using tag = struct sink_tag;
    using in_value_type = std::remove_cv_t<std::remove_reference_t<traits::arg_type_t<function_t>>>;
    using impl_t = details::sink_impl<function_t>;

    sink(function_t f) : m_sink { new impl_t { f } } { }
    sink(sink&& other) : m_sink { std::move(other.m_sink) } { }
    sink(const sink& other) : m_sink { other.m_sink } { }

    inline sink& operator<<(in_value_type&& item)
    {
        add(std::move(item));
        return *this;
    }

    inline sink& operator<<(const in_value_type& item)
    {
        add(item);
        return *this;
    }

    inline void add(in_value_type&& item) { m_sink->add(std::move(item)); }
    inline void add(const in_value_type& item) { m_sink->add(item); }

    inline bool closed() { return m_sink->closed(); }
    inline void close() { m_sink->close(); }

private:
    std::shared_ptr<impl_t> m_sink;
};

template <typename _source_t, typename _sink_t>
struct full_pipeline
{
    using tag = struct full_pipeline_tag;
    using impl_t = details::full_pipeline_impl<_source_t, _sink_t>;

    full_pipeline(_source_t _source, _sink_t _sink)
            : m_full_pipeline { new impl_t { _source, _sink } }
    {
    }
    full_pipeline(full_pipeline&& other) : m_full_pipeline { std::move(other.m_full_pipeline) } { }


    inline bool closed() { return m_full_pipeline->closed(); }
    inline void close() { m_full_pipeline->close(); }

private:
    std::shared_ptr<impl_t> m_full_pipeline;
};

namespace operators
{
    template <typename lhs_t, typename rhs_t,
        std::enable_if_t<traits::is_source_function_v<std::decay_t<
                             lhs_t>> and traits::is_callable<std::decay_t<rhs_t>>::value
                and traits::are_compatible_v<std::decay_t<lhs_t>, std::decay_t<rhs_t>>,
            int> = 0>
    auto operator>>(lhs_t&& lhs, rhs_t&& rhs)
    {
        if constexpr (traits::is_sink_function_v<std::decay_t<rhs_t>>)
            return full_pipeline(source(std::forward<lhs_t>(lhs)), sink(std::forward<rhs_t>(rhs)));
        else
            return source(source(std::forward<lhs_t>(lhs)), std::forward<rhs_t>(rhs));
    }


    template <typename lhs_t, typename rhs_t,
        std::enable_if_t<!traits::is_callable<std::decay_t<lhs_t>>::value
                and traits::has_output_chan_interface_v<std::decay_t<lhs_t>>,
            int> = 0>
    auto operator>>(lhs_t&& lhs, rhs_t&& rhs)
    {
        if constexpr (traits::has_input_chan_interface_v<std::decay_t<
                          lhs_t>> and traits::has_output_chan_interface_v<std::decay_t<lhs_t>>)
            return filter(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
        else
        {
            if constexpr (traits::is_sink_function_v<std::decay_t<rhs_t>>)
                return full_pipeline(std::forward<lhs_t>(lhs), sink(std::forward<rhs_t>(rhs)));
            else
            {
                if constexpr (traits::has_output_chan_interface_v<std::decay_t<
                                  rhs_t>> or traits::is_callable<std::decay_t<rhs_t>>::value)
                    return source(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));

                else
                    return full_pipeline(std::forward<lhs_t>(lhs), std::forward<rhs_t>(rhs));
            }
        }
    }

}


}
