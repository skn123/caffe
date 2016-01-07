#ifndef CAFFE_SYNCEDMEM_HPP_
#define CAFFE_SYNCEDMEM_HPP_

#include <cstdlib>

#include "caffe/common.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

// Theoretically, CaffeMallocHost and CaffeFreeHost should simply call the
// cudaMallocHost and cudaFree functions in order to create pinned memory.
// However, those codes rely on the existence of a cuda GPU (I don't know
// why that is a must since allocating memory should not be accessing the
// GPU resource, but it just creates an error as of Cuda 5.0) and will cause
// problem when running on a machine without GPU. Thus, we simply define
// these two functions for safety and possible future change if the problem
// of calling cuda functions disappears in a future version.
//
// In practice, although we are creating unpinned memory here, as long as we
// are constantly accessing them the memory pages almost always stays in
// the physical memory (assuming we have large enough memory installed), and
// does not seem to create a memory bottleneck here.

inline void CaffeMallocHost(void** ptr, size_t size) {
#if defined(__ICC) || defined(__INTEL_COMPILER)
  *ptr = _mm_malloc(size, 2*1024*1024);
#else
  *ptr = aligned_alloc(2*1024*1024, size);
#endif
  CHECK(*ptr) << "host allocation of size " << size << " failed";
}

inline void CaffeFreeHost(void* ptr) {
#if defined(__ICC) || defined(__INTEL_COMPILER)
  _mm_free(ptr);
#else
  free(ptr);
#endif
}

typedef void (*sync_prv_to_cpu_func)(void* prv_ptr, void* cpu_ptr, void* prv_descriptor);


/**
 * @brief Manages memory allocation and synchronization between the host (CPU)
 *        and device (GPU).
 *
 * TODO(dox): more thorough description.
 */
class SyncedMemory {
 public:
  SyncedMemory()
      : prv_descriptor_(NULL), sync_prv_to_cpu_(NULL), cpu_ptr_(NULL), gpu_ptr_(NULL), prv_ptr_(NULL), size_(0), head_(UNINITIALIZED),
        own_cpu_data_(false), own_prv_data_(false) {}
  explicit SyncedMemory(size_t size)
      : prv_descriptor_(NULL), sync_prv_to_cpu_(NULL), cpu_ptr_(NULL), gpu_ptr_(NULL), prv_ptr_(NULL), size_(size), head_(UNINITIALIZED),
        own_cpu_data_(false), own_prv_data_(false) {}
  ~SyncedMemory();
  const void* cpu_data();
  void set_cpu_data(void* data);
  const void* gpu_data();
  void* mutable_cpu_data();
  void* mutable_gpu_data();

  void set_prv_data(void* data, bool same_data);
  void* init_prv_data(); // we use modified layout, data same as in cpu_ptr_
  const void* prv_data();
  void* mutable_prv_data();
  void* prv_descriptor_;  // TODO: cleanup? TODO: Consider non-void type
  sync_prv_to_cpu_func sync_prv_to_cpu_;

  enum SyncedHead { UNINITIALIZED, HEAD_AT_CPU, HEAD_AT_GPU, SYNCED, HEAD_AT_PRV, SYNCED_PRV};
  SyncedHead head() { return head_; }
  size_t size() { return size_; }

 private:
  void to_cpu();
  void to_gpu();
  void* cpu_ptr_;
  void* gpu_ptr_;
  void* prv_ptr_;
  size_t size_;
  SyncedHead head_;
  bool own_cpu_data_;
  bool own_prv_data_;
  DISABLE_COPY_AND_ASSIGN(SyncedMemory);
};  // class SyncedMemory

}  // namespace caffe

#endif  // CAFFE_SYNCEDMEM_HPP_
