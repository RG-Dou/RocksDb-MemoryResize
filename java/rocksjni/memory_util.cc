// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include <jni.h>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "include/org_rocksdb_MemoryUtil.h"

#include "rocksjni/portal.h"

#include "rocksdb/utilities/memory_util.h"


/*
 * Class:     org_rocksdb_MemoryUtil
 * Method:    getApproximateMemoryUsageByType
 * Signature: ([J[J)Ljava/util/Map;
 */
jobject Java_org_rocksdb_MemoryUtil_getApproximateMemoryUsageByType(
    JNIEnv *env, jclass /*jclazz*/, jlongArray jdb_handles, jlongArray jcache_handles) {
  std::vector<ROCKSDB_NAMESPACE::DB *> dbs;
  jsize db_handle_count = env->GetArrayLength(jdb_handles);
  if(db_handle_count > 0) {
    jlong *ptr_jdb_handles = env->GetLongArrayElements(jdb_handles, nullptr);
    if (ptr_jdb_handles == nullptr) {
      // exception thrown: OutOfMemoryError
      return nullptr;
    }
    for (jsize i = 0; i < db_handle_count; i++) {
      dbs.push_back(
          reinterpret_cast<ROCKSDB_NAMESPACE::DB *>(ptr_jdb_handles[i]));
    }
    env->ReleaseLongArrayElements(jdb_handles, ptr_jdb_handles, JNI_ABORT);
  }

  std::unordered_set<const ROCKSDB_NAMESPACE::Cache *> cache_set;
  jsize cache_handle_count = env->GetArrayLength(jcache_handles);
  if(cache_handle_count > 0) {
    jlong *ptr_jcache_handles = env->GetLongArrayElements(jcache_handles, nullptr);
    if (ptr_jcache_handles == nullptr) {
      // exception thrown: OutOfMemoryError
      return nullptr;
    }
    for (jsize i = 0; i < cache_handle_count; i++) {
      auto *cache_ptr =
          reinterpret_cast<std::shared_ptr<ROCKSDB_NAMESPACE::Cache> *>(
              ptr_jcache_handles[i]);
      cache_set.insert(cache_ptr->get());
    }
    env->ReleaseLongArrayElements(jcache_handles, ptr_jcache_handles, JNI_ABORT);
  }

  std::map<ROCKSDB_NAMESPACE::MemoryUtil::UsageType, uint64_t> usage_by_type;
  if (ROCKSDB_NAMESPACE::MemoryUtil::GetApproximateMemoryUsageByType(
          dbs, cache_set, &usage_by_type) != ROCKSDB_NAMESPACE::Status::OK()) {
    // Non-OK status
    return nullptr;
  }

  jobject jusage_by_type = ROCKSDB_NAMESPACE::HashMapJni::construct(
      env, static_cast<uint32_t>(usage_by_type.size()));
  if (jusage_by_type == nullptr) {
    // exception occurred
    return nullptr;
  }
  const ROCKSDB_NAMESPACE::HashMapJni::FnMapKV<
      const ROCKSDB_NAMESPACE::MemoryUtil::UsageType, const uint64_t, jobject,
      jobject>
      fn_map_kv = [env](
                      const std::pair<ROCKSDB_NAMESPACE::MemoryUtil::UsageType,
                                      uint64_t> &pair) {
        // Construct key
        const jobject jusage_type = ROCKSDB_NAMESPACE::ByteJni::valueOf(
            env, ROCKSDB_NAMESPACE::MemoryUsageTypeJni::toJavaMemoryUsageType(
                     pair.first));
        if (jusage_type == nullptr) {
          // an error occurred
          return std::unique_ptr<std::pair<jobject, jobject>>(nullptr);
        }
        // Construct value
        const jobject jusage_value =
            ROCKSDB_NAMESPACE::LongJni::valueOf(env, pair.second);
        if (jusage_value == nullptr) {
          // an error occurred
          return std::unique_ptr<std::pair<jobject, jobject>>(nullptr);
        }
        // Construct and return pointer to pair of jobjects
        return std::unique_ptr<std::pair<jobject, jobject>>(
            new std::pair<jobject, jobject>(jusage_type,
                                            jusage_value));
      };

  if (!ROCKSDB_NAMESPACE::HashMapJni::putAll(env, jusage_by_type,
                                             usage_by_type.begin(),
                                             usage_by_type.end(), fn_map_kv)) {
    // exception occcurred
    jusage_by_type = nullptr;
  }

  return jusage_by_type;

}

//Dr.G
/*
 * Class:     org_rocksdb_MemoryUtil
 * Method:    cacheResize
 * Signature: ([J[J)Ljava/util/Long;
 */
jobject Java_org_rocksdb_MemoryUtil_cacheResize(
    JNIEnv *env, jclass /*jclazz*/, jlong jcache_handle, jlong target_size) {


  auto *cache_ptr = reinterpret_cast<std::shared_ptr<ROCKSDB_NAMESPACE::Cache> *>(
              jcache_handle);

//  compact_opts->output_file_size_limit =
//      static_cast<uint64_t>(joutput_file_size_limit);
  uint64_t final_size = ROCKSDB_NAMESPACE::MemoryUtil::CacheResize(
      *cache_ptr, static_cast<uint64_t>(target_size));

  const jobject jfinal_size = ROCKSDB_NAMESPACE::LongJni::valueOf(env, final_size);

  return jfinal_size;

}
