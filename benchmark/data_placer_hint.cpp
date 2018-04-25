#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

int B_HDD;
int B_SSD;

std::vector<std::string> split(std::string input, char delimiter) {
  std::istringstream stream(input);
  std::string field;
  std::vector<std::string> result;
  while (getline(stream, field, delimiter)) {
    result.push_back(field);
  }
  return result;
}

bool is_in(const std::vector<std::string> &list, const std::string &s) {
  std::vector<std::string>::const_iterator it;
  for (it = list.begin(); it != list.end(); it++) {
    if (*it == s) {
      return true;
    }
  }
  return false;
}

void read(const std::string &policy, const std::string &file_list, const std::string &object_list, const std::string &working_set_list) {

  /* Load file list */

  std::ifstream ifs_file_list(file_list.c_str());
  if (ifs_file_list.fail()) {
    exit(0);
  }

  std::map<std::string, int> object_map;

  // count the number of times each object is going to be accessed

  std::string file_name;
  while(getline(ifs_file_list, file_name)) {
    std::ifstream ifs_object_list(file_name.c_str());
    std::string line;
    while(getline(ifs_object_list, line)) {
      std::map<std::string, int>::iterator it = object_map.find(line);
      if (it == object_map.end()) {
	object_map[line] = 1;
      } else {
	it->second = it->second + 1;
      }
    }
    //std::vector<std::string> fields = split(line, ',');
    //std::string oid = fields[1];
    //objects.push_back(line);
  }
  ifs_file_list.close();

  std::set<std::string> objects_in_ssd_set;
  {
    std::map<std::string, int>::iterator it;
    for (it = object_map.begin() ; it != object_map.end(); ++it) {
      if (it->second > 1) {
	objects_in_ssd_set.insert(it->first);
      }
    }
  }
  // std::cout << object_map.size() << std::endl;
  // std::cout << objects_in_ssd_set.size() << std::endl;

  assert(objects_in_ssd_set.size() == 0);

  // start object in ssd list for each pattern

  std::map<std::string, std::pair<std::vector<std::string>, std::vector<std::string> > >  both_list;
  ifs_file_list.open(file_list.c_str());
  if (ifs_file_list.fail()) {
    exit(0);
  }
  while(getline(ifs_file_list, file_name)) {
    std::vector<std::string> objects;

    std::string line;
    std::ifstream ifs_object_list(file_name.c_str());
    while(getline(ifs_object_list, line)) {
      objects.push_back(line);
    }
    std::vector<std::string> objects_in_ssd;
    std::vector<std::string> objects_in_hdd;
    int num_objects_in_SSD = objects.size() * B_SSD/(B_HDD + B_SSD);
    std::set<int> rands;
    while (rands.size() < num_objects_in_SSD) {
      int i = rand() % objects.size();
      rands.insert(i);
    }
    for (int i = 0; i < objects.size(); i++) {
      std::set<int>::iterator it = rands.find(i);
      if (it == rands.end()) {
	objects_in_hdd.push_back(objects[*it]);
      } else {
	objects_in_ssd.push_back(objects[*it]);
      }
    }
    // SSD list
    std::string output_file_name_ssd = file_name + "_ssd";
    std::ofstream ofs_ssd_object_list(output_file_name_ssd.c_str());
    if (ofs_ssd_object_list.fail()) {
      exit(0);
    }
    {
      std::vector<std::string>::const_iterator it;
      for (it = objects_in_ssd.begin(); it != objects_in_ssd.end(); it++) {
	ofs_ssd_object_list << *it << std::endl;
      }
    }
    ofs_ssd_object_list.close();
    // HDD list
    std::string output_file_name_hdd = file_name + "_hdd";
    std::ofstream ofs_hdd_object_list(output_file_name_hdd.c_str());
    if (ofs_hdd_object_list.fail()) {
      exit(0);
    }
    {
      std::vector<std::string>::const_iterator it;
      for (it = objects_in_hdd.begin(); it != objects_in_hdd.end(); it++) {
	ofs_hdd_object_list << *it << std::endl;
      }
    }
    ofs_hdd_object_list.close();
    // printf("%d %d\n", objects_in_ssd.size(), objects_in_hdd.size());
  }
  ifs_file_list.close();

#if 0
  std::map<int, int> counts;

  std::vector<std::string> objects_in_ssd;
  std::vector<std::string> objects_in_hdd_tmp;

  std::map<std::string, int>::iterator it;
  for (it = object_map.begin(); it != object_map.end(); ++it) {
    if (it->second > 1) {
      objects_in_ssd.push_back(it->first);
    } else {
      objects_in_hdd_tmp.push_back(it->first);
    }
    int count = it->second;
    std::map<int, int>::iterator it2 = counts.find(count);
    if (it2 == counts.end()) {
      counts[count] = 1;
    } else {
      it2->second = it2->second + 1;
    }
  }

  std::map<int, int>::iterator it2;
  for (it2 = counts.begin(); it2 != counts.end(); ++it2) {
    printf("count=%d,%6d [objs]\n", it2->first, it2->second);
  }
#endif

#if 0
  printf("working set size=%lu [objs] %lu [GiB]\n", object_map.size(), object_map.size() * 8 / 1024);
  int C_ssd = object_map.size() * 8 * B_SSD / (B_HDD + B_SSD);
  printf("C_ssd=%d[MB] %d[objs]\n", C_ssd, C_ssd/8);

  int diff = C_ssd/8 - objects_in_ssd.size();
  int total = objects_in_hdd_tmp.size();

  std::set<int> rands;
  while (rands.size() < diff) {
    int i = rand() % total;
    rands.insert(i);
  }

  std::set<int>::iterator it3;
  for (it3 = rands.begin(); it3 != rands.end(); it3++) {
    objects_in_ssd.push_back(objects_in_hdd_tmp[*it3]);
  }

  double ratio = (double)B_SSD/B_HDD;
  printf("best ratio=%f\n", ratio);

  std::ifstream ifs_file_list2(file_list.c_str());
  if (ifs_file_list2.fail()) {
    exit(0);
  }

#endif

#if 0
  while(getline(ifs_file_list2, file_name)) {
    std::ifstream ifs_object_list(file_name.c_str());
    std::string line;
    int count_ssd = 0;
    int count_hdd = 0;
    while(getline(ifs_object_list, line)) {
      if (is_in(objects_in_ssd, line)) {
	count_ssd++;
      } else {
	count_hdd++;
      }
    }
    printf("filename=%s ratio=%f\n", file_name.c_str(), (double)count_ssd/count_hdd);
  }
  ifs_file_list2.close();
#endif

#if 0
  while (objects_in_ssd.size() < C_ssd/8) {
    objects_in_ssd.push_back();
  }
#endif

#if 0
  std::vector<std::string> objects;
  std::string line;
  while(getline(ifs, line)) {
    std::vector<std::string> fields = split(line, ',');
    std::string oid = fields[1];
    objects.push_back(line);
  }
#endif

#if 0
  std::ofstream ofs_ssd_object_list(object_list.c_str());
  if (ofs_ssd_object_list.fail()) {
    exit(0);
  }
  {
    std::vector<std::string>::const_iterator it;
    for (it = objects_in_ssd.begin(); it != objects_in_ssd.end(); it++) {
      ofs_ssd_object_list << *it << std::endl;
    }
    ofs_ssd_object_list.close();
  }
  std::ofstream ofs_working_set_list(working_set_list.c_str());
  if (ofs_working_set_list.fail()) {
    exit(0);
  }
  {
    std::map<std::string, int>::iterator it2;
    for (it2 = object_map.begin(); it2 != object_map.end(); it2++) {
      ofs_working_set_list << it2->first << ",8388608" << std::endl;
    }
    ofs_working_set_list.close();
  }

  // output

  std::vector<std::string> objects_in_ssd;
  std::vector<std::string> objects_in_hdd_tmp;

  {
    std::ifstream ifs_file_list(file_list.c_str());
    if (ifs_file_list.fail()) {
      exit(0);
    }
    std::map<std::string, int> object_map;

    std::string file_name;
    while(getline(ifs_file_list, file_name)) {
      std::ifstream ifs_object_list(file_name.c_str());
      std::string line;
      while(getline(ifs_object_list, line)) {
	std::map<std::string, int>::iterator it = object_map.find(line);
	if (it == object_map.end()) {
	  object_map[line] = 1;
	} else {
	  it->second = it->second + 1;
	}
      }
      //std::vector<std::string> fields = split(line, ',');
      //std::string oid = fields[1];
      //objects.push_back(line);
    }
    ifs_file_list.close();
  }

#endif

}

int main(int argc, char *argv[]) {

  std::string file_list;
  std::string object_list;
  std::string working_set_list;
  std::string policy;
  
  int opt;
  while ((opt = ::getopt(argc, argv, "p:w:d:s:i:o:h")) != -1) {
    switch (opt) {
    case 'p':
      policy = optarg;
      break;
    case 'i':
      /* input file list */
      file_list = optarg;
      break;
    case 'w':
      /* output object list(working set) */
      working_set_list = optarg;
      break;
    case 'o':
      /* output object list(ssd) */
      object_list = optarg;
      break;
    case 's':
      /* SSD bandwidth */
      B_SSD = atoi(optarg);
      break;
    case 'd':
      /* HDD bandwidth */
      B_HDD = atoi(optarg);
      break;
    case 'h':
    default:
      printf("Usage: %s\n", argv[0]);
      exit(0);
      break;
    }
  }

  read(policy, file_list, object_list, working_set_list);

  return 0;
}
