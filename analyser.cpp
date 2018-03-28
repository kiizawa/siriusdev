#include <assert.h>

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <list>

unsigned long min_timestamp = -1UL;
unsigned long max_timestamp = 0;
unsigned long total = 0;
int count = 0;

std::map<std::string, std::list<unsigned long> > latencies;

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
    std::cout << "Usage: " << argv[0] << " filename" << std::endl;
    exit(0);
  }
  
  std::string filename(argv[1]);
  
  std::ifstream ifs(filename);
  if (ifs.fail()) {
    exit(0);
  }

  std::string line;

  while(getline(ifs, line)) {
    std::vector<std::string> fields = split(line, ',');
    std::string oid = fields[0];
    std::string mode;
    std::string tier;
    std::string sf = fields[3];
    unsigned long timestamp = atol(fields[4].c_str());

    if (timestamp < min_timestamp) {
      min_timestamp = timestamp;
    }
    if (timestamp > max_timestamp) {
      max_timestamp = timestamp;
    }

    if (sf == "s") {
      std::map<std::string, std::list<unsigned long> >::iterator it = latencies.find(oid);
      if (it == latencies.end()) {
	std::list<unsigned long> values;
	values.push_back(timestamp);
	latencies[oid] = values;
      } else {
	std::list<unsigned long> &values = latencies[oid];
	values.push_back(timestamp);
      }
    } else {
      std::map<std::string, std::list<unsigned long> >::iterator it = latencies.find(oid);
      if (it == latencies.end()) {
	std::cout << "BUG! "<< line << std::endl;
	abort();
      }
      std::list<unsigned long> &values = it->second;
      unsigned long latency = timestamp - values.front();
      total += latency;
      count++;
      values.pop_front();
      if (values.empty()) {
	latencies.erase(oid);
      }
    }
  }

  std::cout << "average " << total/count << " [ms]" << std::endl;
  std::cout << "elapsed time " << (max_timestamp - min_timestamp) <<" [ms]" << std::endl;
  
  return 0;
}
