#include "utilities.hpp"

int get_file_size(std::ifstream &file) {
  int position = file.tellg();
  file.seekg(0, std::ios_base::end);
  int length = file.tellg();
  file.seekg(position, std::ios_base::beg);
  return length;
}

const char *get_file_contents(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.good()) {
    std::cerr << "ERROR - Unable to open file " << filepath;
    exit(1);
  }
  int length = get_file_size(file);
  char *buffer = (char*) malloc(length * sizeof(char));
  file.read(buffer, length);
  file.close();
  return buffer;
}

bool file_exists(const std::string &filepath) {
  return std::ifstream(filepath).good();
}
