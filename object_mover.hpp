#ifndef __OBJECT_MOVER_HPP
#define __OBJECT_MOVER_HPP

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <fstream>

//#define USE_SESSION_POOL

class Session {
public:
  Session();
  ~Session();
  void Connect();
  void Reconnect();
  librados::Rados cluster_;
  librados::IoCtx io_ctx_storage_;
  librados::IoCtx io_ctx_archive_;
};

class SessionPool {
public:
  SessionPool(int session_pool_size) {
    for (int i = 0; i < session_pool_size; i++) {
      pool_.insert(std::make_pair(new Session, true));
    }
  }
  ~SessionPool() {
    for (std::map<Session*, bool>::iterator it = pool_.begin(); it != pool_.end(); it++) {
      delete it->first;
    }
  }
  Session* GetSession() {
  retry:
    boost::mutex::scoped_lock l(lock_);
    for (std::map<Session*, bool>::iterator it = pool_.begin(); it != pool_.end(); it++) {
      if (it->second) {
	it->second = false;
	return it->first;
      }
    }
    usleep(100*1000);
    goto retry;
  }
  void PutSession(Session* session) {
    boost::mutex::scoped_lock l(lock_);
    pool_[session] = true;
  }
private:
  boost::mutex lock_;
  std::map<Session*, bool> pool_;
};

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
   * @param[in] thread_pool_size numbuer of threads that execute asynchronous I/Os
   */
  ObjectMover(int thread_pool_size = 32, const std::string &trace_filename = "");
  ~ObjectMover();
  /**
   * Create an object in the specified tier
   *
   * @param[in] tier the tier in which to create the object
   * @param[in] object_name the name of the object
   * @param[in] value the data of the object
   * @param[out] err  0 success
   * @param[out] err <0 failure
   */
  void Create(Tier tier, const std::string &object_name, const librados::bufferlist &bl, int *err);
  void CreateAsync(Tier tier, const std::string &object_name, const librados::bufferlist &bl, int *err) {
    auto f = std::bind(&ObjectMover::Create, this, tier, object_name, bl, err);
    ios_.post(f);
  }
  /**
   * Move an object to the specified tier
   *
   * @param[in] tier the tier to which to move the object
   * @param[in] object_name the name of the object
   * @param[out] err  0 success
   * @param[out] err <0 failure
   */
  void Move(Tier tier, const std::string &object_name, int *err);
  void MoveAsync(Tier tier, const std::string &object_name, int *err) {
    auto f = std::bind(&ObjectMover::Move, this, tier, object_name, err);
    ios_.post(f);
  }
  /**
   * Read an object
   *
   * @param[in] object_name the name of the object
   * @param[out] err  0 success
   * @param[out] err <0 failure
   */
  void Read(const std::string &object_name, librados::bufferlist *bl, int *err);
  void ReadAsync(const std::string &object_name, librados::bufferlist *bl, int *err) {
    auto f = std::bind(&ObjectMover::Read, this, object_name, bl, err);
    ios_.post(f);
  }
  /**
   * Delete an object
   *
   * @param[in] object_name the name of the object
   * @param[out] err  0 success
   * @param[out] err <0 failure
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
   * @retval err  0 success
   * @retval err <0 failure
   */
  int GetLocation(const std::string &object_name);
private:
#ifdef USE_SESSION_POOL
  /**
   *
   */
  SessionPool* session_pool_;
#else
  /**
   *
   */
  std::map<boost::thread::id, Session*> sessions_;
#endif /* !USE_SESSION_POOL */
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
  /**/
  boost::asio::io_service ios_;
  boost::asio::io_service::work *w_;
  boost::thread_group thr_grp_;
  /**/
  std::string TierToString(Tier tier) {
    switch(tier) {
    case FAST:
      return "f";
    case SLOW:
      return "s";
    case ARCHIVE:
      return "a";
    default:
      abort();
    }
  }
  /**/
  std::ofstream trace_;
  boost::mutex lock_;
};

#endif
