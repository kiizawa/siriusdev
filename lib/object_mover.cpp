#include <rados/librados.hpp>
#include "object_mover.hpp"

//#define DEBUG
//#define SHOW_STATS

#ifdef DEBUG
#include <iostream>
#include <fstream>
std::ofstream output_file("/dev/shm/log.txt");
boost::mutex debug_lock;
#endif /* DEBUG */

#ifdef SHOW_STATS

#include <sys/time.h>
#include <set>
#include <vector>

class Stats {
public:
  Stats() {}
  ~Stats() { }
  void Insert(int t) {
    boost::mutex::scoped_lock l(m_);
    latencies_.push_back(t);
  }
  void ShowStats() {
    std::multiset<int> s;
    std::vector<int>::const_iterator it;
    for (it = latencies_.begin(); it != latencies_.end(); it++) {
      s.insert(*it);
    }
    std::multiset<int>::const_iterator it2;
    int total = s.size();
    int latency_sum = 0;
    printf("total=%d\n", total);
    int count = 0;
    bool done_25th, done_50th, done_75th = false;
    for (it2 = s.begin(); it2 != s.end(); it2++) {
      count++;
      latency_sum += *it2;
      if (done_25th == false && count > 0.25 * total) {
	printf("25th percentile=%5d\n", *it2);
	done_25th = true;
      } else if (done_50th == false && count > 0.5 * total) {
	printf("50th percentile=%5d\n", *it2);
	done_50th = true;
      } else if (done_75th == false && count > 0.75 * total) {
	printf("75th percentile=%5d\n", *it2);
	done_75th = true;
      }
    }
    printf("average        =%5d\n", latency_sum/total);
  }
private:
  boost::mutex m_;
  std::vector<int> latencies_;
};


class Timer {
public:
  Timer(Stats *stats, std::string prefix = "") : stats_(stats), prefix_(prefix)  {
    ::gettimeofday(&start_, NULL);
#ifdef DEBUG
    const long s = start_.tv_sec;
    struct tm* time_info = ::localtime(&s);
    {
      boost::mutex::scoped_lock l(debug_lock);
      output_file << prefix_ << " start  " << asctime(time_info);
    }
#endif /* DEBUG */
  }
  ~Timer() {
    ::gettimeofday(&finish_, NULL);
    int latency_in_msec =
      (1000*finish_.tv_sec + finish_.tv_usec/1000) - (1000*start_.tv_sec + start_.tv_usec/1000);
    stats_->Insert(latency_in_msec);
#ifdef DEBUG
    const long s = finish_.tv_sec;
    struct tm* time_info = ::localtime(&s);
    {
      boost::mutex::scoped_lock l(debug_lock);
      output_file << prefix_ << " finish " << asctime(time_info);
    }
    if (latency_in_msec >= 10000) {
      output_file << "warning: " << prefix_ << " took " << latency_in_msec << " [ms]" << std::endl;
    }
#endif /* DEBUG */
  }
private:
  std::string prefix_;
  struct timeval start_, finish_;
  Stats *stats_;
};

Stats stats_create;
Stats stats_move;
Stats stats_read_hdd;
Stats stats_read_ssd;

#endif /* SHOW_STATS */

class Timer2 {
public:
  Timer2(std::ofstream *ofs, boost::mutex *lock,
	 const std::string &oid, const std::string &mode, const std::string &tier)
    : ofs_(ofs), lock_(lock), oid_(oid), mode_(mode), tier_(tier) {
    struct timeval start;
    ::gettimeofday(&start, NULL);
    unsigned long start_msec = 1000*start.tv_sec + start.tv_usec/1000;
    if (ofs_->is_open()) {
      boost::mutex::scoped_lock l(*lock_);
      *ofs_ << oid_ << "," << mode_ << "," << tier_ << ",s," << start_msec << std::endl;
    }
#if 1
    start_msec_ = start_msec;
#endif
  }
  ~Timer2() {
    struct timeval finish;
    ::gettimeofday(&finish, NULL);
    unsigned long finish_msec = 1000*finish.tv_sec + finish.tv_usec/1000;
    if (ofs_->is_open()) {
      boost::mutex::scoped_lock l(*lock_);
      *ofs_ << oid_ << "," << mode_ << "," << tier_ << ",f," << finish_msec << std::endl;
    }
    if (finish_msec - start_msec_ > 10000 && mode_ == "x") {
      std::cout << "BUG!" << std::endl;
    }
  }
private:
  std::ofstream *ofs_;
  boost::mutex *lock_;
  std::string oid_;
  std::string mode_;
  std::string tier_;
#if 1
  unsigned long start_msec_;
#endif
};

Session::Session(SessionPool* session_pool) : session_pool_(session_pool) {
  Connect();
}

Session::~Session() {
  cluster_.shutdown();
}

void Session::Connect() {
  int ret;

  /* Declare the cluster handle and required variables. */
  char user_name[] = "client.admin";
  uint64_t flags;

  /* Initialize the cluster handle with the "ceph" cluster name and "client.admin" user */
  {
    ret = cluster_.init2(user_name, "ceph", flags);
    if (ret < 0) {
      std::cerr << "Couldn't initialize the cluster handle! error " << ret << std::endl;
      abort();
    } else {
      // std::cout << "Created a cluster handle." << std::endl;
    }
  }

  /* Read a Ceph configuration file to configure the cluster handle. */
  {
    ret = cluster_.conf_read_file("/share/ceph.conf");
    if (ret < 0) {
      std::cerr << "Couldn't read the Ceph configuration file! error " << ret << std::endl;
      abort();
    } else {
      // std::cout << "Read the Ceph configuration file." << std::endl;
    }
  }

  /* Connect to the cluster */
  {
    ret = cluster_.connect();
    if (ret < 0) {
      std::cerr << "Couldn't connect to cluster! error " << ret << std::endl;
      abort();
    } else {
      // std::cout << "Connected to the cluster." << std::endl;
    }
  }

  if (cluster_.pool_lookup("cache_pool") != -ENOENT) {
    ret = cluster_.ioctx_create("cache_pool", io_ctx_cache_);
    if (ret < 0) {
      std::cerr << "Couldn't set up ioctx! error " << ret << std::endl;
      abort();
    } else {
      // std::cout << "Created an ioctx for the pool." << std::endl;
    }
  }

  if (cluster_.pool_lookup("storage_pool") != -ENOENT) {
    ret = cluster_.ioctx_create("storage_pool", io_ctx_storage_);
    if (ret < 0) {
      std::cerr << "Couldn't set up ioctx! error " << ret << std::endl;
      abort();
    } else {
      // std::cout << "Created an ioctx for the pool." << std::endl;
    }
  }

  if (cluster_.pool_lookup("archive_pool") != -ENOENT) {
    ret = cluster_.ioctx_create("archive_pool", io_ctx_archive_);
    if (ret < 0) {
      std::cerr << "Couldn't set up ioctx! error " << ret << std::endl;
      abort();
    } else {
      // std::cout << "Created an ioctx for the pool." << std::endl;
    }
  }
}

void Session::Reconnect() {
  cluster_.shutdown();
  Connect();
}

int Session::AioOperate(Tier tier, const std::string& oid, librados::ObjectWriteOperation *op, int flags) {
  int r;
  librados::AioCompletion *completion = cluster_.aio_create_completion();
  session_pool_->Notify(this);
  switch (tier) {
  case CACHE:
    r = io_ctx_cache_.aio_operate(oid, completion, op, flags);
    break;
  case STORAGE:
    r = io_ctx_storage_.aio_operate(oid, completion, op, flags);
    break;
  case ARCHIVE:
    r = io_ctx_archive_.aio_operate(oid, completion, op, flags);
    break;
  default:
    abort();
  }
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  return r;
}

ObjectMover::ObjectMover(int thread_pool_size, const std::string &trace_filename) {
  w_ = new boost::asio::io_service::work(ios_);
  session_pool_ = new SessionPool(thread_pool_size);
  for (int i = 0; i < thread_pool_size; ++i) {
    boost::thread* t = thr_grp_.create_thread(boost::bind(&boost::asio::io_service::run, &ios_));
    session_pool_->ReserveSession(t->get_id());
  }
  thr_grp_.create_thread(boost::bind(&SessionPool::WatchSessions, session_pool_));
  if (!trace_filename.empty()) {
    trace_.open(trace_filename);
  }
}

ObjectMover::~ObjectMover() {
  session_pool_->End();
  delete w_;
  thr_grp_.join_all();
  delete session_pool_;
#ifdef SHOW_STATS
  printf("stats (Create)\n");
  stats_create.ShowStats();
  printf("stats (Read from HDD)\n");
  stats_read_hdd.ShowStats();
  printf("stats (Move)\n");
  stats_move.ShowStats();
  printf("stats (Read from SSD)\n");
  stats_read_ssd.ShowStats();
#endif /* SHOW_STATS */
#ifdef DEBUG
  output_file.close();
#endif /* DEBUG */
  if (trace_.is_open()) {
    trace_.close();
  }
}

void ObjectMover::Create(Tier tier, const std::string &object_name, const librados::bufferlist &bl, int *err) {
  Timer2 t(&trace_, &lock_, object_name, "w", TierToString(tier));
  int r;
#ifdef USE_MICRO_TIERING
  switch(tier) {
  case FAST:
  case SLOW:
    {
      librados::ObjectWriteOperation op;
      librados::bufferlist v;
      if (tier == FAST) {
	v.append("fast");
	op.set_alloc_hint2(0, 0, LIBRADOS_ALLOC_HINT_FLAG_FAST_TIER);
      } else {
	v.append("slow");
	op.set_alloc_hint2(0, 0, 0);
      }
      op.write_full(bl);
      op.setxattr("tier", v);
      Session *s = session_pool_->GetSession(boost::this_thread::get_id());
      r = s->AioOperate(Session::STORAGE, object_name, &op);
      if (r != 0) {
	printf("write_full/setxattr failed r=%d\n", r);
	break;
      }
      break;
    }
  case ARCHIVE:
    {
      {
	// 1. create an object in Archive Pool
	librados::ObjectWriteOperation op;
	op.write_full(bl);
	librados::bufferlist v;
	v.append("archive");
	op.setxattr("tier", v);
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	r = s->AioOperate(Session::ARCHIVE, object_name, &op);
	if (r != 0) {
	  printf("write_full/setxattr failed r=%d\n", r);
	  break;
	}
      }
      {
	// 2. create a dummy object in Storage Pool

	librados::ObjectWriteOperation op;
	librados::bufferlist bl_dummy;
	op.write_full(bl_dummy);
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	r = s->AioOperate(Session::STORAGE, object_name, &op);
	if (r != 0) {
	  printf("write_full failed r=%d\n", r);
	  break;
	}
      }
      {
	// 3. replace the dummy object in Storage Pool with a redirect

	librados::ObjectWriteOperation op;
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	op.set_redirect(object_name, s->io_ctx_archive_, 0);
	r = s->AioOperate(Session::STORAGE, object_name, &op);
	if (r != 0) {
	  printf("set_redirect failed r=%d\n", r);
	  break;
	}
      }
      break;
    }

  default:
    abort();
  }
#else
  switch(tier) {
  case FAST:
    {
      librados::ObjectWriteOperation op;
      op.write_full(bl);
      librados::bufferlist v;
      v.append("fast");
      op.setxattr("tier", v);
      Session *s = session_pool_->GetSession(boost::this_thread::get_id());
      librados::AioCompletion *completion = s->cluster_.aio_create_completion();
      r = s->io_ctx_cache_.aio_operate(object_name, completion, &op);
      assert(r == 0);
      completion->wait_for_safe();
      r = completion->get_return_value();
      completion->release();
      if (r != 0) {
	printf("write_full/setxattr failed r=%d\n", r);
	break;
      }
      break;
    }

  case SLOW:
  case ARCHIVE:
    {
      {
	// 1. create an object in Slow/Archive Pool
	librados::ObjectWriteOperation op;
	op.write_full(bl);
	librados::bufferlist v;
	if (tier == SLOW) {
	  v.append("slow");
	} else {
	  v.append("archive");
	}
	op.setxattr("tier", v);
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	if (tier == SLOW) {
	  r = s->io_ctx_storage_.aio_operate(object_name, completion, &op);
	} else {
	  assert(tier == ARCHIVE);
	  r = s->io_ctx_archive_.aio_operate(object_name, completion, &op);
	}
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r != 0) {
	  printf("write_full/setxattr failed r=%d\n", r);
	  break;
	}
      }
      {
	// 2. create a dummy object in Cache Pool

	librados::ObjectWriteOperation op;
	librados::bufferlist bl_dummy;
	op.write_full(bl_dummy);
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_cache_.aio_operate(object_name, completion, &op);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r != 0) {
	  printf("write_full failed r=%d\n", r);
	  break;
	}
      }
      {
	// 3. replace the dummy object in Cache Pool with a redirect

	librados::ObjectWriteOperation op;
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	if (tier == SLOW) {
	  op.set_redirect(object_name, s->io_ctx_storage_, 0);
	} else {
	  assert(tier == ARCHIVE);
	  op.set_redirect(object_name, s->io_ctx_archive_, 0);
	}
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_cache_.aio_operate(object_name, completion, &op);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r != 0) {
	  printf("set_redirect failed r=%d\n", r);
	  break;
	}
      }
      break;
    }

  default:
    abort();
  }
#endif /* !USE_MICRO_TIERING */
  *err = r;
}

void ObjectMover::Read(const std::string &object_name, librados::bufferlist *bl, int *err) {
  Timer2 t(&trace_, &lock_, object_name, "r", "-");
  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
#ifdef USE_MICRO_TIERING
  *err = s->io_ctx_storage_.read(object_name, *bl, 0, 0);
#else
  *err = s->io_ctx_cache_.read(object_name, *bl, 0, 0);
#endif /* !USE_MICRO_TIERING */
}

void ObjectMover::Move(Tier tier, const std::string &object_name, int *err) {
#ifdef USE_MICRO_TIERING
  Timer2 t(&trace_, &lock_, object_name, "m", TierToString(tier));
  int r = 0;
  switch(tier) {
  case FAST:
  case SLOW:
    {
      Lock(object_name);
      int current_tier = GetLocation(object_name);
      if (current_tier == tier) {
	// the object is already stored in specified tier
	Unlock(object_name);
	break;
      }
      if (current_tier == ARCHIVE) {
	// remove the redirect in Storage Pool
	librados::ObjectWriteOperation op;
	op.remove();
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r != 0) {
	  Unlock(object_name);
	  printf("remove failed r=%d\n", r);
	  *err = r;
	  return;
	}
	// TODO: the following operations are not atomic!
	// promote the objcet to Storage Pool
	librados::ObjectWriteOperation op2;
	op2.copy_from(object_name, s->io_ctx_archive_, 0);
	// modify the metadata
	librados::bufferlist v2;
	if (tier == FAST) {
	  v2.append("fast");
	  op2.set_alloc_hint2(0, 0, LIBRADOS_ALLOC_HINT_FLAG_FAST_TIER);
	} else {
	  v2.append("slow");
	  op2.set_alloc_hint2(0, 0, 0);
	}
	op2.setxattr("tier", v2);
	completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_storage_.aio_operate(object_name, completion, &op2, 0);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
        if (r != 0) {
	  Unlock(object_name);
	  printf("set_alloc_hint2 failed r=%d\n", r);
	  break;
	}
	// remove the object in Archive Pool
	librados::ObjectWriteOperation op3;
	op3.remove();
	completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_archive_.aio_operate(object_name, completion, &op3);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
        if (r != 0) {
	  Unlock(object_name);
	  printf("remove failed r=%d\n", r);
	  break;
	}
      } else {
	librados::ObjectWriteOperation op;
	librados::bufferlist v;
	if (tier == FAST) {
	  v.append("fast");
	  op.set_alloc_hint2(0, 0, LIBRADOS_ALLOC_HINT_FLAG_FAST_TIER);
	} else {
	  v.append("slow");
	  op.set_alloc_hint2(0, 0, 0);
	}
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
        if (r != 0) {
	  Unlock(object_name);
	  printf("set_alloc_hint2 failed r=%d\n", r);
	  break;
	}
      }
      Unlock(object_name);
      break;
    }

  case ARCHIVE:
    {
      Lock(object_name);
      if (GetLocation(object_name) == ARCHIVE) {
	// the object is already stored in Archive Pool
	Unlock(object_name);
	break;
      }
    retry_copy:
      {
	// demote the object
	int r;
	int version;
	librados::ObjectWriteOperation op;
	Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	op.copy_from(object_name, s->io_ctx_storage_, 0);
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_archive_.aio_operate(object_name, completion, &op);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	if (r != 0) {
	  printf("copy_from failed! r=%d\n", r);
	  completion->release();
	  Unlock(object_name);
	  break;
	}
	version = completion->get_version64();
	completion->release();

	// replace the object in Storage Pool with a redirect
	op.assert_version(version);
	op.set_redirect(object_name, s->io_ctx_archive_, 1);

	// modify the metadata
	librados::bufferlist bl;
	bl.append("archive");
	op.setxattr("tier", bl);
	completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r != 0) {
	  // the object was modified after copy_from() completed
	  goto retry_copy;
	}
      }
      Unlock(object_name);
      break;
    }

  default:
    abort();
  }
  *err = r;
#else
  Timer2 t(&trace_, &lock_, object_name, "m", TierToString(tier));
  int r = 0;

  switch(tier) {
  case FAST:
    {
      Lock(object_name);
      int current_tier = GetLocation(object_name);
      if (current_tier == tier) {
	// the object is already stored in specified tier
	Unlock(object_name);
	break;
      }
      // remove the redirect in Cache Pool
      librados::ObjectWriteOperation op;
      op.remove();
      Session *s = session_pool_->GetSession(boost::this_thread::get_id());
      librados::AioCompletion *completion = s->cluster_.aio_create_completion();
      r = s->io_ctx_cache_.aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
      assert(r == 0);
      completion->wait_for_safe();
      r = completion->get_return_value();
      completion->release();
      if (r != 0) {
	Unlock(object_name);
	printf("remove failed r=%d\n", r);
	*err = r;
	return;
      }
      // TODO: the following operations are not atomic!
      // promote the objcet to Cache Pool
      librados::ObjectWriteOperation op2;
      if (current_tier == SLOW) {
	op2.copy_from(object_name, s->io_ctx_storage_, 0);
      } else {
	assert(current_tier == ARCHIVE);
	op2.copy_from(object_name, s->io_ctx_archive_, 0);
      }
      // modify the metadata
      librados::bufferlist v2;
      v2.append("fast");
      op2.setxattr("tier", v2);
      completion = s->cluster_.aio_create_completion();
      r = s->io_ctx_cache_.aio_operate(object_name, completion, &op2, 0);
      assert(r == 0);
      completion->wait_for_safe();
      r = completion->get_return_value();
      completion->release();
      if (r != 0) {
	Unlock(object_name);
	printf("setxattr failed r=%d\n", r);
	break;
      }
      // remove the object in Storage/Archive Pool
      librados::ObjectWriteOperation op3;
      op3.remove();
      completion = s->cluster_.aio_create_completion();
      if (current_tier == SLOW) {
	r = s->io_ctx_storage_.aio_operate(object_name, completion, &op3);
      } else {
	assert(current_tier == ARCHIVE);
	r = s->io_ctx_archive_.aio_operate(object_name, completion, &op3);
      }
      assert(r == 0);
      completion->wait_for_safe();
      r = completion->get_return_value();
      completion->release();
      if (r != 0) {
	Unlock(object_name);
	printf("remove failed r=%d\n", r);
	break;
      }
      Unlock(object_name);
      break;
    }

  case SLOW:
  case ARCHIVE:
    {
      Lock(object_name);
      int current_tier = GetLocation(object_name);
      if (current_tier == tier) {
	// the object is already stored in Storage/Archive Pool
	Unlock(object_name);
	break;
      }
      if (current_tier == FAST) {

	/* Move from Fast tier to Storage/Archive tier */

      retry_copy:
	{
	  // demote the object
	  int r;
	  int version;
	  librados::ObjectWriteOperation op;
	  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	  op.copy_from(object_name, s->io_ctx_cache_, 0);
	  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	  if (tier == SLOW) {
	    r = s->io_ctx_storage_.aio_operate(object_name, completion, &op);
	  } else {
	    assert(tier == ARCHIVE);
	    r = s->io_ctx_archive_.aio_operate(object_name, completion, &op);
	  }
	  assert(r == 0);
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  if (r != 0) {
	    printf("copy_from failed! r=%d\n", r);
	    completion->release();
	    Unlock(object_name);
	    break;
	  }
	  version = completion->get_version64();
	  completion->release();

	  // replace the object in Cache Pool with a redirect
	  op.assert_version(version);
	  if (tier == SLOW) {
	    op.set_redirect(object_name, s->io_ctx_storage_, 1);
	  } else {
	    assert(tier == ARCHIVE);
	    op.set_redirect(object_name, s->io_ctx_archive_, 1);
	  }
	  // modify the metadata
	  librados::bufferlist bl;
	  if (tier == SLOW) {
	    bl.append("storage");
	  } else {
	    assert(tier == ARCHIVE);
	    bl.append("archive");
	  }
	  op.setxattr("tier", bl);
	  completion = s->cluster_.aio_create_completion();
	  r = s->io_ctx_cache_.aio_operate(object_name, completion, &op, 0);
	  assert(r == 0);
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  completion->release();
	  if (r != 0) {
	    // the object was modified after copy_from() completed
	    goto retry_copy;
	  }
	}
      } else {

	/* move between Storage <--> Archive tiers */

	{
	  // 1. remove the redirect in Cache Pool.
	  librados::ObjectWriteOperation op;
	  op.remove();
	  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	  r = s->io_ctx_cache_.aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
	  assert(r == 0);
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  completion->release();
	  if (r != 0) {
	    printf("remove failed r=%d\n", r);
	    Unlock(object_name);
	    break;
	  }
	}

	{
	  // 2. demote the object
	  int r;
	  librados::ObjectWriteOperation op;
	  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	  if (tier == SLOW) {
	    assert(current_tier == ARCHIVE);
	    op.copy_from(object_name, s->io_ctx_archive_, 0);
	    r = s->io_ctx_storage_.aio_operate(object_name, completion, &op);
	  } else {
	    assert(current_tier == SLOW);
	    op.copy_from(object_name, s->io_ctx_storage_, 0);
	    r = s->io_ctx_archive_.aio_operate(object_name, completion, &op);
	  }
	  assert(r == 0);
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  completion->release();
	  if (r != 0) {
	    printf("copy_from failed! r=%d\n", r);
	    Unlock(object_name);
	    break;
	  }
	}

	{
	  // 3. remove the object in Storage/Archive Pool.
	  librados::ObjectWriteOperation op;
	  op.remove();
	  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	  if (current_tier == SLOW) {
	    r = s->io_ctx_storage_.aio_operate(object_name, completion, &op);
	  } else {
	    assert(current_tier == ARCHIVE);
	    r = s->io_ctx_archive_.aio_operate(object_name, completion, &op);
	  }
	  assert(r == 0);
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  completion->release();
	  if (r != 0) {
	    printf("remove failed r=%d\n", r);
	    Unlock(object_name);
	    break;
	  }
	}

	{
	  // 4. create a dummy object in Cache Pool

	  librados::ObjectWriteOperation op;
	  librados::bufferlist bl_dummy;
	  op.write_full(bl_dummy);
	  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	  r = s->io_ctx_cache_.aio_operate(object_name, completion, &op);
	  assert(r == 0);
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  completion->release();
	  if (r != 0) {
	    printf("write_full failed r=%d\n", r);
	    break;
	  }
	}

	{
	  // 5. replace the dummy object in Cache Pool with a redirect

	  librados::ObjectWriteOperation op;
	  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
	  if (tier == SLOW) {
	    op.set_redirect(object_name, s->io_ctx_storage_, 0);
	  } else {
	    assert(tier == ARCHIVE);
	    op.set_redirect(object_name, s->io_ctx_archive_, 0);
	  }
	  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	  r = s->io_ctx_cache_.aio_operate(object_name, completion, &op);
	  assert(r == 0);
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  completion->release();
	  if (r != 0) {
	    printf("set_redirect failed r=%d\n", r);
	    break;
	  }
	}
      }
      Unlock(object_name);
      break;
    }

  default:
    abort();
  }
  *err = r;
#endif /* !USE_MICRO_TIERING */
}

int ObjectMover::GetLocation(const std::string &object_name) {
  int r;
  librados::ObjectReadOperation op;
  librados::bufferlist bl;
  int err;
  op.getxattr("tier", &bl, &err);
  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
#ifdef USE_MICRO_TIERING
  r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0, NULL);
#else
  r = s->io_ctx_cache_.aio_operate(object_name, completion, &op, 0, NULL);
#endif /* !USE_MICRO_TIERING */
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  assert(r == 0);
  std::string tier_string(bl.c_str(), bl.length());
  int tier = -1;
  if (tier_string == "fast") {
    tier = FAST;
  } else if (tier_string == "slow") {
    tier = SLOW;
  } else if (tier_string == "archive") {
    tier = ARCHIVE;
  } else {
    abort();
  }
  return tier;
}

void ObjectMover::Lock(const std::string &object_name) {
 retry_lock:
  int r = 0;
  librados::ObjectWriteOperation op;
  librados::bufferlist v1;
  v1.append("1");
  op.cmpxattr("lock", LIBRADOS_CMPXATTR_OP_NE, v1);
  librados::bufferlist v2;
  v2.append("1");
  op.setxattr("lock", v2);
  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
#ifdef USE_MICRO_TIERING
  r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
#else
  r = s->io_ctx_cache_.aio_operate(object_name, completion, &op, 0);
#endif /* !USE_MICRO_TIERING */
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  if (r == -ECANCELED) {
    goto retry_lock;
  }
  assert(r == 0);
}

void ObjectMover::Unlock(const std::string &object_name) {
  int r = 0;
  librados::ObjectWriteOperation op;
  librados::bufferlist v1;
  v1.append("1");
  op.cmpxattr("lock", LIBRADOS_CMPXATTR_OP_EQ, v1);
  librados::bufferlist v2;
  v2.append("0");
  op.setxattr("lock", v2);
  Session *s = session_pool_->GetSession(boost::this_thread::get_id());
  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
#ifdef USE_MICRO_TIERING
  r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
#else
  r = s->io_ctx_cache_.aio_operate(object_name, completion, &op, 0);
#endif /* !USE_MICRO_TIERING */
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  assert(r == 0);
}

void ObjectMover::Delete(const std::string &object_name, int *err) {
  int r;
#ifdef USE_MICRO_TIERING
  {
    // If the object is a redirect, remove the target.
    librados::ObjectWriteOperation op;
    op.remove();
    Session *s = session_pool_->GetSession(boost::this_thread::get_id());
    librados::AioCompletion *completion = s->cluster_.aio_create_completion();
    r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
    assert(r == 0);
    completion->wait_for_safe();
    r = completion->get_return_value();
    completion->release();
    if (r != 0) {
      printf("remove failed r=%d\n", r);
      *err = r;
      return;
    }
  }
  {
    // If the object is a redirect, remove the redirect itself.
    librados::ObjectWriteOperation op;
    op.remove();
    Session *s = session_pool_->GetSession(boost::this_thread::get_id());
    librados::AioCompletion *completion = s->cluster_.aio_create_completion();
    r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
    assert(r == 0);
    completion->wait_for_safe();
    r = completion->get_return_value();
    completion->release();
    if (r == -ENOENT) {
      r = 0;
    }
  }
#else
  {
    // If the object is a redirect, remove the target.
    librados::ObjectWriteOperation op;
    op.remove();
    Session *s = session_pool_->GetSession(boost::this_thread::get_id());
    librados::AioCompletion *completion = s->cluster_.aio_create_completion();
    r = s->io_ctx_cache_.aio_operate(object_name, completion, &op, 0);
    assert(r == 0);
    completion->wait_for_safe();
    r = completion->get_return_value();
    completion->release();
    if (r != 0) {
      printf("remove failed r=%d\n", r);
      *err = r;
      return;
    }
  }
  {
    // If the object is a redirect, remove the redirect itself.
    librados::ObjectWriteOperation op;
    op.remove();
    Session *s = session_pool_->GetSession(boost::this_thread::get_id());
    librados::AioCompletion *completion = s->cluster_.aio_create_completion();
    r = s->io_ctx_cache_.aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
    assert(r == 0);
    completion->wait_for_safe();
    r = completion->get_return_value();
    completion->release();
    if (r == -ENOENT) {
      r = 0;
    }
  }
#endif /* !USE_MICRO_TIERING */
  *err = r;
}
