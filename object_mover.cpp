#include <rados/librados.hpp>
#include "object_mover.hpp"

int ObjectMover::Create(Tier tier, const std::string &object_name, const std::string &value) {
  int r = 0;
  librados::bufferlist bl;
  bl.append(value);
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
      librados::AioCompletion *completion = cluster_->aio_create_completion();
      r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
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
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_archive_->aio_operate(object_name, completion, &op);
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
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op);
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
	op.set_redirect(object_name, *io_ctx_archive_, 0);
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
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
  return r;
}

int ObjectMover::Move(Tier tier, const std::string &object_name) {
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
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
	assert(r == 0);
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r != 0) {
	  Unlock(object_name);
	  printf("remove failed r=%d\n", r);
	  return r;
	}
	// TODO: the following operations are not atomic!
	// promote the objcet to Storage Pool
	librados::ObjectWriteOperation op2;
	op2.copy_from(object_name, *io_ctx_archive_, 0);
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
	completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op2, 0);
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
	completion = cluster_->aio_create_completion();
	r = io_ctx_archive_->aio_operate(object_name, completion, &op3);
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
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
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
	op.copy_from(object_name, *io_ctx_storage_, 0);
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_archive_->aio_operate(object_name, completion, &op);
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
	op.set_redirect(object_name, *io_ctx_archive_, 1);

	// modify the metadata
	librados::bufferlist bl;
	bl.append("archive");
	op.setxattr("tier", bl);
	completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
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
  return r;
}

int ObjectMover::GetLocation(const std::string &object_name) {
  int r;
  librados::ObjectReadOperation op;
  librados::bufferlist bl;
  int err;
  op.getxattr("tier", &bl, &err);
  librados::AioCompletion *completion = cluster_->aio_create_completion();
  r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0, NULL);
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
  librados::AioCompletion *completion = cluster_->aio_create_completion();
  r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
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
  librados::AioCompletion *completion = cluster_->aio_create_completion();
  r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  assert(r == 0);
}

int ObjectMover::Delete(const std::string &object_name) {
  int r;

  // Remove the object in Storage Pool.
  librados::ObjectWriteOperation op;
  op.remove();
  librados::AioCompletion *completion = cluster_->aio_create_completion();
  r = io_ctx_storage_->aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  if (r != 0) {
    printf("remove failed r=%d\n", r);
    return r;
  }

  // If the object exists in Archive Pool, remove it too.
  op.assert_exists();
  op.remove();
  completion = cluster_->aio_create_completion();
  r = io_ctx_archive_->aio_operate(object_name, completion, &op);
  assert(r == 0);
  completion->wait_for_safe();
  r = completion->get_return_value();
  completion->release();
  if (r == ECANCELED) {
    r = 0;
  }
  return r;
}
