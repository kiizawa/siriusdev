#include <iostream>
#include <sstream>
#include <string>
#include <rados/librados.hpp>
#include "object_mover.hpp"

#define PARALLELISM 128
#define OBJECT_NUM 10000
#define WAIT_MSEC 10

int main() {

  int ret = 0;

  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_pool_size controls the parallelism.
   */
  int thread_pool_size = PARALLELISM;
  ObjectMover om(thread_pool_size);

  /* Create an object in Fast Tier (SSD) */
  librados::bufferlist bl;
  for (int i = 0; i < 1024*1024; i++) {
    bl.append("ceph1234");
  }

  int rets[PARALLELISM];
  for (int i = 0; i < PARALLELISM; i++) {
    rets[i] = 0;
  }
  for (int i = 0; i < OBJECT_NUM; i++) {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
  retry:
    int used = 0;
    for (int j = 0; j < PARALLELISM; j++) {
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
    if (used == PARALLELISM) {
      usleep(WAIT_MSEC*1000);
      goto retry;
    }
  }
  while (true) {
    int done = 0;
    for (int j = 0; j < PARALLELISM; j++) {
      int ret = rets[j];
      if (ret == 0) {
	done++;
      }
    }
    if (done == PARALLELISM) {
      printf("all writes done!\n");
      break;
    }
  }

  return 0;
}
