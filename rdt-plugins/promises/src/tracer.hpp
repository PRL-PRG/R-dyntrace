#ifndef __TRACER_HPP__
#define __TRACER_HPP__

#include "SqlSerializer.hpp"
#include "globals.hpp"
#include "hooks.hpp"
#include "recorder.hpp"
#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

rdt_handler *setup_tracing(SEXP options);

void cleanup_tracing(SEXP options);

#ifdef __cplusplus
}
#endif

#endif /* __TRACER_HPP__ */
