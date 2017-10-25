//
// Created by nohajc on 3.4.17.
//

#include <string>
#include <regex>
#include <set>

#include <cstdlib>
#include <cstdarg>
// Should work on Linux and macOS
#include <dlfcn.h>
#include <cstdio>
#include <assert.h>

#include "rdt.h"

#include "dyn_fn_lookup.h"

#ifdef __APPLE__
static const std::string LIB_EXT = ".dylib";
#elif __linux__
static const std::string LIB_EXT = ".so";
#else
#error "Only OS X and Linux supported."
#endif

static void * rdt_dll_handle = NULL;

//static std::set<void*> loaded_libs;


void init_dll_handle(void * handle) {
    rdt_dll_handle = handle;
}

static std::string infer_lib_location(const std::string &path, const std::string &fn_name) {
    std::regex tracer_name_ex("[^_]*_([^_]*)_.*");
    std::smatch tracer_name_match;
    std::string tracer_name;

    if (std::regex_match(fn_name, tracer_name_match, tracer_name_ex)) {
        if (tracer_name_match.size() == 2) {
            tracer_name = tracer_name_match[1].str().c_str();
            return path + "/" + tracer_name + "/lib/librdt-" + tracer_name + LIB_EXT;
        }
    }

    return "";
}

void * find_fn_by_name(const char * format, ...) {
    void * fn_ptr = NULL;
    char * fn_name;
    va_list args;
    va_start(args, format);
    int outcome = vasprintf(&fn_name, format, args);
    assert(outcome > 0);
    va_end(args);

    if (rdt_dll_handle) {
        // TODO: How about caching the pointers so we don't have to
        // do dlsym everytime we call Rdt with a previously used tracer?
        fn_ptr = dlsym(rdt_dll_handle, fn_name);
    }

    if (!fn_ptr) {
        // Try to load the function from a plugin library
        SEXP path = findVar(install(".RdtPlugins"), R_GlobalEnv);
        if (TYPEOF(path) == STRSXP) {
            std::string path_str = get_string(path);
            if (!path_str.empty()) {
                std::string lib_name = infer_lib_location(path_str, fn_name);
                // printf("%s\n", lib_name.c_str());
                void * hnd = dlopen(lib_name.c_str(), RTLD_LAZY);
                if (hnd) {
                    fn_ptr = dlsym(hnd, fn_name);
                }
            }
        }
    }

    free(fn_name);

    return fn_ptr;
}
