#include <iostream>
#include <string>
#include <rados/librados.hpp>
#include "tiering_manager.hpp"

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
  
  /* Initialize a Tiering Manager */
  TieringManager tm(&cluster, &io_ctx_storage, &io_ctx_archive);

  /* Create an object in Fast Tier (SSD) */
  std::string object("foo");
  ret = tm.Create(TieringManager::FAST, object, "bar");
  assert(ret == 0);

  /* Move the object to Archive Tier (Tape Drive) */
  ret = tm.Move(TieringManager::ARCHIVE, object);
  assert(ret == 0);



  return 0;
}
