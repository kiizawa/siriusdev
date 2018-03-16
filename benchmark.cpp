#include <iostream>
#include <sstream>
#include <string>
#include <rados/librados.hpp>
#include "object_mover.hpp"

int main() {

  int ret = 0;

  librados::Rados cluster;
  librados::IoCtx io_ctx_storage, io_ctx_archive;

  /* Declare the cluster handle and required variables. */
  char user_name[] = "client.admin";
  uint64_t flags;

  /* Initialize the cluster handle with the "ceph" cluster name and "client.admin" user */
  {
    ret = cluster.init2(user_name, "ceph", flags);
    if (ret < 0) {
      std::cerr << "Couldn't initialize the cluster handle! error " << ret << std::endl;
      ret = EXIT_FAILURE;
      return 1;
    } else {
      std::cout << "Created a cluster handle." << std::endl;
    }
  }

  /* Read a Ceph configuration file to configure the cluster handle. */
  {
    ret = cluster.conf_read_file("/share/ceph.conf");
    if (ret < 0) {
      std::cerr << "Couldn't read the Ceph configuration file! error " << ret << std::endl;
      ret = EXIT_FAILURE;
      return 1;
    } else {
      std::cout << "Read the Ceph configuration file." << std::endl;
    }
  }

  /* Connect to the cluster */
  {
    ret = cluster.connect();
    if (ret < 0) {
      std::cerr << "Couldn't connect to cluster! error " << ret << std::endl;
      ret = EXIT_FAILURE;
      return 1;
    } else {
      std::cout << "Connected to the cluster." << std::endl;
    }
  }

  {
    ret = cluster.ioctx_create("storage_pool", io_ctx_storage);
    if (ret < 0) {
      std::cerr << "Couldn't set up ioctx! error " << ret << std::endl;
      exit(EXIT_FAILURE);
    } else {
      std::cout << "Created an ioctx for the pool." << std::endl;
    }
  }

  {
    ret = cluster.ioctx_create("archive_pool", io_ctx_archive);
    if (ret < 0) {
      std::cerr << "Couldn't set up ioctx! error " << ret << std::endl;
      exit(EXIT_FAILURE);
    } else {
      std::cout << "Created an ioctx for the pool." << std::endl;
    }
  }
  
  /* Initialize a Object Mover */

  /**
   * Operations on obejcts can be executed asynchronously by background threads.
   * thread_pool_size controls the parallelism.
   */
  int thread_pool_size = 128;
  ObjectMover om(&cluster, &io_ctx_storage, &io_ctx_archive, thread_pool_size);

  /* Create an object in Fast Tier (SSD) */
  librados::bufferlist bl;
  for (int i = 0; i < 1024*1024; i++) {
    bl.append("ceph");
  }
  
  int rets[128];
  for (int i = 0; i < 128; i++) {
    rets[i] = 1;
  }

  for (int i = 0; i < 128; i++) {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(10) << i;
    std::string object = os.str();
  }

#if 0
 
  ret = 1;
  om.CreateAsync(ObjectMover::SLOW, object, bl, &ret);
  while (ret == 1);
  assert(ret == 0);

  /* Move the object to Archive Tier (Tape Drive) */
  ret = 1;
  om.MoveAsync(ObjectMover::FAST, object, &ret);
  while (ret == 1);
  assert(ret == 0);
#endif
  
  return 0;
}
