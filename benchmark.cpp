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
      exit(0);
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

  /* Create objects in Slow Tier (HDD) */
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
  retry1:
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
      goto retry1;
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
      printf("all creates done!\n");
      break;
    }
  }

  /* Read objects from Slow Tier (HDD) */

  std::vector<librados::bufferlist*> bls;
  for (int i = 0; i < thread_num; i++) {
    bls.push_back(new librados::bufferlist);
  }
  for (int i = 0; i < object_num; i++) {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
  retry2:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	rets[j] = 1;
	bls[j]->clear();
	om.ReadAsync(object, bls[j], &rets[j]);
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
      goto retry2;
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
      printf("all reads (slow) done!\n");
      break;
    }
  }

  /* Move objects into Fast Tier (SSD) */
  for (int i = 0; i < object_num; i++) {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
  retry3:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	rets[j] = 1;
	om.MoveAsync(ObjectMover::FAST, object, &rets[j]);
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
      goto retry3;
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
  }

  /* Read objects from Fast Tier (SSD) */
  for (int i = 0; i < object_num; i++) {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
  retry4:
    int used = 0;
    for (int j = 0; j < thread_num; j++) {
      int ret = rets[j];
      if (ret == 0) {
	rets[j] = 1;
	bls[j]->clear();
	om.ReadAsync(object, bls[j], &rets[j]);
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
      goto retry4;
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
      printf("all reads (fast) done!\n");
      break;
    }
  }

  return 0;
}
