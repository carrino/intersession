// Sound.ino: the microphone's loudness and its spectrum.
//
// What it adds: setupMic() starts the microphone, updateSpectrum() turns each block of
// samples into the loudness of 13 bands from bass to treble, sendSpectrum() publishes
// them for the page's dot grid, and micDecibels() is the overall loudness since it was
// last asked, in dBA like a sound level meter. The bands also feed the beat finder in
// Beat.ino.
//
// Why: the mic is not on the I2C bus like the other sensors. It sends a stream of single
// bits (PDM) that the chip's own hardware turns into 16,000 samples a second and hands
// over in blocks of 256, every 16 ms, through the onMicData() callback. The spectrum is
// an FFT of each block: 128 bins, each 62.5 Hz wide, up to 8 kHz, added up into bands
// spaced like a piano's octaves.
//
// The overall loudness is the bands added back up, each counted the way an ear hears it:
// ears are most sensitive around 2 to 4 kHz and nearly deaf to deep rumble, and sound
// meters copy that with a standard curve called A-weighting. Without it a quiet room read
// 51 dB: almost all of that was below 100 Hz, where the air handling, the building and
// the mic's own drift live, and none of it can be heard.

// The PDM driver's gain register is 0.5 dB per step with 40 as 0 dB. The default of 20
// (-10 dB) leaves a quiet room below the converter's smallest step, so everything quiet
// reads as the same hiss; +10 dB lifts it clear while a loud clap still fits.
const int MIC_GAIN = 60;

// Turns a level relative to full scale into an approximate sound pressure level: the mic
// puts out -26 dBFS at 94 dB SPL, and the +10 dB of gain comes off again.
const float MIC_DB_OFFSET = 94 + 26 - 10;

// Each band is sent as a byte: 0 at SPECTRUM_FLOOR_DB, 255 at SPECTRUM_TOP_DB, in rough
// dB SPL. SPECTRUM_BANDS is in day1.ino, next to the readings.
const int MIC_RATE = 16000;
const int FFT_SIZE = 256;  // one block from the mic
const float SPECTRUM_LOW_HZ = 100, SPECTRUM_HIGH_HZ = 8000;
const float SPECTRUM_FLOOR_DB = 20, SPECTRUM_TOP_DB = 80;

int16_t micSamples[FFT_SIZE];

// A full block of samples handed from the PDM callback to loop() for the spectrum. The
// callback fills it only after loop() has taken the last one.
volatile int16_t fftBlock[FFT_SIZE];
volatile bool fftBlockReady = false;
arm_rfft_fast_instance_f32 fft;
float hannWindow[FFT_SIZE];
int bandEdges[SPECTRUM_BANDS + 1];      // FFT bin where each band starts; the last is the end
uint8_t spectrumPeaks[SPECTRUM_BANDS];  // the loudest each band got since the last send
float earWeights[SPECTRUM_BANDS];       // how much of each band's power counts toward the loudness
float loudnessSum = 0;                  // the weighted power of each block, added up since micDecibels()
int loudnessBlocks = 0;                 // was last asked, and how many blocks that was

void setupMic() {
  setupSpectrum();
  PDM.onReceive(onMicData);
  if (!PDM.begin(1, MIC_RATE)) {
    warn("Mic failed to start");
  }
  PDM.setGain(MIC_GAIN);
}

// The loudness since the last call, in dBA: the average over the blocks in between.
float micDecibels() {
  static float decibels = 0;
  if (loudnessBlocks > 0) {
    decibels = toDecibels(loudnessSum / loudnessBlocks);
    loudnessSum = 0;
    loudnessBlocks = 0;
  }
  return decibels;
}

// Sound power, as the average of the samples squared, in decibels of sound pressure.
float toDecibels(float power) {
  return 10 * log10f(power / (32768.0f * 32768.0f) + 1e-12f) + MIC_DB_OFFSET;  // the 1e-12 keeps log10 finite in silence
}

// Called by the PDM library whenever a chunk of microphone samples is ready. Hands a full
// block to loop() for the spectrum, unless loop() has not taken the last one yet.
void onMicData() {
  int count = PDM.read(micSamples, sizeof(micSamples)) / sizeof(micSamples[0]);
  if (count == FFT_SIZE && !fftBlockReady) {
    for (int i = 0; i < FFT_SIZE; i++) {
      fftBlock[i] = micSamples[i];
    }
    fftBlockReady = true;
  }
}

// ---- Sound spectrum ----

// The window that fades each block in and out, so a note shows in its own band instead of
// smearing across all of them; where each band's bins start; and each band's ear weight.
void setupSpectrum() {
  arm_rfft_fast_init_f32(&fft, FFT_SIZE);
  for (int i = 0; i < FFT_SIZE; i++) {
    hannWindow[i] = 0.5f * (1 - cosf(2 * PI * i / (FFT_SIZE - 1)));
  }
  float binHz = (float)MIC_RATE / FFT_SIZE;
  float ratio = powf(SPECTRUM_HIGH_HZ / SPECTRUM_LOW_HZ, 1.0f / SPECTRUM_BANDS);  // each band this much higher
  bandEdges[0] = lroundf(SPECTRUM_LOW_HZ / binHz);
  for (int b = 1; b <= SPECTRUM_BANDS; b++) {
    int edge = lroundf(SPECTRUM_LOW_HZ * powf(ratio, b) / binHz);
    bandEdges[b] = min(max(edge, bandEdges[b - 1] + 1), FFT_SIZE / 2);  // at least one bin each
  }
  for (int b = 0; b < SPECTRUM_BANDS; b++) {
    earWeights[b] = earWeight(SPECTRUM_LOW_HZ * powf(ratio, b + 0.5f));  // at the middle of the band
  }
}

// The A-weighting curve from the sound level meter standard (IEC 61672), as a share of
// the power: 1 at 1 kHz, about 0.01 at 100 Hz, a little over 1 from 2 to 4 kHz.
float earWeight(float hz) {
  float f2 = hz * hz;
  float gain = 1.2589f * 12194 * 12194 * f2 * f2 /
               ((f2 + 20.6f * 20.6f) * sqrtf((f2 + 107.7f * 107.7f) * (f2 + 737.9f * 737.9f)) * (f2 + 12194.0f * 12194));
  return gain * gain;
}

// Turns the latest block, if there is a new one, into the loudness of each band. The bands
// go to the beat finder in Beat.ino, and the loudest each reaches is kept until it is sent.
void updateSpectrum() {
  static float samples[FFT_SIZE], bins[FFT_SIZE], power[FFT_SIZE / 2];
  float bandDb[SPECTRUM_BANDS], loudness = 0;
  if (!fftBlockReady) {
    return;
  }
  for (int i = 0; i < FFT_SIZE; i++) {
    samples[i] = fftBlock[i] * hannWindow[i];
  }
  fftBlockReady = false;  // the callback may fill the block again now
  arm_rfft_fast_f32(&fft, samples, bins, 0);
  arm_cmplx_mag_squared_f32(bins, power, FFT_SIZE / 2);
  for (int b = 0; b < SPECTRUM_BANDS; b++) {
    float sum = 0;
    for (int k = bandEdges[b]; k < bandEdges[b + 1]; k++) {
      sum += power[k];
    }
    // The band's power in sample units. The FFT and the window (which adds up to about
    // FFT_SIZE / 2) each scale the bins up, and the window also spreads every sound over
    // 1.5 bins' worth, so this undoes all three.
    float bandPower = 2 * sum / 1.5f / (FFT_SIZE / 2 * FFT_SIZE / 2);
    loudness += bandPower * earWeights[b];
    bandDb[b] = toDecibels(bandPower);
    float level = constrain((bandDb[b] - SPECTRUM_FLOOR_DB) * 255 / (SPECTRUM_TOP_DB - SPECTRUM_FLOOR_DB), 0, 255);
    spectrumPeaks[b] = max(spectrumPeaks[b], (uint8_t)level);
  }
  loudnessSum += loudness;
  loudnessBlocks++;
  updateBeat(bandDb);
}

// Sends the loudest each band got since the last send, about 30 times a second, so a
// short sound between sends still shows.
void sendSpectrum() {
  memcpy(readings.spectrum, spectrumPeaks, sizeof(readings.spectrum));
  memset(spectrumPeaks, 0, sizeof(spectrumPeaks));
  publish(spectrumChar);
}
