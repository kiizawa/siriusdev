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

void read(const std::string trace_filename, int thread_num, int object_num) {

  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_num controls the parallelism.
   */
  ObjectMover om("/tmp/share/ceph.conf", thread_num, trace_filename);
  
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

  for (int i = 0; i < object_num; i++) {
    std::ostringstream os;
    os << "oname" << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
  retry:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret > 0) {
	rets[j] = 0;
	bls[j]->clear();
	om.ReadAsync(object, bls[j], 0, &rets[j]);
	// while (rets[j] == 1);
	// assert(rets[j] == 0);
	break;
      } else {
	used++;
	assert(ret == 0);
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

void write(const std::string &trace_filename, ObjectMover::Tier tier, int thread_num, int object_num) {

  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_num controls the parallelism.
   */
  ObjectMover om("/tmp/share/ceph.conf", thread_num, trace_filename);

  /* Create objects in Slow Tier (HDD) */

  std::vector<int> rets;
  for (int i = 0; i < thread_num; i++) {
    rets.push_back(0);
  }

  std::map<int, std::string> waiting;

  /* write */

  librados::bufferlist bl;
  for (int i = 0; i < 1024*1024; i++) {
    bl.append("ceph1234");
  }
  
  for (int i = 0; i < object_num; i++) {
    std::ostringstream os;
    os << "oname" << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
  retry:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	rets[j] = 1;
	waiting[j] = object;
	om.CreateAsync(tier, object, bl, &rets[j]);
	// while (rets[j] == 1);
	// assert(rets[j] == 0);
	break;
      } else {
	used++;
	assert(ret == 1);
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
      }
    }
    if (done == thread_num) {
      printf("all creates done!\n");
      break;
    }
    std::map<int, std::string>::iterator it;
    for (it = waiting.begin(); it != waiting.end(); ++it) {
      printf("waiting %s\n", it->second.c_str());
    }
    usleep(3000*1000);
  }

}

void move(const std::string &trace_filename, ObjectMover::Tier tier, int thread_num, int object_num) {

  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_num controls the parallelism.
   */
  ObjectMover om("/tmp/share/ceph.conf", thread_num, trace_filename);

  /* Move objects into Fast Tier (SSD) */

  std::vector<int> rets;
  for (int i = 0; i < thread_num; i++) {
    rets.push_back(0);
  }

  for (int i = 0; i < object_num; i++) {
    std::ostringstream os;
    os << "oname" << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
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
	assert(ret == 1);
      }
    }
    if (used == thread_num) {
      usleep(WAIT_MSEC*1000);
      goto retry;
    }
  }
  while (true) {
    int done = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	done++;
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

  std::string trace_filename;
  Mode mode;
  int object_num = OBJECT_NUM;
  int thread_num = THREAD_NUM;
  ObjectMover::Tier tier;

  int opt;
  while ((opt = ::getopt(argc, argv, "f:o:t:m:r:h")) != -1) {
    switch (opt) {

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
    read(trace_filename, thread_num, object_num);
    break;

  case WRITE:
    write(trace_filename, tier, thread_num, object_num);
    break;

  case MOVE:
    move(trace_filename, tier, thread_num, object_num);
    break;

  default:
    break;    
  }
  
  return 0;
}
