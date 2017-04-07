//
// Created by nohajc on 3.4.17.
//

#include <cstdlib>
#include <cstdarg>
// Should work on Linux and macOS
#include <dlfcn.h>
#include <cstdio>
#include <assert.h>

#include "dyn_fn_lookup.h"

static void * rdt_dll_handle = NULL;

void init_dll_handle(void * handle) {
    rdt_dll_handle = handle;
}

void * find_fn_by_name(const char * format, ...) {
    if (rdt_dll_handle) {
        char * fn_name;
        va_list args;
        va_start(args, format);
        int outcome = vasprintf(&fn_name, format, args);
        assert(outcome > 0);
        va_end(args);

        // TODO: How about caching the pointers so we don't have to
        // do dlsym everytime we call Rdt with a previously used tracer?
        void * fn_ptr = dlsym(rdt_dll_handle, fn_name);
        free(fn_name);
        return fn_ptr;
    }
    return NULL;
}
