#pragma once

#include <cstddef>
#include <cstdint>

namespace dsp::buffers {

// =================
// Stereo Buffer
// =================

struct StereoBuffer {
  float* buffer = nullptr;
  float* left = nullptr;
  float* right = nullptr;
};

void initStereoBuffer(StereoBuffer& buffer, size_t size);
StereoBuffer createStereoBufferSlice(const StereoBuffer& buffer, uint32_t offset);
void destroyStereoBuffer(StereoBuffer& buffer);

// Circular Stereo Buffer where size is always power of 2
struct StereoRingBuffer : StereoBuffer {
  size_t size = 0;
  size_t mask = 0;
};

void initStereoRingBuffer(StereoRingBuffer& cb, size_t requestedSize);
void destroyStereoRingBuffer(StereoRingBuffer& cb);

// ======================
// Stereo Buffer Pool
// ======================

struct StereoBufferView {
  float* left = nullptr;
  float* right = nullptr;
  size_t size = 0;
};

struct StereoBufferSlot {
  StereoBufferView view{};
  bool inUse = false;
};

struct StereoBufferPool {
  float* storage = nullptr;
  StereoBufferSlot* slots = nullptr;
  uint32_t numSlots = 0;
  uint32_t framesPerSlot = 0;
};

bool initStereoBufferPool(StereoBufferPool& pool, uint32_t numSlots, uint32_t framesPerSlot);
void destroyStereoBufferPool(StereoBufferPool& pool);

StereoBufferView getStereoBufferView(const StereoBufferPool& pool, uint32_t slotIndex);

int32_t acquireStereoBufferSlot(StereoBufferPool& pool);
void releaseStereoBufferSlot(StereoBufferPool& pool, uint32_t slotIndex);
void clearStereoBuffer(StereoBufferView view);

} // namespace dsp::buffers
