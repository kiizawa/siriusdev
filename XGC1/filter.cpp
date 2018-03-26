#include <stdlib.h>

#include <assert.h>

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

unsigned long min_timestamp = -1UL;
unsigned long max_timestamp = 0;
unsigned long total = 0;

std::map<std::string, unsigned long> latencies;

std::vector<std::string> split(std::string input, char delimiter) {
  std::istringstream stream(input);
  std::string field;
  std::vector<std::string> result;
  while (getline(stream, field, delimiter)) {
    result.push_back(field);
  }
  return result;
}

int main(int argc, char *argv[]) {
  
  if (argc == 1) {
    std::cout << "Usage: " << argv[0] << " filename rank" << std::endl;
    exit(0);
  }

  std::string filename(argv[1]);

  std::ifstream ifs(filename);
  if (ifs.fail()) {
    exit(0);
  }

  std::string line;
  
  while(getline(ifs, line)) {
    std::vector<std::string> fields = split(line, '/');
    std::string ts = fields[2];
    std::string var = fields[3];
    std::string version = fields[4];
    /* reader */
    if (ts == "0" && var == "temperat" && version == "1") {
      std::cout << line << std::endl;
    }
  }
  ifs.close();

  return 0;
}
