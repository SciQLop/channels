/*------------------------------------------------------------------------------
-- This file is a part of the SciQLop Software
-- Copyright (C) 2025, Plasma Physics Laboratory - CNRS
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
#include "traits.hpp"
#include <concepts>
#include <utility>

namespace channels
{

namespace details
{
    template <typename>
    struct callable_signature;

    template <typename R, typename... Args>
    struct callable_signature<R(Args...)>
    {
        using args = std::tuple<Args...>;
        using rtype = R;
    };

    // For function pointers
    template <typename R, typename... Args>
    struct callable_signature<R (*)(Args...)>
    {
        using args = std::tuple<Args...>;
        using rtype = R;
    };

    // For member function pointers
    template <typename C, typename R, typename... Args>
    struct callable_signature<R (C::*)(Args...)>
    {
        using args = std::tuple<Args...>;
        using rtype = R;
    };

    // For const member function pointers
    template <typename C, typename R, typename... Args>
    struct callable_signature<R (C::*)(Args...) const>
    {
        using args = std::tuple<Args...>;
        using rtype = R;
    };


    // Helper to detect the callable signature of a class with operator()
    template <typename T, typename = void>
    struct function_traits;

    // Primary template for function_traits (used for non-overloaded operator())
    template <typename T>
    struct function_traits<T, std::void_t<decltype(&T::operator())>>
    {
        using type = decltype(&T::operator());
        using args = typename callable_signature<type>::args;
        using rtype = typename callable_signature<type>::rtype;
    };

    // Helper to check if a type is callable (for overloaded operators)
    template <typename T, typename = void>
    struct is_callable : std::false_type
    {
    };

    template <typename T>
    struct is_callable<T, std::void_t<decltype(std::declval<T&>()())>> : std::true_type
    {
    };

    template <typename F>
    concept callable = std::is_function_v<std::remove_pointer_t<std::remove_reference_t<F>>>
        || std::is_member_function_pointer_v<F>
        || (std::is_class_v<F>
            && (requires { typename function_traits<F>::args; } || is_callable<F>::value));


    template <typename T>
    using return_type_t = typename callable_signature<T>::rtype;

    template <typename T>
    using first_arg_t = typename std::tuple_element<0, typename callable_signature<T>::args>::type;


} // namespace detail

template <typename T>
concept Optional = requires(T t) {
    { t.has_value() } -> std::same_as<bool>;
    { t.value() } -> std::convertible_to<typename T::value_type>;
    { t.operator*() } -> std::convertible_to<typename T::value_type>;
    { t.operator->() } -> std::convertible_to<typename T::value_type*>;
};

template <typename T>
concept _baseChannel = requires(T t) {
    { t.closed() } -> std::same_as<bool>;
    { t.close() } -> std::same_as<void>;
    { t.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept ChannelInput = requires(T t) {
    { t.add(std::declval<typename T::in_value_type>()) } -> std::same_as<void>;
    { t << std::declval<typename T::in_value_type>() } -> std::same_as<T&>;
} && _baseChannel<T>;

template <typename T>
concept ChannelOutput = requires(T t) {
    { t.take() } -> Optional;
    { *t.take() } -> std::convertible_to<typename T::out_value_type>;
} && _baseChannel<T>;

template <typename T>
concept Source = ChannelOutput<T> && !ChannelInput<T>;

template <typename T>
concept Sink = ChannelInput<T> && !ChannelOutput<T>;

template <typename T>
concept FunctionThatTakesVoid
    = details::callable<T> && std::tuple_size_v<typename details::callable_signature<T>::args> == 0;

template <typename T>
concept FunctionThatReturnsVoid
    = details::callable<T> && std::is_same_v<typename details::callable_signature<T>::rtype, void>;

template <typename T>
concept FunctionThatTakesOnlyOneArgument
    = details::callable<T> && std::tuple_size_v<typename details::callable_signature<T>::args> == 1;

template <typename T>
concept SourceCompatibleCallable = FunctionThatTakesVoid<T> && !FunctionThatReturnsVoid<T>;


template <typename T>
concept SinkCompatibleCallable = FunctionThatReturnsVoid<T> && !FunctionThatTakesVoid<T>
    && FunctionThatTakesOnlyOneArgument<T>;


template <typename T>
concept ChannelConcept = ChannelInput<T> && ChannelOutput<T>;

template <typename T, typename U>
concept CompatibleCallable
    = std::is_convertible_v<typename details::callable_signature<U>::rtype,
          typename std::tuple_element<0, typename details::callable_signature<T>::args>::type>
    && std::tuple_size_v<typename details::callable_signature<T>::args> == 1;


template <typename T, typename U>
concept SourceToChannelCompatible
    = SourceCompatibleCallable<T> && ChannelInput<U> && std::is_same_v<traits::return_type_t<T> , typename U::in_value_type>;

template <typename T, typename U>
concept ChannelToSinkCompatible
    = ChannelOutput<T> && SinkCompatibleCallable<U>
    && std::is_same_v<details::first_arg_t<U>, typename T::out_value_type>;


template <typename T>
concept CanProduce
    = ChannelOutput<T> || SourceCompatibleCallable<T> || !FunctionThatReturnsVoid<T>;

template <typename T>
concept CanConsume
    = ChannelInput<T> || SinkCompatibleCallable<T> || (!FunctionThatTakesVoid<T> && FunctionThatTakesOnlyOneArgument<T>);

}

#ifdef CHANNELS_COMPILE_TIME_TESTS

namespace channels
{

namespace details
{
    static_assert(callable<int(int)>);
    static_assert(callable<void(int, double)>);
    static_assert(callable<std::plus<int>>);
    static_assert(!callable<int>);
    static_assert(std::is_same_v<callable_signature<int(void)>::rtype, int>);
    static_assert(std::is_same_v<callable_signature<void(int)>::args, std::tuple<int>>);
    static_assert(std::is_same_v<callable_signature<void(int)>::rtype, void>);
    static_assert(
        std::is_same_v<callable_signature<void(int, double)>::args, std::tuple<int, double>>);

    struct _dummy
    {
        void operator()(int) { }
    };
    static_assert(callable<_dummy>);
}

static_assert(SourceCompatibleCallable<int(void)>);
static_assert(!SourceCompatibleCallable<int(int)>);
static_assert(!SourceCompatibleCallable<int(int, double)>);
static_assert(!SourceCompatibleCallable<void(void)>);


static_assert(SinkCompatibleCallable<void(int)>);
static_assert(!SinkCompatibleCallable<int(void)>);
static_assert(!SinkCompatibleCallable<void(void)>);
static_assert(!SinkCompatibleCallable<void(int, double)>);


static_assert(!CompatibleCallable<int(int), void(double)>);
static_assert(CompatibleCallable<int(int), int(void)>);
static_assert(
    CompatibleCallable<int(std::tuple<int, double, char>), std::tuple<int, double, char>(int)>);
static_assert(!CompatibleCallable<int(int, double), int(double, int)>);

} // namespace channels::tests

#endif
