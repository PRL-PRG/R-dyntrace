#include "globals.hpp"

SqlSerializer * serializer = nullptr;
tracer_state_t * state = nullptr;

tracer_state_t &tracer_state() { return *state; }

SqlSerializer &tracer_serializer() { return *serializer; }

tracer_state_t *set_tracer_state(tracer_state_t *new_state) {
  tracer_state_t *old_state = state;
  state = new_state;
  return old_state;
}

SqlSerializer *set_tracer_serializer(SqlSerializer *new_serializer) {
  SqlSerializer *old_serializer = serializer;
  serializer = new_serializer;
  return old_serializer;
}
