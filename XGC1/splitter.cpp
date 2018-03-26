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

  int process_num = atoi(argv[2]);
  
  std::string filename(argv[1]);

  std::ifstream ifs(filename);
  if (ifs.fail()) {
    exit(0);
  }

  std::string line;

  int count = 0;

  int total_rank = 10;
  int total_process = 5;
  int ranks_per_process = total_rank/total_process;
  
  while(getline(ifs, line)) {
    count++;
    std::vector<std::string> fields = split(line, ',');
    int rank = atoi(fields[0].c_str());
    std::string oid = fields[1];
    if (rank >= process_num * ranks_per_process &&  rank < (process_num + 1) * ranks_per_process) {
#if 1
      /* reader */
      std::cout << oid << std::endl;
#else
      /* writer */
      if (count % 40 == 0) {
	std::cout << oid << ",20480000" << std::endl;
      } else {
	std::cout << oid << ",7680000" << std::endl;
      }
#endif
    }
  }
  ifs.close();

  return 0;
}
