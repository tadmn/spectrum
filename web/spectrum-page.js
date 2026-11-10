"use strict";

// Glue between the page and the analyzer (spectrum.js / spectrum.wasm). Opens the microphone, feeds
// its audio into the analyzer, and handles full screen. Must load before spectrum.js.
(() => {
    const maxChannels = 2;

    const startScreen = document.getElementById("start");
    const startButton = document.getElementById("start-button");
    const startLabel = document.getElementById("start-label");
    const startError = document.getElementById("start-error");
    const picker = document.getElementById("input-picker");
    const deviceSelect = document.getElementById("input-device");

    let ready = false;
    let controlsHidden = false;
    let inputBuffers = [];
    let maxBlockFrames = 0;

    let context = null;
    let inputNode = null;
    let source = null;
    let stream = null;

    // The analyzer listens for mouse & keyboard events on the whole window. Keep the events meant for
    // the page's own controls from also reaching it. These are added first, so they run first.
    for (const type of ["mousedown", "wheel", "keydown", "keyup", "keypress"]) {
        window.addEventListener(type, (event) => {
            if (event.target instanceof Element && event.target.closest(".ui"))
                event.stopImmediatePropagation();
        }, { capture: true });
    }

    function unsupportedReason() {
        if (!window.isSecureContext)
            return "The microphone is only available when this page is served over HTTPS.";
        if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia)
            return "This browser doesn't support microphone input.";
        if (!window.AudioWorkletNode)
            return "This browser is too old to run the analyzer.";
        return null;
    }

    function errorMessage(error) {
        switch (error && error.name) {
            case "NotAllowedError":
            case "SecurityError":
                return "Microphone access was blocked. Allow it in your browser's site settings, then try again.";
            case "NotFoundError":
            case "OverconstrainedError":
                return "No microphone was found. Connect one and try again.";
            case "NotReadableError":
            case "AbortError":
                return "The microphone is in use by another app or couldn't be opened.";
            default:
                return "Couldn't start the microphone" + (error && error.message ? ": " + error.message : ".");
        }
    }

    function showStartScreen(error) {
        startError.textContent = error || "";
        startScreen.hidden = false;
        updatePicker();
        startButton.focus({ preventScroll: true });
    }

    // Audio ---------------------------------------------------------------------------------------

    // Copies a block of audio into the analyzer's input buffers and analyzes it
    function analyze(channels) {
        const numChannels = Math.min(channels.length, maxChannels);
        const numFrames = Math.min(channels[0].length, maxBlockFrames);

        // Looked up every time, as the heap gets replaced when memory grows
        const heap = Module.HEAPF32;
        for (let i = 0; i < numChannels; ++i)
            heap.set(channels[i].subarray(0, numFrames), inputBuffers[i] >> 2);

        Module._spectrumProcess(numChannels, numFrames);
    }

    function openInput(deviceId) {
        // The browser's voice processing would distort what gets analyzed
        const audio = {
            echoCancellation: false,
            noiseSuppression: false,
            autoGainControl: false,
            channelCount: { ideal: maxChannels },
        };

        if (deviceId)
            audio.deviceId = { exact: deviceId };

        return navigator.mediaDevices.getUserMedia({ audio });
    }

    function createContext(sampleRate) {
        if (context)
            context.close();

        context = new AudioContext(sampleRate ? { latencyHint: "interactive", sampleRate } : { latencyHint: "interactive" });
        inputNode = null;
        return context.resume();
    }

    async function createInputNode() {
        await context.audioWorklet.addModule("mic-worklet.js");

        inputNode = new AudioWorkletNode(context, "spectrum-input", {
            numberOfInputs: 1,
            numberOfOutputs: 1,
            outputChannelCount: [1],
            channelCount: maxChannels,
            channelCountMode: "clamped-max",
            channelInterpretation: "discrete",
        });

        inputNode.port.onmessage = (event) => analyze(event.data);

        // Browsers only run nodes that lead to an output. Nothing gets written to it, so it stays silent.
        inputNode.connect(context.destination);

        Module._spectrumSetSampleRate(context.sampleRate);
    }

    function closeInput() {
        if (source)
            source.disconnect();

        if (stream) {
            for (const track of stream.getTracks()) {
                track.removeEventListener("ended", onInputEnded);
                track.stop();
            }
        }

        source = null;
        stream = null;
    }

    async function connectInput(newStream) {
        if (!inputNode)
            await createInputNode();

        closeInput();
        stream = newStream;

        const track = stream.getAudioTracks()[0];
        track.addEventListener("ended", onInputEnded);

        try {
            source = context.createMediaStreamSource(stream);
        } catch (error) {
            if (error.name !== "NotSupportedError")
                throw error;

            // Some browsers can't resample microphones, so run at the microphone's rate instead
            await createContext(track.getSettings().sampleRate);
            await createInputNode();
            source = context.createMediaStreamSource(stream);
        }

        source.connect(inputNode);
    }

    function onInputEnded() {
        closeInput();
        showStartScreen("The audio input was disconnected.");
    }

    async function start() {
        startButton.disabled = true;
        startLabel.textContent = "Starting…";
        startError.textContent = "";

        try {
            // Started within the click, as browsers only let audio start from one
            const resumed = context ? context.resume() : createContext();

            // Falls back to the default input if the one picked earlier is gone
            const newStream = await openInput(deviceSelect.value).catch((error) => {
                if (deviceSelect.value && (error.name === "OverconstrainedError" || error.name === "NotFoundError"))
                    return openInput(null);
                throw error;
            });

            await resumed;
            await connectInput(newStream);

            startScreen.hidden = true;
            updatePicker();
            wakePicker();
            await updateDeviceList();
        } catch (error) {
            console.error(error);
            showStartScreen(errorMessage(error));
        } finally {
            startButton.disabled = false;
            startLabel.textContent = "Start microphone";
        }
    }

    async function switchInput(deviceId) {
        try {
            await connectInput(await openInput(deviceId));
        } catch (error) {
            console.error(error);
            closeInput();
            showStartScreen(errorMessage(error));
        }

        await updateDeviceList();
    }

    // Input picker --------------------------------------------------------------------------------

    async function updateDeviceList() {
        const devices = (await navigator.mediaDevices.enumerateDevices()).filter((d) => d.kind === "audioinput");
        const track = stream && stream.getAudioTracks()[0];
        const current = track ? track.getSettings().deviceId : deviceSelect.value;

        deviceSelect.replaceChildren(...devices.map((device, i) =>
            new Option(device.label || `Microphone ${i + 1}`, device.deviceId, false, device.deviceId === current)));
    }

    // Only shows while listening, and hides with the analyzer's controls
    function updatePicker() {
        picker.hidden = !startScreen.hidden || controlsHidden;
    }

    let idleTimer = 0;

    function wakePicker() {
        picker.classList.remove("idle");
        clearTimeout(idleTimer);
        idleTimer = setTimeout(() => picker.classList.add("idle"), 2500);
    }

    deviceSelect.addEventListener("change", () => switchInput(deviceSelect.value));
    window.addEventListener("mousemove", wakePicker, { passive: true });
    window.addEventListener("touchstart", wakePicker, { passive: true });

    if (navigator.mediaDevices) {
        navigator.mediaDevices.addEventListener("devicechange", () => {
            if (stream)
                updateDeviceList();
        });
    }

    // Full screen ---------------------------------------------------------------------------------

    function fullScreenElement() {
        return document.fullscreenElement || document.webkitFullscreenElement || null;
    }

    function toggleFullScreen() {
        const page = document.documentElement;

        if (fullScreenElement()) {
            const exit = document.exitFullscreen || document.webkitExitFullscreen;
            if (exit)
                Promise.resolve(exit.call(document)).catch(() => {});
        } else {
            const request = page.requestFullscreen || page.webkitRequestFullscreen;
            if (request)
                Promise.resolve(request.call(page)).catch(() => {});
        }
    }

    for (const type of ["fullscreenchange", "webkitfullscreenchange"]) {
        document.addEventListener(type, () => {
            if (ready)
                Module._spectrumSetFullScreen(fullScreenElement() ? 1 : 0);
        });
    }

    // Analyzer module -----------------------------------------------------------------------------

    window.Module = {
        canvas: document.getElementById("canvas"),

        // Called by the analyzer once it's up and running
        spectrumReady() {
            ready = true;
            maxBlockFrames = Module._spectrumMaxBlockFrames();
            for (let i = 0; i < maxChannels; ++i)
                inputBuffers.push(Module._spectrumInputBuffer(i));

            const reason = unsupportedReason();
            if (reason) {
                startLabel.textContent = "Start microphone";
                showStartScreen(reason);
                return;
            }

            startButton.disabled = false;
            startLabel.textContent = "Start microphone";
            startButton.focus({ preventScroll: true });
        },

        // Called by the analyzer's full screen button
        spectrumToggleFullScreen: toggleFullScreen,

        // Called by the analyzer whenever its controls are hidden or shown
        spectrumSetControlsHidden(hidden) {
            controlsHidden = hidden;
            updatePicker();
        },

        onAbort() {
            startButton.disabled = true;
            startLabel.textContent = "Unavailable";
            showStartScreen("The analyzer couldn't start. It needs a browser with WebGL 2 and WebAssembly support.");
        },
    };

    startButton.addEventListener("click", start);
})();
