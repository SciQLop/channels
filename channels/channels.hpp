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
#include <queue>
#include <type_traits>
#include <optional>

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
            if(!m_closed.load())
            {
                m_queue.push_back(std::move(item));
                m_cv.notify_one();
            }
        }

        inline std::optional<T> take()
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return !empty() || m_closed.load(); });
            if(m_closed.load())
                return std::nullopt; // maybe optional!
            T item = m_queue.front();
            m_queue.pop_front();
            lock.unlock();
            m_cv.notify_one();
            return std::optional<T>{item};
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

template <typename T, std::size_t max_size, typename full_policy_t = full_policy::wait_for_space>
struct channel
{
    using value_type = T;

    channel() = default;
    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;

    inline channel& operator<<(T&& item)
    {
        add(std::move(item));
        return *this;
    }

    inline channel& operator<<(const T& item)
    {
        add(item);
        return *this;
    }

    inline std::optional<T> take() { return m_queue.take(); }
    inline void add(T&& item) { m_queue.add(std::move(item)); }
    inline void add(const T& item) { m_queue.add(item); }

    inline bool closed() { return m_queue.closed(); };
    inline void close() { m_queue.close(); }

    inline std::size_t size() const noexcept { return std::size(m_queue); }

private:
    details::_fixed_size_queue<T, max_size, full_policy_t> m_queue;
};

}
