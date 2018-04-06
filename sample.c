#include "object_mover.h"

int main() {

  int ret;
  
  /* initialize a Object Mover */
  sirius_ceph_initialize();

  const char *oid = "foo";
  const char *val = "bar";

  /* create an object in Fast Tier (SSD) */
  ret = 1;
  sirius_ceph_create_async(SIRIUS_CEPH_TIER_SSD, oid, val, strlen(val), &ret);
  while (ret == 1);
  assert(ret == 0);

  /* move the object to Slow Tier (HDD) */
  ret = 1;
  sirius_ceph_move_async(SIRIUS_CEPH_TIER_HDD, oid, &ret);
  while (ret == 1);
  assert(ret == 0);

  /* read the object */
  ret = 0;
  char buf[64];
  sirius_ceph_read(oid, buf, 64, &ret);
  assert(ret == strlen(val));

  /* delete the object */
  ret = 1;
  sirius_ceph_delete_async(oid, &ret);
  while (ret == 1);
  assert(ret == 0);

  return 0;
}
