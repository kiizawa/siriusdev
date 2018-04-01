#include <assert.h>

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <list>

//#define DEBUG

unsigned long min_timestamp = -1UL;
unsigned long max_timestamp = 0;
unsigned long total = 0;
int count = 0;

std::map<std::string, std::list<unsigned long> > latencies;

#ifdef DEBUG
std::map<std::string, std::pair<unsigned long, unsigned long> > series;
std::multimap<unsigned long, std::string> log;
#endif /* DEBUG */

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
#ifdef DEBUG
      log.insert(std::make_pair(latency, oid));
      series[oid] = std::make_pair(values.front(), timestamp);
#endif /* DEBUG */
      count++;
      values.pop_front();
      if (values.empty()) {
	latencies.erase(oid);
      }
    }
  }

  std::cout << "average " << total/count << " [ms]" << std::endl;
  std::cout << "elapsed time " << (max_timestamp - min_timestamp) <<" [ms]" << std::endl;

#ifdef DEBUG
  std::multimap<unsigned long, std::string>::reverse_iterator rit = log.rbegin();
  std::cout << "max " << rit->first << "[ms] oid=" << rit->second << std::endl;
  const long s = series[rit->second].first/1000;
  struct tm* time_info_s = ::localtime(&s);
  std::cout << "start  " << asctime(time_info_s);
  const long e = series[rit->second].second/1000;
  struct tm* time_info_e = ::localtime(&e);
  std::cout << "finish " << asctime(time_info_e);
#endif /* DEBUG */

  return 0;
}
