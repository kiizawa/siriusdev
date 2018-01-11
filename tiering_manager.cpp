#include <rados/librados.hpp>
#include "tiering_manager.hpp"

int TieringManager::Create(Tier tier, const std::string &object_name, const std::string &value) {
  int r = 0;
  librados::bufferlist bl;
  bl.append(value);
  switch(tier) {
  case FAST:
  case SLOW:
    {
      librados::ObjectWriteOperation op;
      if (tier == FAST) {
	op.set_alloc_hint2(0, 0, LIBRADOS_ALLOC_HINT_FLAG_FAST_TIER);
      } else {
	op.set_alloc_hint2(0, 0, 0);
      }
      op.write_full(bl);
      // add a metadata that shows this object is stored in Storage Pool
      librados::bufferlist v;
      v.append("0");
      op.setxattr("archive", v);
      librados::AioCompletion *completion = cluster_->aio_create_completion();
      r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
      if (r != 0) {
	printf("aio_operate failed r=%d\n", r);
	completion->release();
	break;
      }
      completion->wait_for_safe();
      r = completion->get_return_value();
      completion->release();
      break;
    }
  case ARCHIVE:
    {
      {
	// 1. create an object in Archive Pool

	librados::ObjectWriteOperation op;
	op.write_full(bl);
	// add a metadata that shows this object is stored in Archive Pool
	librados::bufferlist v;
	v.append("1");
	op.setxattr("archive", v);
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_archive_->aio_operate(object_name, completion, &op);
	if (r != 0) {
	  printf("aio_operate failed r=%d\n", r);
	  completion->release();
	  break;
	}
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
	if (r != 0) {
	  printf("aio_operate failed r=%d\n", r);
	  completion->release();
	  break;
	}
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
	if (r != 0) {
	  printf("aio_operate failed r=%d\n", r);
	  completion->release();
	  break;
	}
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r != 0) {
	  printf("set_redirect failed r=%d\n", r);
	  break;
	}
      }
    }
    break;
  default:
    abort();
  }
  return r;
}

int TieringManager::Move(Tier tier, const std::string &object_name) {
  int r = 0;
  switch(tier) {
  case FAST:
  case SLOW:
    {
      // check if this object is stored in Archive Pool
      librados::ObjectWriteOperation op;
      int ret = 0;
      librados::bufferlist v;
      v.append("0");
      op.cmpxattr("archive", LIBRADOS_CMPXATTR_OP_EQ, v);
      if (tier == FAST) {
	op.set_alloc_hint2(0, 0, LIBRADOS_ALLOC_HINT_FLAG_FAST_TIER);
      } else {
	op.set_alloc_hint2(0, 0, 0);
      }
      librados::AioCompletion *completion = cluster_->aio_create_completion();
      r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
      if (r != 0) {
	printf("aio_operate failed r=%d\n", r);
	completion->release();
	break;
      }
      completion->wait_for_safe();
      r = completion->get_return_value();
      completion->release();
      if (r == -ECANCELED) {
	// The object is stored in Archive Pool.
	librados::ObjectWriteOperation op;
	// remove the redirect
	op.remove();
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
	if (r != 0) {
	  printf("aio_operate failed r=%d\n", r);
	  completion->release();
	  break;
	}
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r != 0) {
	  printf("remove failed r=%d\n", r);
	  break;
	}
	// promote the objcet to Storage Pool
	op.copy_from(object_name, *io_ctx_archive_, 0);
	// modify the metadata
	librados::bufferlist v;
	v.append("0");
	op.setxattr("archive", v);
	// 
	if (tier == FAST) {
	  op.set_alloc_hint2(0, 0, LIBRADOS_ALLOC_HINT_FLAG_FAST_TIER);
	} else {
	  op.set_alloc_hint2(0, 0, 0);
	}
	completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
	if (r != 0) {
	  printf("aio_operate failed r=%d\n", r);
	  completion->release();
	  break;
	}
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
      } else if (r != 0) {
	printf("set_alloc_hint2 failed r=%d\n", r);
	break;
      }
      break;
    }

  case ARCHIVE:
    {
      {
	librados::ObjectWriteOperation op;
	int ret = 0;
	// check if this object is stored in Storage Pool
	librados::bufferlist v1;
	v1.append("0");
	op.cmpxattr("archive", LIBRADOS_CMPXATTR_OP_EQ, v1);
	// Prevent another client from demoting this object,
	// becasue demoting the object that is already in Archive Pool seems to enter an infinite loop.
	librados::bufferlist v2;
	v2.append("2");
	op.setxattr("archive", v2);
	librados::AioCompletion *completion = cluster_->aio_create_completion();
	r = io_ctx_storage_->aio_operate(object_name, completion, &op, 0);
	if (r != 0) {
	  printf("aio_operate failed r=%d\n", r);
	  completion->release();
	  break;
	}
	completion->wait_for_safe();
	r = completion->get_return_value();
	completion->release();
	if (r == -ECANCELED) {
	  // The object is already in Archive Pool or another client is demoting it.
	  r = 0;
	  break;
	}
	r = 0;
      retry:
	{
	  // demote the object
	  int r;
	  int version;
	  librados::ObjectWriteOperation op;
	  op.copy_from(object_name, *io_ctx_storage_, 0);
	  librados::AioCompletion *completion = cluster_->aio_create_completion();
	  r = io_ctx_archive_->aio_operate(object_name, completion, &op);
	  if (r != 0) {
	    printf("aio_operate failed r=%d\n", r);
	    completion->release();
	    break;
	  }
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  if (r != 0) {
	    printf("copy_from failed! ret=%d\n", r);
	    completion->release();
	    break;
	  }
	  version = completion->get_version64();
	  completion->release();

	  // replace the object in Storage Pool with a redirect
	  op.assert_version(version);
	  op.set_redirect(object_name, *io_ctx_archive_, 1);
	  // modify the metadata
	  librados::bufferlist bl;
	  bl.append("1");
	  op.setxattr("archive", bl);
	  completion = cluster_->aio_create_completion();
	  r = io_ctx_storage_->aio_operate(object_name, completion, &op, librados::OPERATION_IGNORE_REDIRECT);
	  if (r != 0) {
	    printf("aio_operate failed r=%d\n", r);
	    completion->release();
	    break;
	  }
	  completion->wait_for_safe();
	  r = completion->get_return_value();
	  completion->release();
	  if (r != 0) {
	    // the object was modified after copy_from() completed
	    goto retry;
	  }
	}
      }
      break;
    }

  default:
    abort();
  }
  return r;
}
