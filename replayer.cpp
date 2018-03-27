#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>

#include <map>

#include <rados/librados.hpp>

#include "object_mover.hpp"

#define OBJECT_NUM 10000
#define THREAD_NUM 128
#define WAIT_MSEC 10

enum Mode {
  READ,
  WRITE,
  MOVE
};

std::vector<std::string> split(std::string input, char delimiter) {
  std::istringstream stream(input);
  std::string field;
  std::vector<std::string> result;
  while (getline(stream, field, delimiter)) {
    result.push_back(field);
  }
  return result;
}

void read(const std::string trace_filename, int thread_num, const std::string &object_list) {

  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_num controls the parallelism.
   */
  ObjectMover om(thread_num, trace_filename);
  
  /* Load object list */

  std::ifstream ifs(object_list);
  if (ifs.fail()) {
    exit(0);
  }

  std::vector<std::string> objects;
  
  std::string line;
  while(getline(ifs, line)) {
    objects.push_back(line);
  }
  
  /* Read objects from Slow Tier (HDD) */

  std::vector<int> rets;
  for (int i = 0; i < thread_num; i++) {
    rets.push_back(1);
  }

  /* read */

  std::vector<librados::bufferlist*> bls;
  for (int i = 0; i < thread_num; i++) {
    bls.push_back(new librados::bufferlist);
  }

  for (std::vector<std::string>::iterator it = objects.begin(); it != objects.end(); it++) {
    std::string object = *it;
  retry:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret > 0) {
	rets[j] = 0;
	bls[j]->clear();
	om.ReadAsync(object, bls[j], &rets[j]);
	// while (rets[j] == 1);
	// assert(rets[j] == 0);
	break;
      } else {
	used++;
	if (ret != 0) {
	  printf("read failed! error=%d\n", ret);
	  abort();
	}
      }
    }
    if (used == thread_num) {
      usleep(WAIT_MSEC*1000);
      goto retry;
    }
  }

  /* wait */

  while (true) {
    int done = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret > 0) {
	done++;
      } else {
	if (ret != 0) {
	  printf("read failed! error=%d\n", ret);
	  abort();
	}
      }
    }
    if (done == thread_num) {
      printf("all reads (slow) done!\n");
      break;
    }
    usleep(WAIT_MSEC*1000);
  }

  std::vector<librados::bufferlist*>::iterator it;
  for (it = bls.begin(); it != bls.end(); it++) {
    delete *it;
  }

}

void write(const std::string &trace_filename, ObjectMover::Tier tier, int thread_num, const std::string &object_list) {

  std::map<int, std::string> waiting;

  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_num controls the parallelism.
   */
  ObjectMover om(thread_num, trace_filename);

  /* Load object list */

  std::ifstream ifs(object_list);
  if (ifs.fail()) {
    exit(0);
  }

  std::set<int> object_size_set;
  std::map<std::string, int> object_map;
  
  std::string line;
  while(getline(ifs, line)) {
    std::vector<std::string> fields = split(line, ',');
    std::string oid = fields[0];
    int size = atoi(fields[1].c_str());
    object_map[oid] = size;
    object_size_set.insert(size);
  }

  std::map<int, librados::bufferlist*> bl_map;
  for (std::set<int>::iterator it = object_size_set.begin(); it != object_size_set.end(); it++) {
    int size = *it;
    librados::bufferlist *bl = new librados::bufferlist;
    for (int i = 0; i < size; i++) {
      bl->append("c");
    }
    bl_map[size] = bl; 
  }
  
  /* Create objects in Slow Tier (HDD) */

  std::vector<int> rets;
  for (int i = 0; i < thread_num; i++) {
    rets.push_back(0);
  }

  /* write */

  for (std::map<std::string, int>::iterator it = object_map.begin(); it != object_map.end(); it++) {
    std::string object = it->first;
  retry:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	rets[j] = 1;
	waiting[j] = object;
	om.CreateAsync(tier, object, *bl_map[it->second], &rets[j]);
	// while (rets[j] == 1);
	// assert(rets[j] == 0);
	break;
      } else {
	used++;
	if (ret != 1) {
	  printf("write failed! error=%d\n", ret);
	  abort();
	}
      }
    }
    if (used == thread_num) {
      usleep(WAIT_MSEC*1000);
      goto retry;
    }
  }

  /* wait */

  while (true) {
    int done = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	done++;
      }	else {
	if (ret != 1) {
	  printf("write failed! error=%d\n", ret);
	  abort();
	}
	printf("waiting for object=%s\n", waiting[j].c_str());
      }
    }
    if (done == thread_num) {
      printf("all creates done!\n");
      break;
    }
    printf("done=%d\n", done);
    usleep(3000*1000);
  }

}

void move(const std::string &trace_filename, ObjectMover::Tier tier, int thread_num, const std::string &object_list) {

  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_num controls the parallelism.
   */
  ObjectMover om(thread_num, trace_filename);

  /* Load object list */

  std::ifstream ifs(object_list);
  if (ifs.fail()) {
    exit(0);
  }

  std::vector<std::string> objects;
  
  std::string line;
  while(getline(ifs, line)) {
    objects.push_back(line);
  }

  /* Move objects into Fast Tier (SSD) */

  std::vector<int> rets;
  for (int i = 0; i < thread_num; i++) {
    rets.push_back(0);
  }

  for (std::vector<std::string>::iterator it = objects.begin(); it != objects.end(); it++) {
    std::string object = *it;
  retry:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	rets[j] = 1;
	om.MoveAsync(tier, object, &rets[j]);
	// while (rets[j] == 1);
	// assert(rets[j] == 0);
	break;
      } else {
	used++;
	if (ret != 1) {
	  printf("move failed! error=%d\n", ret);
	  abort();
	}
      }
    }
    if (used == thread_num) {
      usleep(WAIT_MSEC*1000);
      goto retry;
    }
  }

  /* wait */

  while (true) {
    int done = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	done++;
      } else {
	if (ret != 1) {
	  printf("move failed! error=%d\n", ret);
	  abort();
	}
      }
    }
    if (done == thread_num) {
      printf("all moves done!\n");
      break;
    }
    usleep(WAIT_MSEC*1000);
  }
}

int main(int argc, char *argv[]) {

  std::string object_list;
  std::string trace_filename;
  Mode mode;
  int object_num = OBJECT_NUM;
  int thread_num = THREAD_NUM;
  ObjectMover::Tier tier;

  int opt;
  while ((opt = ::getopt(argc, argv, "l:f:o:t:m:r:h")) != -1) {
    switch (opt) {

    case 'l':
      /* object list */
      object_list = optarg;
      break;

    case 'f':
      /* prefix */
      trace_filename = optarg;
      break;

    case 'm':
      /* access mode */
      if (*optarg == 'r') {
        mode = READ;
      } else if (*optarg == 'w') {
	mode = WRITE;
      } else if (*optarg == 'm') {
	mode = MOVE;
      } else {
	printf("Invalid access mode\n");
	exit(0);
      }
      break;

    case 'o':
      /* number of objects */
      object_num = ::atoi(optarg);
      break;

    case 't':
      /* number of threads */
      thread_num = ::atoi(optarg);
      break;

    case 'r':
      /* tier */
      if (*optarg == 'a') {
	tier = ObjectMover::ARCHIVE;
      } else if (*optarg == 's') {
	tier = ObjectMover::SLOW;
      } else if (*optarg == 'f') {
	tier = ObjectMover::FAST;
      } else {
	printf("Invalid tier\n");
	exit(0);
      }
      break;

    case 'h':
    default:
      printf("Usage: %s [-n objects] [-t threads]\n", argv[0]);
      exit(0);
      break;
    }
  }

  switch(mode) {

  case READ:
    read(trace_filename, thread_num, object_list);
    break;

  case WRITE:
    write(trace_filename, tier, thread_num, object_list);
    break;

  case MOVE:
    move(trace_filename, tier, thread_num, object_list);
    break;

  default:
    break;    
  }
  
  return 0;
}
