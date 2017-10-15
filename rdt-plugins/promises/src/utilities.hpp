#ifndef __UTILITIES_HPP__
#define __UTILITIES_HPP__

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <rdt.h>
#include <string>
#include <vector>

int get_file_size(std::ifstream &file);

const char *get_file_contents(const char *filepath);

bool file_exists(const std::string &filepath);

bool sexp_to_bool(SEXP value, bool default_value = false);

int sexp_to_int(SEXP value, int default_value = 0);

std::string sexp_to_string(SEXP value,
                           std::string default_value = std::string(""));

template <typename T>
std::underlying_type_t<T> to_underlying_type(const T &enum_val);

#endif /* __UTILITIES_HPP__ */
