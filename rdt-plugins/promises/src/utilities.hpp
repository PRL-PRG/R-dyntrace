#ifndef __UTILITIES_HPP__
#define __UTILITIES_HPP__

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>

int get_file_size(std::ifstream &file);
const char *get_file_contents(const char *filepath);
bool file_exists(const std::string &filepath);

#endif /* __UTILITIES_HPP__ */
