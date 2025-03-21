
#pragma once

#include <choc_SampleBuffers.h>

template<typename T>
class FifoBuffer {
  public:
    FifoBuffer(int numChannels, int numFrames) : mBuffer(numChannels, numFrames) { clear(); }

    int size() const noexcept { return mSize; }
    int freeSpace() const noexcept { return mBuffer.getNumFrames() - mSize; }
    bool isFull() const noexcept { return freeSpace() == 0; }
    int capacity() const noexcept { return mBuffer.getNumFrames(); }

    choc::buffer::ChannelArrayView<T> getBuffer() const noexcept { return mBuffer.getStart(mSize); }

    choc::buffer::ChannelArrayView<T> push(choc::buffer::ChannelArrayView<T> const& buffer) {
        assert(buffer.getNumChannels() == mBuffer.getNumChannels());

        const auto framesToWrite = std::min(freeSpace(), static_cast<int>(buffer.getNumFrames()));
        choc::buffer::copyIntersection(mBuffer.fromFrame(mSize), buffer.getStart(framesToWrite));
        mSize += framesToWrite;
        return buffer.fromFrame(framesToWrite);
    }

    void pop(int numFramesToPop) {
        const auto framesToPop = std::min(numFramesToPop, mSize);
        if (framesToPop <= 0)
            return;

        // Shift remaining data to the front of the buffer. Clearing is just for extra safety
        choc::buffer::copyIntersectionAndClearOutside(mBuffer, mBuffer.fromFrame(numFramesToPop));

        mSize = std::max(mSize - framesToPop, 0);
    }

    void clear() {
        mBuffer.clear();  // Just for safety
        mSize = 0;
    }

  private:
    choc::buffer::ChannelArrayBuffer<T> mBuffer;
    int mSize = 0;
};
