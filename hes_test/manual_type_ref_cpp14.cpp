/// @file manual_type_ref_cpp14.cpp
/// @brief 
/// @authors 
/// Compile with: g++ -std=c++14 -I../include manual_type_ref_cpp14.cpp -o bin/manual_type_ref_cpp14
#include <tuple>
#include <cstdint>
#include <utility>
#include <iostream>
#include <type_traits>
#include <initializer_list>

template<typename...>
using void_t = void;

template<typename>
struct type {};

template<std::size_t>
struct ubiq
{
    template<typename T>
    operator T() const;
};


template<typename, typename, typename = void>
struct _size_impl
{};

template<typename T, std::size_t head, std::size_t... tail>
struct _size_impl<T, std::index_sequence<head, tail...>,
    void_t<decltype(T{ubiq<head>{}, ubiq<tail>{}...})>
> :
    std::integral_constant<std::size_t, sizeof...(tail) + 1>
{};

template<typename T, std::size_t head, std::size_t... tail, typename _>
struct _size_impl<T, std::index_sequence<head, tail...>, _> :
    _size_impl<T, std::index_sequence<tail...>>
{};

template<typename T>
struct _size :
    _size_impl<T, std::make_index_sequence<sizeof(T)>>
{};

template<typename T>
using size = typename _size<T>::type;

template<typename F>
struct deducer
{
    F f;

    template<typename T>
    operator T() const {
        f(type<T>{});
        return {};
    }
};

template<typename F>
deducer<std::decay_t<F>> deduce(F&& f) {
    return {std::forward<F>(f)};
}

template<typename F, typename T, typename... Props>
auto as_tuple_impl(F&& f, T&& t, type<Props>...)
    -> std::enable_if_t<size<std::decay_t<T>>::value != sizeof...(Props)> {
    std::decay_t<T>{
        Props{}...,
        deduce(
            [&f, &t](auto prop) {
                as_tuple_impl(
                    std::forward<F>(f),
                    std::forward<T>(t),
                    type<Props>{}...,
                    prop
                );
            }
        )
    };
}

template<typename F, typename T, typename... Props>
auto as_tuple_impl(F&& f, T&& t, type<Props>...)
    -> std::enable_if_t<size<std::decay_t<T>>::value == sizeof...(Props)> {
    std::forward<F>(f)(reinterpret_cast<std::tuple<Props...>&&>(t));
}

template<typename F, typename T, typename... Props>
auto as_tuple_impl(F&& f, T& t, type<Props>...)
    -> std::enable_if_t<size<T>::value == sizeof...(Props)> {
    std::forward<F>(f)(reinterpret_cast<std::tuple<Props...>&>(t));
}

template<typename T, typename F>
auto as_tuple(T&& t, F&& f) {
    as_tuple_impl(std::forward<F>(f), std::forward<T>(t));
}

template<typename... R, std::size_t h, std::size_t... t>
void print_tuple_impl(std::tuple<R...> const& r, std::index_sequence<h, t...>) {
    std::cout << '{' << std::get<h>(r);
    std::initializer_list<int>{
        (void(std::cout << ", " << std::get<t>(r)), 0)...
    };
    std::cout << '}';
}

template<typename... T>
void print_tuple(std::tuple<T...> const& t) {
    print_tuple_impl(t, std::index_sequence_for<T...>{});
}

enum class color {
    red,
    green,
    blue
};

struct reg {
    int a;
    char b;
    double d;
    void* e;
    color f;
    std::string g;
};



std::ostream& operator <<(std::ostream& os, color c) {
    switch(c) {
        case color::red:
            os << "red";
            break;
        case color::green:
            os << "green";
            break;
        case color::blue:
            os << "blue";
            break;
    };

    return os;
}

int main() {

    as_tuple(reg{42, 'a', 3.1, nullptr, color::green, "hello world!"}, [](auto&& t) {
        print_tuple(std::forward<decltype(t)>(t));
        std::cout << std::endl;
    });

}