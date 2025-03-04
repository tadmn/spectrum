
#pragma once

#include <choc_SampleBuffers.h>

#include <sys/types.h>

template<typename T>
class FifoBuffer
{
public:
    FifoBuffer(uint numChannels, uint numFrames)
        : mBuffer(numChannels, numFrames)
    {
        clear();
    }

    uint freeSpace() const noexcept { return mBuffer.getNumFrames() - mSize; }
    bool isFull() const noexcept { return freeSpace() == 0; }

    uint capacity() const noexcept { return mBuffer.getNumFrames(); }
    choc::buffer::ChannelArrayView<T> getBuffer() const noexcept { return mBuffer.getStart(mSize); }

    choc::buffer::ChannelArrayView<T> push(choc::buffer::ChannelArrayView<T> const & buffer)
    {
        assert(buffer.getNumChannels() == mBuffer.getNumChannels());

        auto const framesToWrite = std::min(freeSpace(), buffer.getNumFrames());
        choc::buffer::copyIntersection(mBuffer.fromFrame(mSize), buffer.getStart(framesToWrite));
        mSize += framesToWrite;
        return buffer.fromFrame(framesToWrite);
    }

    void pop(uint numFramesToPop)
    {
        auto const framesToPop = std::min(numFramesToPop, mSize);
        if (framesToPop <= 0)
            return;

        // Shift remaining data to the front of the buffer. Clearing is just for extra safety.
        choc::buffer::copyIntersectionAndClearOutside(mBuffer, mBuffer.fromFrame(numFramesToPop));

        mSize = std::max(mSize - framesToPop, 0u);
    }

    void clear()
    {
        mBuffer.clear(); // Just for safety.
        mSize = 0;
    }

private:
    choc::buffer::ChannelArrayBuffer<T> mBuffer;
    uint mSize = 0;
};
