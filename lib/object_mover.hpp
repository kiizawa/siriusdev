#ifndef __OBJECT_MOVER_HPP
#define __OBJECT_MOVER_HPP

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <fstream>

#include <rados/librados.hpp>

//#define USE_MICRO_TIERING
//#define DEBUG

class SessionPool;

class Session {
public:
  enum Tier {
    CACHE,
    STORAGE,
    ARCHIVE,
  };
  Session(SessionPool* session_pool);
  ~Session();
  void Connect();
  void Reconnect();
  int AioOperate(Tier tier, const std::string& oid, librados::ObjectWriteOperation *op, int flags = 0);
  librados::Rados cluster_;
  librados::IoCtx io_ctx_cache_;
  librados::IoCtx io_ctx_storage_;
  librados::IoCtx io_ctx_archive_;
private:
  SessionPool* session_pool_;
};

class SessionPool {
public:
  SessionPool(int session_pool_size) : flag_(false) {
    for (int i = 0; i < session_pool_size; i++) {
      pool_.insert(std::make_pair(new Session(this), true));
    }
  }
  ~SessionPool() {
#if 0
    std::multiset<int> counts;
    for (std::map<Session*, bool>::iterator it = pool_.begin(); it != pool_.end(); it++) {
      counts.insert(it->first->debug_count_);
    }
    std::multiset<int>::iterator it;
    for (it = counts.begin(); it != counts.end(); it++) {
      std::cout << "count " << *it << std::endl;
    }
#endif
    for (std::map<Session*, bool>::iterator it = pool_.begin(); it != pool_.end(); it++) {
      delete it->first;
    }
  }
  void ReserveSession(boost::thread::id id) {
    boost::mutex::scoped_lock l(lock_);
    for (std::map<Session*, bool>::iterator it = pool_.begin(); it != pool_.end(); it++) {
      if (it->second) {
	it->second = false;
	reserve_map_[id] = it->first;
	return;
      }
    }
    abort();
  }
  Session* GetSession(boost::thread::id id) {
    boost::mutex::scoped_lock l(lock_);
    return reserve_map_[id];
  }
  void Notify(Session* session) {
    boost::mutex::scoped_lock l(lock_);
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    debug_last_used_[session] = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  }
  void WatchSessions() {
    while (true) {
      struct timeval tv;
      ::gettimeofday(&tv, NULL);
      unsigned long now = tv.tv_sec * 1000 + tv.tv_usec / 1000;
      std::multiset<unsigned long> latencies;
      {
	boost::mutex::scoped_lock l(lock_);
	std::map<Session*, unsigned long>::const_iterator it;
	for (it = debug_last_used_.begin(); it != debug_last_used_.end(); it++) {
	  latencies.insert(now - it->second);
	}
      }
      int index = 0;
      unsigned long median;
      std::vector<unsigned long> l;
      std::multiset<unsigned long>::reverse_iterator rit = latencies.rbegin();
      while (rit != latencies.rend()) {
	if (index < 5) {
	  l.push_back(*rit);
	}
	if (index == 5) {
	  median = *rit;
	  break;
	}
	rit++;
	index++;
      }
#if 0
      if (l.size() == 5) {
	std::cout << "latencies [s] ";
	std::cout << "median = ";
	std::cout << std::setw(4) << std::right << median/1000 << " ";
	std::cout << "max = [" ;
	std::cout << std::setw(4) << std::right << l[0]/1000 << " ";
	std::cout << std::setw(4) << std::right << l[1]/1000 << " ";
	std::cout << std::setw(4) << std::right << l[2]/1000 << " ";
	std::cout << std::setw(4) << std::right << l[3]/1000 << " ";
	std::cout << std::setw(4) << std::right << l[4]/1000 << " ";
	std::cout << "]" << std::endl;
      }
#endif
      if (flag_) {
	return;
      }
      sleep(1);
    }
  }
  void End() {
    flag_ = true;
  }
private:
  boost::mutex lock_;
  std::map<Session*, bool> pool_;
  std::map<Session*, unsigned long> debug_last_used_;
  std::map<boost::thread::id, Session*> reserve_map_;
  bool flag_;
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
  void CRead(const std::string &object_name, char *buf, size_t len, int *err);
  void CReadAsync(const std::string &object_name, char *buf, size_t len, int *err) {
    auto f = std::bind(&ObjectMover::CRead, this, object_name, buf, len, err);
    ios_.post(f);
  }
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
  /**
   *
   */
  SessionPool* session_pool_;
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