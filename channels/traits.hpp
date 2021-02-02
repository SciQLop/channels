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
#include "tags.hpp"
#include <tuple>

namespace channels::traits
{
template <typename T, typename = void>
constexpr bool is_channel_like_v = false;

template <typename T>
constexpr bool is_channel_like_v<T,
    decltype(std::declval<typename T::tag>(),
        void())> = std::is_same_v<typename T::tag,
                       channel_tag> or std::is_same_v<typename T::tag, pipeline_tag>;


template <typename T, typename = void>
constexpr bool has_input_chan_interface_v = false;

template <typename T>
constexpr bool has_input_chan_interface_v<T,
    decltype(std::declval<T>().add(std::declval<typename T::in_value_type>()), void())> = true;

template <typename T, typename = void>
constexpr bool has_output_chan_interface_v = false;

template <typename T>
constexpr bool has_output_chan_interface_v<T, decltype(std::declval<T>().take(), void())> = true;

// https://stackoverflow.com/questions/15393938/find-out-whether-a-c-object-is-callable
template <typename T, typename U = void>
struct is_callable
{
    static bool const constexpr value
        = std::conditional_t<std::is_class<std::remove_reference_t<T>>::value,
            is_callable<std::remove_reference_t<T>, int>, std::false_type>::value;
};

template <typename T, typename U, typename... Args>
struct is_callable<T(Args...), U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T (*)(Args...), U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T (&)(Args...), U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......), U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T (*)(Args......), U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T (&)(Args......), U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) const, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) volatile, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) const volatile, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) const, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) volatile, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) const volatile, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...)&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) const&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) volatile&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) const volatile&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......)&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) const&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) volatile&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) const volatile&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...)&&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) const&&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) volatile&&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args...) const volatile&&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......)&&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) const&&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) volatile&&, U> : std::true_type
{
};
template <typename T, typename U, typename... Args>
struct is_callable<T(Args......) const volatile&&, U> : std::true_type
{
};

template <typename T>
struct is_callable<T, int>
{
private:
    using YesType = char (&)[1];
    using NoType = char (&)[2];

    struct Fallback
    {
        void operator()();
    };

    struct Derived : T, Fallback
    {
    };

    template <typename U, U>
    struct Check;

    template <typename>
    static YesType Test(...);

    template <typename C>
    static NoType Test(Check<void (Fallback::*)(), &C::operator()>*);

public:
    static bool const constexpr value = sizeof(Test<Derived>(0)) == sizeof(YesType);
};

template <typename R, typename... Args>
struct signature_final
{
    using return_type = R;
    using argument_type = std::tuple<Args...>;
};

template <typename T>
struct signature_impl : signature_impl<decltype(&T::operator())>
{
};

template <typename R, typename... Args>
struct signature_impl<R(Args...)&> : signature_impl<R(Args...)>
{
};

template <typename R, typename... Args>
struct signature_impl<R(Args...)> : signature_final<R, Args...>
{
};

template <typename R, typename... Args>
struct signature_impl<R(*)(Args...)> : signature_final<R, Args...>
{
};

template <typename T, typename R, typename... Args>
struct signature_impl<R (T::*)(Args...)> : signature_impl<R(Args...)>
{
};

template <typename T, typename R, typename... Args>
struct signature_impl<R (T::*)(Args...)&> : signature_impl<R(Args...)>
{
};

template <typename T, typename R, typename... Args>
struct signature_impl<R (T::*)(Args...) const> : signature_impl<R(Args...)>
{
};

template <typename T>
struct signature : signature_impl<T>
{
};


template <typename T>
using return_type_t = typename signature<T>::return_type;

template <typename T, std::size_t index = 0>
using arg_type_t = std::tuple_element_t<index, typename signature<T>::argument_type>;

template <typename T, bool U>
struct returns_void_impl_t;

template <typename T>
struct returns_void_impl_t<T, false> : std::false_type
{
};

template <typename T>
struct returns_void_impl_t<T, true> : std::is_same<return_type_t<T>, void>::type
{
};


template <typename T>
struct returns_void_t : returns_void_impl_t<T, is_callable<T>::value>
{
};

template <typename T>
constexpr bool returns_void_v = returns_void_t<T>::value;


template <typename T, bool U>
struct takes_void_impl_t;

template <typename T>
struct takes_void_impl_t<T, false> : std::false_type
{
};

template <typename T>
struct takes_void_impl_t<T, true>
        : std::is_same<typename signature<T>::argument_type, std::tuple<>>::type
{
};

template <typename T>
struct takes_void_t : takes_void_impl_t<T, std::is_invocable_v<T>>
{
};

template <typename T>
constexpr bool takes_void_v = takes_void_t<T>::value;


template <typename T>
constexpr bool is_source_function_v = takes_void_v<T> and !returns_void_v<T>;

template <typename T>
constexpr bool is_sink_function_v = !takes_void_v<T> and returns_void_v<T>;


template <typename T, typename U, bool V>
struct are_compatible_impl;

template <typename T, typename U>
struct are_compatible_impl<T, U, false> : std::false_type
{
};


template <typename T, typename U>
struct are_compatible_impl<T, U, true>
        : std::integral_constant<bool,
              std::is_same_v<return_type_t<T>,
                  std::remove_cv_t<std::remove_reference_t<arg_type_t<U>>>>>
{
};


template <typename T, typename U>
struct are_compatible_t : are_compatible_impl<T, U, is_callable<T>::value and is_callable<U>::value>
{
};


template <typename function1_t, typename function2_t>
constexpr bool are_compatible_v = are_compatible_t<function1_t, function2_t>::value;

}
