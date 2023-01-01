/*
 * High-level tee. Any class that supports "<<" will work.
 * Minimum C++ version: C++11
 */
#pragma once

#include <tuple>
#include <utility>

// Iterate through the members of Tuple, recursively
template<typename Tuple, std::size_t rest>
struct _TeeHelper {
    template<typename T>
    static void output(Tuple &&t, T &&arg) {
        std::get<std::tuple_size<Tuple>::value - rest>(t) 
            << std::forward<T>(arg);
        _TeeHelper<Tuple, rest - 1>::output(std::forward<Tuple>(t), 
            std::forward<T>(arg));
    }
};

// Base case
template<typename Tuple>
struct _TeeHelper<Tuple, 0> {
    template<typename T>
    static void output(Tuple &&t, T &&arg) {(void)t; (void)arg;}
};

template<typename... Streams>
class Tee {
    typedef std::tuple<Streams...> Tuple;
    Tuple s;
public:
    Tee(Streams&&... streams): s(std::forward_as_tuple(streams...)) {}

    template<typename T>
    Tee<Streams...> &operator<<(T &&arg) {
        _TeeHelper<Tuple, std::tuple_size<Tuple>::value>::
            output(std::forward<Tuple>(s), arg);
        return *this;
    }
};
