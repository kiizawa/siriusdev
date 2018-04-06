#ifndef __OBJECT_MOVER_H
#define __OBJECT_MOVER_H

#include "object_mover.hpp"

enum {
  SIRIUS_CEPH_TIER_SSD,
  SIRIUS_CEPH_TIER_HDD,
  SIRIUS_CEPH_TIER_TD,
};

ObjectMover *om;

/**
 * Initialize
 */
extern "C" void sirius_ceph_initialize() {
  om = new ObjectMover();
}

/**
 * Create an object in the specified tier asynchronously
 *
 * @param[in] tier the tier in which to create the object
 * @param[in] oid the name of the object
 * @param[in] buf pointer the buffer
 * @param[in] len length of the buffer
 * @param[out] err  0 success
 * @param[out] err <0 failure
 */
extern "C" void sirius_ceph_create_async(int tier, const char *oid, const char *buf, size_t len, int *err) {
  std::string object_name(oid);
  librados::bufferlist bl;
  bl.append(buf, len);
  om->CreateAsync(static_cast<ObjectMover::Tier>(tier), object_name, bl, err);
}

/**
 * Move an object to the specified tier asynchronously
 *
 * @param[in] tier the tier to which to move the object
 * @param[in] oid the name of the object
 * @param[out] err  0 success
 * @param[out] err <0 failure
 */
extern "C" void sirius_ceph_move_async(int tier, const char *oid, int *err) {
  std::string object_name(oid);
  om->MoveAsync(static_cast<ObjectMover::Tier>(tier), object_name, err);
}

/**
 * Read an object asynchronously
 *
 * @param[in] oid the name of the object
 * @param[in] buf where to store the results 
 * @param[in] len the number of bytes to read
 * @param[out] ret number of bytes read on success, negative error code on failure
 */
extern "C" void sirius_ceph_read_async(const char *oid, char* buf, size_t len, int *ret) {
  std::string object_name(oid);
  om->CReadAsync(object_name, buf, len, ret);
}

/**
 * Delete an object asynchronously
 *
 * @param[in] oid the name of the object
 * @param[out] err  0 success
 * @param[out] err <0 failure
 */
extern "C" void sirius_ceph_delete_async(const char *oid, int *err) {
  std::string object_name(oid);
  om->DeleteAsync(object_name, err);
}

/**
 * Finalize
 */
extern "C" void sirius_ceph_finalize() {
  delete om;
}

#endif
