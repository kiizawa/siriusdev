#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>

#include <rados/librados.hpp>

#include "object_mover.hpp"

#define OBJECT_NUM 10000
#define THREAD_NUM 128
#define WAIT_MSEC 10

int main(int argc, char *argv[]) {

  int object_num = OBJECT_NUM;
  int thread_num = THREAD_NUM;

  int opt;

  while ((opt = ::getopt(argc, argv, "n:t:h")) != -1) {
    switch (opt) {
    case 'n':
      object_num = ::atoi(optarg);
      break;

    case 't':
      thread_num = ::atoi(optarg);
      break;

    case 'h':
    default:
      printf("Usage: %s [-n objects] [-t threads]\n", argv[0]);
      break;
    }
  }

  int ret = 0;

  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_num controls the parallelism.
   */
  ObjectMover om(thread_num);

  /* Create an object in Fast Tier (SSD) */
  librados::bufferlist bl;
  for (int i = 0; i < 1024*1024; i++) {
    bl.append("ceph1234");
  }

  std::vector<int> rets;
  for (int i = 0; i < thread_num; i++) {
    rets.push_back(0);
  }
  for (int i = 0; i < object_num; i++) {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
  retry:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	rets[j] = 1;
	om.CreateAsync(ObjectMover::SLOW, object, bl, &rets[j]);
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
      printf("all writes done!\n");
      break;
    }
  }

  return 0;
}
