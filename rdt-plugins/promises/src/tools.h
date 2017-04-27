#ifndef R_3_3_1_TOOLS_H
#define R_3_3_1_TOOLS_H

#include <string>

namespace tools {

    template<typename T>
    std::underlying_type_t<T> enum_cast(const T &enum_val) {
        return static_cast<std::underlying_type_t<T>>(enum_val);
    }

    bool file_exists(const std::string &fname);

}

#endif //R_3_3_1_TOOLS_H_H
