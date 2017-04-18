//
// Created by nohajc on 17.3.17.
//

#ifndef R_3_3_1_RDT_REGISTER_HOOK_H
#define R_3_3_1_RDT_REGISTER_HOOK_H

#include <utility>
#include "rdt.h"

// TODO: remove this header

//#define DECL_HOOK(hook_name) \
//    struct hook_name { \
//        template<typename T> \
//        inline static void init(rdt_handler& h) { \
//            h.probe_##hook_name = T::fn_##hook_name; \
//        } \
//    }; \
//    static void fn_##hook_name
//
//#define REGISTER_HOOKS(tracer_name, ...) []() { \
//        typedef tracer_name tr; \
//        rdt_handler h = {0}; \
//        register_hooks<tr, __VA_ARGS__>::do_it(h); \
//        return h; \
//    }();
//
//template<typename T, typename ...Hs>
//struct register_hooks;
//
//template<typename T, typename H, typename ...Hs>
//struct register_hooks<T, H, Hs...> {
//    inline static void do_it(rdt_handler &h) {
//        H::template init<T>(h);
//        register_hooks<T, Hs...>::do_it(h);
//    }
//};
//
//template<typename T>
//struct register_hooks<T> {
//    inline static void do_it(rdt_handler& h) {}
//};

#endif //R_3_3_1_RDT_REGISTER_HOOK_H
