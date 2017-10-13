#include "tracer.hpp"

SqlSerializer *SQL_SERIALIZER = nullptr;

void set_tracer_serializer(const std::string &database_path,
                           const std::string &schema_path) {
  SQL_SERIALIZER = new SqlSerializer(database_path, schema_path);
}

SqlSerializer &tracer_serializer() { return *SQL_SERIALIZER; }
