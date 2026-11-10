// Runs on the audio rendering thread and hands each block of input audio to the page as soon as it
// arrives, so the analyzer always has the latest audio when it draws.
class SpectrumInput extends AudioWorkletProcessor {
    process(inputs) {
        const input = inputs[0];

        // Has no channels while nothing is connected
        if (input && input.length > 0) {
            const channels = input.map((channel) => channel.slice());
            this.port.postMessage(channels, channels.map((channel) => channel.buffer));
        }

        // Leaves the output silent
        return true;
    }
}

registerProcessor("spectrum-input", SpectrumInput);
