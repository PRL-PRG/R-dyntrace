#include "utilities.hpp"

int get_file_size(std::ifstream &file) {
    int position = file.tellg();
    file.seekg(0, std::ios_base::end);
    int length = file.tellg();
    file.seekg(position, std::ios_base::beg);
    return length;
}

const char *get_file_contents(const char *filepath) {
    std::ifstream file(filepath);
    if (!file.good()) {
        std::cerr << "ERROR - Unable to open file " << filepath;
        exit(1);
    }
    int length = get_file_size(file);
    char *buffer = (char *)malloc(length * sizeof(char));
    file.read(buffer, length);
    file.close();
    return buffer;
}

bool file_exists(const std::string &filepath) {
    return std::ifstream(filepath).good();
}

bool sexp_to_bool(SEXP value, bool default_value) {
    if (value != NULL && value != R_NilValue)
        return LOGICAL(value)[0] == TRUE;
    else
        return default_value;
}

int sexp_to_int(SEXP value, int default_value) {
    if (value != NULL && value != R_NilValue && TYPEOF(value) == REALSXP)
        return (int)*REAL(value);
    else
        return default_value;
}

std::string sexp_to_string(SEXP value, std::string default_value) {

    if (value != R_NilValue && TYPEOF(value) == STRSXP)
        return std::string(CHAR(STRING_ELT(value, 0)));
    else
        return default_value;
}
