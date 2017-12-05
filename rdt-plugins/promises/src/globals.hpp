#ifndef __GLOBALS_HPP__
#define __GLOBALS_HPP__

#include "State.hpp"
#include "SqlSerializer.hpp"

tracer_state_t &tracer_state();

SqlSerializer &tracer_serializer();

tracer_state_t *set_tracer_state(tracer_state_t *new_state);

SqlSerializer *set_tracer_serializer(SqlSerializer *new_serializer);

#endif /* __GLOBALS_HPP__ */
