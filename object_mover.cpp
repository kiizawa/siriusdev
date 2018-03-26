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
    int start_msec = 1000*start.tv_sec + start.tv_usec/1000;
    if (ofs_->is_open()) {
      boost::mutex::scoped_lock l(*lock_);
      *ofs_ << oid_ << "," << mode_ << "," << tier_ << ",s," << start_msec << std::endl;
    }
  }
  ~Timer2() {
    struct timeval finish;
    ::gettimeofday(&finish, NULL);
    int finish_msec = 1000*finish.tv_sec + finish.tv_usec/1000;
    if (ofs_->is_open()) {
      boost::mutex::scoped_lock l(*lock_);
      *ofs_ << oid_ << "," << mode_ << "," << tier_ << ",f," << finish_msec << std::endl;
    }
  }
private:
  std::ofstream *ofs_;
  boost::mutex *lock_;
  std::string oid_;
  std::string mode_;
  std::string tier_;
};

Session::Session() {

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

  {
    ret = cluster_.ioctx_create("storage_pool", io_ctx_storage_);
    if (ret < 0) {
      std::cerr << "Couldn't set up ioctx! error " << ret << std::endl;
      abort();
    } else {
      // std::cout << "Created an ioctx for the pool." << std::endl;
    }
  }

  {
    ret = cluster_.ioctx_create("archive_pool", io_ctx_archive_);
    if (ret < 0) {
      std::cerr << "Couldn't set up ioctx! error " << ret << std::endl;
      abort();
    } else {
      // std::cout << "Created an ioctx for the pool." << std::endl;
    }
  }
}

Session::~Session() {
  cluster_.shutdown();
}

ObjectMover::ObjectMover(int thread_pool_size, const std::string &trace_filename) {
  w_ = new boost::asio::io_service::work(ios_);
  for (int i = 0; i < thread_pool_size; ++i) {
    boost::thread* t = thr_grp_.create_thread(boost::bind(&boost::asio::io_service::run, &ios_));
    sessions_.insert(std::make_pair(t->get_id(), new Session));
  }
  if (!trace_filename.empty()) {
    trace_.open(trace_filename);
  }
}

ObjectMover::~ObjectMover() {
  std::map<boost::thread::id, Session*>::iterator it;
  for (it = sessions_.begin(); it != sessions_.end(); it++) {
    delete it->second;
  }
  delete w_;
  thr_grp_.join_all();
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
  int r = 0;
  switch(tier) {
  case FAST:
  case SLOW:
    {
      int r;
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
      Session *s = sessions_[boost::this_thread::get_id()];
      librados::AioCompletion *completion = s->cluster_.aio_create_completion();
      r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
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
  case ARCHIVE:
    {
      {
	// 1. create an object in Archive Pool

	librados::ObjectWriteOperation op;
	op.write_full(bl);
	librados::bufferlist v;
	v.append("archive");
	op.setxattr("tier", v);
	Session *s = sessions_[boost::this_thread::get_id()];
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_archive_.aio_operate(object_name, completion, &op);
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
	// 2. create a dummy object in Storage Pool

	librados::ObjectWriteOperation op;
	librados::bufferlist bl_dummy;
	op.write_full(bl_dummy);
	Session *s = sessions_[boost::this_thread::get_id()];
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_storage_.aio_operate(object_name, completion, &op);
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
	// 3. replace the dummy object in Storage Pool with a redirect

	librados::ObjectWriteOperation op;
	Session *s = sessions_[boost::this_thread::get_id()];
	op.set_redirect(object_name, s->io_ctx_archive_, 0);
	librados::AioCompletion *completion = s->cluster_.aio_create_completion();
	r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
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
  *err = r;
}

void ObjectMover::Read(const std::string &object_name, librados::bufferlist *bl, int *err) {
  Timer2 t(&trace_, &lock_, object_name, "r", "-");
  Session *s = sessions_[boost::this_thread::get_id()];
  *err = s->io_ctx_storage_.read(object_name, *bl, 0, 0);
}

void ObjectMover::Move(Tier tier, const std::string &object_name, int *err) {
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
	Session *s = sessions_[boost::this_thread::get_id()];
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
	Session *s = sessions_[boost::this_thread::get_id()];
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
	Session *s = sessions_[boost::this_thread::get_id()];
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
}

int ObjectMover::GetLocation(const std::string &object_name) {
  int r;
  librados::ObjectReadOperation op;
  librados::bufferlist bl;
  int err;
  op.getxattr("tier", &bl, &err);
  Session *s = sessions_[boost::this_thread::get_id()];
  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
  r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0, NULL);
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
  Session *s = sessions_[boost::this_thread::get_id()];
  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
  r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
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
  Session *s = sessions_[boost::this_thread::get_id()];
  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
  r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, 0);
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  assert(r == 0);
}

void ObjectMover::Delete(const std::string &object_name, int *err) {
  int r;

  // Remove the object in Storage Pool.
  librados::ObjectWriteOperation op;
  op.remove();
  Session *s = sessions_[boost::this_thread::get_id()];
  librados::AioCompletion *completion = s->cluster_.aio_create_completion();
  r = s->io_ctx_storage_.aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  if (r != 0) {
    printf("remove failed r=%d\n", r);
    *err = r;
    return;
  }

  // If the object exists in Archive Pool, remove it too.
  op.assert_exists();
  op.remove();
  completion = s->cluster_.aio_create_completion();
  r = s->io_ctx_archive_.aio_operate(object_name, completion, &op);
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  if (r == -ENOENT) {
    r = 0;
  }
  *err = r;
  return;
}
