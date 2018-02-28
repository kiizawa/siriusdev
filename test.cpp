#include <stdio.h>

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <rados/librados.hpp>

#include "object_mover.hpp"
//#include "test_library.hpp"

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
  
  // test

  ObjectMover om(&cluster, &io_ctx_storage, &io_ctx_archive);

  std::string object_ff("FF");
  std::string object_fs("FS");
  std::string object_fa("FA");

  std::string object_sf("SF");
  std::string object_ss("SS");
  std::string object_sa("SA");

  std::string object_af("AF");
  std::string object_as("AS");
  std::string object_aa("AA");

  librados::bufferlist bl;
  bl.append("value");

  std::string result;

  // 1. FAST -> FAST

  printf("test: FAST    -> FAST   \n");

  om.Create(ObjectMover::FAST, object_ff, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_ff, object_ff);

  om.Move(ObjectMover::FAST, object_ff, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_ff, object_ff);

  // 2. FAST -> SLOW

  printf("test: FAST    -> SLOW   \n");

  om.Create(ObjectMover::FAST, object_fs, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_fs, object_fs);

  om.Move(ObjectMover::SLOW, object_fs, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_fs, object_fs);

  // 3. FAST -> ARCHIVE

  printf("test: FAST    -> ARCHIVE\n");

  om.Create(ObjectMover::FAST, object_fa, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_fa, object_fa);

  om.Move(ObjectMover::ARCHIVE, object_fa, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_fa, object_fa);
  
  // 4. SLOW -> FAST

  printf("test: SLOW    -> FAST   \n");

  om.Create(ObjectMover::SLOW, object_sf, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_sf, object_sf);

  om.Move(ObjectMover::FAST, object_sf, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_sf, object_sf);

  // 5. SLOW -> SLOW

  printf("test: SLOW    -> SLOW   \n");

  om.Create(ObjectMover::SLOW, object_ss, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_ss, object_ss);

  om.Move(ObjectMover::SLOW, object_ss, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_ss, object_ss);

  // 6. SLOW -> ARCHIVE

  printf("test: SLOW    -> ARCHIVE\n");
  
  om.Create(ObjectMover::SLOW, object_sa, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_sa, object_sa);

  om.Move(ObjectMover::ARCHIVE, object_sa, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_sa, object_sa);
  
  // 7. ARCHIVE -> FAST

  printf("test: ARCHIVE -> FAST   \n");

  om.Create(ObjectMover::ARCHIVE, object_af, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_af, object_af);

  om.Move(ObjectMover::FAST, object_af, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_af, object_af);

  // 8. ARCHIVE -> SLOW

  printf("test: ARCHIVE -> SLOW   \n");

  om.Create(ObjectMover::ARCHIVE, object_as, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_as, object_as);

  om.Move(ObjectMover::SLOW, object_as, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_as, object_as);

  // 9. ARCHIVE -> ARCHIVE

  printf("test: ARCHIVE -> ARCHIVE\n");
  
  om.Create(ObjectMover::ARCHIVE, object_aa, bl, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_aa, object_aa);

  om.Move(ObjectMover::ARCHIVE, object_aa, &ret);
  assert(ret == 0);
  //read(io_ctx_storage, object_aa, object_aa);

  // delete all the objects

  om.Delete(object_ff, &ret);
  assert(ret == 0);

  om.Delete(object_fs, &ret);
  assert(ret == 0);

  om.Delete(object_fa, &ret);
  assert(ret == 0);

  om.Delete(object_sf, &ret);
  assert(ret == 0);

  om.Delete(object_ss, &ret);
  assert(ret == 0);

  om.Delete(object_sa, &ret);
  assert(ret == 0);

  om.Delete(object_af, &ret);
  assert(ret == 0);

  om.Delete(object_as, &ret);
  assert(ret == 0);

  om.Delete(object_aa, &ret);
  assert(ret == 0);

  return 0;
}
