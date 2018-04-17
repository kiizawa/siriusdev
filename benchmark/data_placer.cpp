#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> split(std::string input, char delimiter) {
  std::istringstream stream(input);
  std::string field;
  std::vector<std::string> result;
  while (getline(stream, field, delimiter)) {
    result.push_back(field);
  }
  return result;
}

void read(const std::string &object_list) {

  /* Load object list */

  std::ifstream ifs(object_list.c_str());
  if (ifs.fail()) {
    exit(0);
  }

  std::vector<std::string> objects;

  std::string line;
  while(getline(ifs, line)) {
    objects.push_back(line);
  }
}

int main(int argc, char *argv[]) {

  std::string object_list;

  int opt;
  while ((opt = ::getopt(argc, argv, "l:h")) != -1) {
    switch (opt) {
    case 'l':
      /* object list */
      object_list = optarg;
      break;
    case 'h':
    default:
      printf("Usage: %s\n", argv[0]);
      exit(0);
      break;
    }
  }

  read(object_list);

  return 0;
}
