#ifndef __OBJECT_MOVER_HPP
#define __OBJECT_MOVER_HPP

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>

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
   * @param thread_pool_size size of thread pool that do asynchronous I/Os
   */
  ObjectMover(librados::Rados *cluster,
	      librados::IoCtx *io_ctx_storage,
	      librados::IoCtx *io_ctx_archive,
	      int thread_pool_size = 32);
  ~ObjectMover();
  /**
   * Create an object in the specified tier
   *
   * @param tier the tier in which to create the object
   * @param object_name the name of the object
   * @param value the data of the object
   */
  void Create(Tier tier, const std::string &object_name, const librados::bufferlist &bl, int *err);
  void CreateAsync(Tier tier, const std::string &object_name, const librados::bufferlist &bl, int *err) {
    auto f = std::bind(&ObjectMover::Create, this, tier, object_name, bl, err);
    ios_.post(f);
  }
  /**
   * Move an object to the specified tier
   *
   * @param tier the tier to which to move the object
   * @param object_name the name of the object
   */
  void Move(Tier tier, const std::string &object_name, int *err);
  void MoveAsync(Tier tier, const std::string &object_name, int *err) {
    auto f = std::bind(&ObjectMover::Move, this, tier, object_name, err);
    ios_.post(f);
  }
  /**
   * Delete an object
   *
   * @param object_name the name of the object
   */
  void Delete(const std::string &object_name, int *err);
  void DeleteAsync(const std::string &object_name, int *err) {
    auto f = std::bind(&ObjectMover::Delete, this, object_name, err);
    ios_.post(f);
  }

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
  /**/
  boost::asio::io_service ios_;
  boost::asio::io_service::work *w_;
  boost::thread_group thr_grp_;
};

#endif
