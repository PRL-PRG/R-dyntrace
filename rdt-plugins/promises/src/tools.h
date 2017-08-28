#ifndef R_3_3_1_TOOLS_H
#define R_3_3_1_TOOLS_H

#include <string>
#include <vector>

namespace tools {

    template<typename T>
    std::underlying_type_t<T> enum_cast(const T &enum_val) {
        return static_cast<std::underlying_type_t<T>>(enum_val);
    }

    bool file_exists(const std::string &fname);

    template<typename T>
    std::vector<std::vector<T>> split_vector(const std::vector<T> &values, size_t block_size) {
        std::vector<std::vector<T>> result;
        typename std::vector<T>::const_iterator iterator = values.begin();

        size_t remaining_elements = values.size();
        do {
            std::vector<T> sub_vector;
            if(values.end() - iterator > block_size) {
                sub_vector.assign(iterator, iterator + block_size);
                iterator += block_size;
                remaining_elements -= block_size;

                result.push_back(sub_vector);
            } else {
                sub_vector.assign(iterator, values.end());
                //sub_vector.resize(block_size);
                remaining_elements = 0;

                result.push_back(sub_vector);
            }
        } while (remaining_elements > 0);

        return result;
    }
}

#endif //R_3_3_1_TOOLS_H_H
