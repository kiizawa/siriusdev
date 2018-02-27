#ifndef __OBJECT_MOVER_HPP
#define __OBJECT_MOVER_HPP

/**
 * Wrapper functions for moving Ceph objects across tiers
 */
class ObjectMover {
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
  ObjectMover(librados::Rados *cluster, librados::IoCtx *io_ctx_storage, librados::IoCtx *io_ctx_archive) :
    cluster_(cluster), io_ctx_storage_(io_ctx_storage), io_ctx_archive_(io_ctx_archive) {
  }
  ~ObjectMover() {}
  /**
   * Create an object in the specified tier
   *
   * @param tier the tier in which to create the object
   * @param object_name the name of the object
   * @param value the data of the object
   */
  int Create(Tier tier, const std::string &object_name, const std::string &value);
  int Create(Tier tier, const std::string &object_name, const librados::bufferlist &bl);
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
  /**
   * Get the Location of the object
   *
   * @param object_name the name of the object
   */
  int GetLocation(const std::string &object_name);
private:
  /**
   * Lock advisory lock
   *
   * @param object_name the name of the object
   */
  void Lock(const std::string &object_name);
  /**
   * Unlock advisory lock
   *
   * @param object_name the name of the object
   */
  void Unlock(const std::string &object_name);
  librados::Rados *cluster_;
  librados::IoCtx *io_ctx_storage_;
  librados::IoCtx *io_ctx_archive_;
};

#endif
