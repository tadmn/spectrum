#pragma once

#include <choc/audio/choc_SampleBuffers.h>
#include <tb_Core.h>

#include "AnalyzerProcessor.h"
#include "State.h"

// Feeds a block of audio into the analyzers, one channel per analyzer. In summed mode the first
// analyzer gets the average of all channels instead, using mix_buffer as scratch space. Returns false
// if the block's channel count isn't supported by the analyzers.
inline bool analyzeAudio(choc::buffer::ChannelArrayView<float> in, State& state,
                         AnalyzerProcessors& analyzer_processors,
                         choc::buffer::ChannelArrayBuffer<float>& mix_buffer) {
    const auto num_channels = in.getNumChannels();
    if (num_channels == 0 || num_channels > analyzer_processors.size())
        return false;

    if (num_channels > 1 && state.channel_mode() == ChannelMode::Summed) {
        // Average the input channels into a single buffer to be processed
        tb_assert(mix_buffer.getNumFrames() >= in.getNumFrames());
        auto mix = mix_buffer.getStart(in.getNumFrames());
        copy(mix, in.getChannel(0));
        for (uint32_t channel = 1; channel < num_channels; ++channel)
            add(mix, in.getChannel(channel));

        applyGain(mix, 1.0f / static_cast<float>(num_channels));
        analyzer_processors.front()->processAudio(mix);
    } else {
        analyzer_processors.front()->processAudio(in.getChannelRange({ 0, 1 }));
    }

    // Fed in every mode so that they have current audio as soon as separate channels are turned on
    for (uint32_t channel = 1; channel < num_channels; ++channel)
        analyzer_processors[channel]->processAudio(in.getChannelRange({ channel, channel + 1 }));

    return true;
}
