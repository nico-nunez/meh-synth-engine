#include "dsp/Buffers.h"
#include "dsp/Math.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>

namespace dsp::buffers {

// =================
// Stereo Buffer
// =================

void initStereoBuffer(StereoBuffer& buffer, size_t size) {
  buffer.buffer = new float[size * 2]();
  buffer.left = buffer.buffer;
  buffer.right = buffer.left + size;
}

StereoBuffer createStereoBufferSlice(const StereoBuffer& buffer, uint32_t offset) {
  return {nullptr, buffer.left + offset, buffer.right + offset};
}

void destroyStereoBuffer(StereoBuffer& buffer) {
  delete[] buffer.buffer;
  buffer.buffer = nullptr;
  buffer.left = nullptr;
  buffer.right = nullptr;
}

// Circular StereoBuffer
void initStereoRingBuffer(StereoRingBuffer& cb, size_t requestedSize) {
  cb.size = dsp::math::nextPow2(requestedSize);
  cb.mask = cb.size - 1;
  initStereoBuffer(cb, cb.size);
}

void destroyStereoRingBuffer(StereoRingBuffer& cb) {
  destroyStereoBuffer(cb);
  cb.size = 0;
  cb.mask = 0;
}

// ======================
// Stereo Buffer Pool
// ======================

bool initStereoBufferPool(StereoBufferPool& pool, uint32_t numSlots, uint32_t framesPerSlot) {
  if (numSlots == 0 || framesPerSlot == 0)
    return false;

  uint32_t bufferSize = framesPerSlot * 2;

  float* storage = new (std::nothrow) float[numSlots * bufferSize]();
  if (!storage)
    return false;

  StereoBufferSlot* slots = new (std::nothrow) StereoBufferSlot[numSlots]{};
  if (!slots) {
    delete[] storage;
    return false;
  }

  pool.storage = storage;
  pool.slots = slots;
  pool.numSlots = numSlots;
  pool.framesPerSlot = framesPerSlot;

  for (uint32_t i = 0; i < numSlots; ++i) {
    float* slotBase = storage + (i * bufferSize);

    pool.slots[i].view.left = slotBase;
    pool.slots[i].view.right = slotBase + framesPerSlot;
    pool.slots[i].view.size = framesPerSlot;

    pool.slots[i].inUse = false;

    clearStereoBuffer(pool.slots->view); // zero init
  }

  return true;
}

void destroyStereoBufferPool(StereoBufferPool& pool) {
  delete[] pool.slots;
  delete[] pool.storage;

  pool.storage = nullptr;
  pool.slots = nullptr;
  pool.numSlots = 0;
  pool.framesPerSlot = 0;
}

StereoBufferView getStereoBufferView(const StereoBufferPool& pool, uint32_t slotIndex) {
  return pool.slots[slotIndex].view;
}

int32_t acquireStereoBufferSlot(StereoBufferPool& pool) {
  for (uint32_t i = 0; i < pool.numSlots; ++i) {
    if (pool.slots[i].inUse)
      continue;

    clearStereoBuffer(pool.slots[i].view);
    pool.slots[i].inUse = true;

    return static_cast<int32_t>(i);
  }

  return -1;
}

void releaseStereoBufferSlot(StereoBufferPool& pool, uint32_t slotIndex) {
  pool.slots[slotIndex].inUse = false;
}

void clearStereoBuffer(StereoBufferView view) {
  std::fill_n(view.left, view.size, 0.0f);
  std::fill_n(view.right, view.size, 0.0f);
}
} // namespace dsp::buffers
