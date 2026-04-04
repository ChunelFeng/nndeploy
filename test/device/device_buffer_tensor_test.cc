#include "nndeploy/device/buffer.h"
#include "nndeploy/device/device.h"
#include "nndeploy/device/tensor.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <unordered_set>

namespace {

class CountingDevice : public nndeploy::device::Device {
 public:
  CountingDevice()
      : Device(nndeploy::base::DeviceType(nndeploy::base::kDeviceTypeCodeCpu,
                                          0)) {}

  nndeploy::device::BufferDesc toBufferDesc(
      const nndeploy::device::TensorDesc &desc,
      const nndeploy::base::IntVector &config) override {
    size_t size = desc.data_type_.size();
    if (desc.stride_.empty()) {
      for (int dim : desc.shape_) {
        size *= dim;
      }
    } else {
      size = desc.stride_[0] * desc.shape_[0];
    }
    return nndeploy::device::BufferDesc(size, config);
  }

  void *allocate(size_t size) override {
    void *ptr = std::malloc(size == 0 ? 1 : size);
    EXPECT_NE(ptr, nullptr);
    if (ptr != nullptr) {
      ++allocate_count_;
      live_allocations_.insert(ptr);
    }
    return ptr;
  }

  void *allocate(const nndeploy::device::BufferDesc &desc) override {
    return allocate(desc.getRealSize());
  }

  void deallocate(void *ptr) override {
    if (ptr == nullptr) {
      return;
    }
    auto iter = live_allocations_.find(ptr);
    EXPECT_NE(iter, live_allocations_.end());
    if (iter != live_allocations_.end()) {
      live_allocations_.erase(iter);
    }
    ++deallocate_count_;
    std::free(ptr);
  }

  nndeploy::base::Status copy(void *src, void *dst, size_t size,
                              nndeploy::device::Stream *stream =
                                  nullptr) override {
    if (src == nullptr || dst == nullptr) {
      return nndeploy::base::kStatusCodeErrorNullParam;
    }
    std::memcpy(dst, src, size);
    return nndeploy::base::kStatusCodeOk;
  }

  nndeploy::base::Status download(void *src, void *dst, size_t size,
                                  nndeploy::device::Stream *stream =
                                      nullptr) override {
    return copy(src, dst, size, stream);
  }

  nndeploy::base::Status upload(void *src, void *dst, size_t size,
                                nndeploy::device::Stream *stream =
                                    nullptr) override {
    return copy(src, dst, size, stream);
  }

  nndeploy::base::Status copy(nndeploy::device::Buffer *src,
                              nndeploy::device::Buffer *dst,
                              nndeploy::device::Stream *stream =
                                  nullptr) override {
    return copy(src->getData(), dst->getData(),
                std::min(src->getRealSize(), dst->getRealSize()), stream);
  }

  nndeploy::base::Status download(nndeploy::device::Buffer *src,
                                  nndeploy::device::Buffer *dst,
                                  nndeploy::device::Stream *stream =
                                      nullptr) override {
    return copy(src, dst, stream);
  }

  nndeploy::base::Status upload(nndeploy::device::Buffer *src,
                                nndeploy::device::Buffer *dst,
                                nndeploy::device::Stream *stream =
                                    nullptr) override {
    return copy(src, dst, stream);
  }

  nndeploy::base::Status init() override {
    return nndeploy::base::kStatusCodeOk;
  }

  nndeploy::base::Status deinit() override {
    return nndeploy::base::kStatusCodeOk;
  }

  int allocateCount() const { return allocate_count_; }
  int deallocateCount() const { return deallocate_count_; }
  size_t liveAllocationCount() const { return live_allocations_.size(); }

 private:
  int allocate_count_ = 0;
  int deallocate_count_ = 0;
  std::unordered_set<void *> live_allocations_;
};

class BufferTensorLifetimeTest : public testing::Test {
 protected:
  CountingDevice device_;
};

TEST_F(BufferTensorLifetimeTest, BufferCopyAssignmentReleasesOldStorage) {
  {
    nndeploy::device::Buffer lhs(&device_, 16);
    nndeploy::device::Buffer rhs(&device_, 32);

    EXPECT_EQ(device_.allocateCount(), 2);
    EXPECT_EQ(device_.deallocateCount(), 0);

    lhs = rhs;

    EXPECT_EQ(device_.deallocateCount(), 1);
    EXPECT_EQ(device_.liveAllocationCount(), 1U);
    EXPECT_EQ(lhs.getData(), rhs.getData());
  }

  EXPECT_EQ(device_.allocateCount(), 2);
  EXPECT_EQ(device_.deallocateCount(), 2);
  EXPECT_EQ(device_.liveAllocationCount(), 0U);
}

TEST_F(BufferTensorLifetimeTest, BufferMoveAssignmentReleasesOldStorage) {
  void *moved_data = nullptr;

  {
    nndeploy::device::Buffer lhs(&device_, 16);
    nndeploy::device::Buffer rhs(&device_, 32);
    moved_data = rhs.getData();

    lhs = std::move(rhs);

    EXPECT_EQ(device_.deallocateCount(), 1);
    EXPECT_EQ(device_.liveAllocationCount(), 1U);
    EXPECT_EQ(lhs.getData(), moved_data);
    EXPECT_EQ(rhs.getData(), nullptr);
  }

  EXPECT_EQ(device_.allocateCount(), 2);
  EXPECT_EQ(device_.deallocateCount(), 2);
  EXPECT_EQ(device_.liveAllocationCount(), 0U);
}

TEST_F(BufferTensorLifetimeTest, BufferSharedCopiesReleaseStorageOnce) {
  {
    nndeploy::device::Buffer original(&device_, 64);
    {
      nndeploy::device::Buffer copy = original;
      EXPECT_EQ(copy.getData(), original.getData());
      EXPECT_EQ(device_.deallocateCount(), 0);
    }
    EXPECT_EQ(device_.deallocateCount(), 0);
  }

  EXPECT_EQ(device_.allocateCount(), 1);
  EXPECT_EQ(device_.deallocateCount(), 1);
  EXPECT_EQ(device_.liveAllocationCount(), 0U);
}

TEST_F(BufferTensorLifetimeTest, TensorCopyAssignmentReleasesOldStorage) {
  nndeploy::device::TensorDesc desc(nndeploy::base::dataTypeOf<float>(),
                                    nndeploy::base::kDataFormatN,
                                    {4});

  {
    nndeploy::device::Tensor lhs(&device_, desc, "lhs");
    nndeploy::device::Tensor rhs(&device_, desc, "rhs");

    EXPECT_EQ(device_.allocateCount(), 2);
    EXPECT_EQ(device_.deallocateCount(), 0);

    lhs = rhs;

    EXPECT_EQ(device_.deallocateCount(), 1);
    EXPECT_EQ(device_.liveAllocationCount(), 1U);
    ASSERT_NE(lhs.getBuffer(), nullptr);
    ASSERT_NE(rhs.getBuffer(), nullptr);
    EXPECT_EQ(lhs.getBuffer()->getData(), rhs.getBuffer()->getData());
  }

  EXPECT_EQ(device_.allocateCount(), 2);
  EXPECT_EQ(device_.deallocateCount(), 2);
  EXPECT_EQ(device_.liveAllocationCount(), 0U);
}

TEST_F(BufferTensorLifetimeTest, TensorMoveAssignmentReleasesOldStorage) {
  nndeploy::device::TensorDesc desc(nndeploy::base::dataTypeOf<float>(),
                                    nndeploy::base::kDataFormatN,
                                    {4});
  void *moved_data = nullptr;

  {
    nndeploy::device::Tensor lhs(&device_, desc, "lhs");
    nndeploy::device::Tensor rhs(&device_, desc, "rhs");
    moved_data = rhs.getBuffer()->getData();

    lhs = std::move(rhs);

    EXPECT_EQ(device_.deallocateCount(), 1);
    EXPECT_EQ(device_.liveAllocationCount(), 1U);
    ASSERT_NE(lhs.getBuffer(), nullptr);
    EXPECT_EQ(lhs.getBuffer()->getData(), moved_data);
    EXPECT_EQ(rhs.getBuffer(), nullptr);
  }

  EXPECT_EQ(device_.allocateCount(), 2);
  EXPECT_EQ(device_.deallocateCount(), 2);
  EXPECT_EQ(device_.liveAllocationCount(), 0U);
}

TEST_F(BufferTensorLifetimeTest, TensorSharedDeallocateReleasesStorageOnce) {
  nndeploy::device::TensorDesc desc(nndeploy::base::dataTypeOf<float>(),
                                    nndeploy::base::kDataFormatN,
                                    {8});

  nndeploy::device::Tensor original(&device_, desc, "origin");
  nndeploy::device::Tensor copy = original;

  original.deallocate();
  EXPECT_EQ(device_.deallocateCount(), 0);
  EXPECT_EQ(device_.liveAllocationCount(), 1U);

  copy.deallocate();
  EXPECT_EQ(device_.deallocateCount(), 1);
  EXPECT_EQ(device_.liveAllocationCount(), 0U);
}

}  // namespace
