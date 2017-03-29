//
// Created by nohajc on 28.3.17.
//

#ifndef R_3_3_1_TUPLE_FOR_EACH_H
#define R_3_3_1_TUPLE_FOR_EACH_H

#include <tuple>

namespace {
    template<int I, typename T, typename F>
    struct for_each_impl {
        static void for_each(T &t, F fn) {
            fn(std::get<std::tuple_size<T>::value - I>(t));
            for_each_impl<I - 1, T, F>::for_each(t, fn);
        }
    };

    template<typename T, typename F>
    struct for_each_impl<1, T, F> {
        static void for_each(T &t, F fn) {
            fn(std::get<std::tuple_size<T>::value - 1>(t));
        }
    };
}

template<typename F, typename ...T>
void tuple_for_each(std::tuple<T...> tp, F fn) {
    return for_each_impl<std::tuple_size<decltype(tp)>::value, decltype(tp), F>::for_each(tp, fn);
}

#endif //R_3_3_1_TUPLE_FOR_EACH_H
