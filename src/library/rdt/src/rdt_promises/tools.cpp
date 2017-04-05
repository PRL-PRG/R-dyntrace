#include "tools.h"

#include <string>
#include <fstream>

namespace tools {

    bool file_exists(const std::string &fname) {
        std::ifstream f(fname);
        return f.good();
    }

}