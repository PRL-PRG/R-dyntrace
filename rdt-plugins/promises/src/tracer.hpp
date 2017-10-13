#ifndef __TRACER_HPP__
#define __TRACER_HPP__

#include "SqlSerializer.hpp"

SqlSerializer &tracer_serializer();
void set_tracer_serializer(const std::string filename);

#endif /* __TRACER_HPP__ */
