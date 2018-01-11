#ifndef __TIERING_MANAGER_HPP
#define __TIERING_MANAGER_HPP

/**
 * Wrapper functions for moving Ceph objects across tiers
 */
class TieringManager {
public:
  enum Tier {
    FAST = 0,
    SLOW,
    ARCHIVE,
  };
  /**
   * Constructor
   *
   * @param cluster Ceph cluster 
   * @param io_ctx_storage IoCtx associated with Storage Pool
   * @param io_ctx_archive IoCtx associated with Archive Pool
   */
  TieringManager(librados::Rados *cluster, librados::IoCtx *io_ctx_storage, librados::IoCtx *io_ctx_archive) :
    cluster_(cluster), io_ctx_storage_(io_ctx_storage), io_ctx_archive_(io_ctx_archive) {
  }
  ~TieringManager() {}
  /**
   * Create an object in the specified tier
   *
   * @param tier the tier in which to create the object
   * @param object_name the name of the object
   * @param value the data of the object
   */
  int Create(Tier tier, const std::string &object_name, const std::string &value);
  /**
   * Move an object to the specified tier
   *
   * @param tier the tier to which to move the object
   * @param object_name the name of the object
   */
  int Move(Tier tier, const std::string &object_name);
  /**
   * Delete an object
   *
   * @param object_name the name of the object
   */
  int Delete(const std::string &object_name);
private:
  librados::Rados *cluster_;
  librados::IoCtx *io_ctx_storage_;
  librados::IoCtx *io_ctx_archive_;
};

#endif
