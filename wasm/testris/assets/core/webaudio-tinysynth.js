// STUB — bukan webaudio-tinysynth.js asli, cuma mock biar GameAudio gak crash
// pas dites tanpa library aslinya. Ganti dengan file gzip asli kamu nanti.
class WebAudioTinySynth {
    constructor() {
        this.actx = { state: "suspended", resume: () => { this.actx.state = "running"; } };
        console.log("[STUB] WebAudioTinySynth constructed (bukan library asli)");
    }
    setMasterVol(v) { console.log(`[STUB] setMasterVol(${v})`); }
    loadMIDI(bytes) { console.log(`[STUB] loadMIDI(${bytes.byteLength} bytes)`); }
    setLoop(n) { console.log(`[STUB] setLoop(${n})`); }
    playMIDI() { console.log("[STUB] playMIDI() — no real audio, ini cuma mock"); }
    stopMIDI() { console.log("[STUB] stopMIDI()"); }
}
