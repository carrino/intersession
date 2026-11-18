// Sound.ino: the microphone's loudness and its spectrum.
//
// What it adds: setupMic() starts the microphone, micDecibels() is the loudness since it
// was last asked, updateSpectrum() turns each block of samples into the loudness of 13
// bands from bass to treble, and sendSpectrum() publishes them for the page's dot grid.
//
// Why: the mic is not on the I2C bus like the other sensors. It sends a stream of single
// bits (PDM) that the chip's own hardware turns into 16,000 samples a second and hands
// over in blocks of 256, every 16 ms, through the onMicData() callback. Loudness is the
// RMS of those samples. The spectrum is an FFT of each block: 128 bins, each 62.5 Hz wide,
// up to 8 kHz, added up into bands spaced like a piano's octaves.

// The PDM driver's gain register is 0.5 dB per step with 40 as 0 dB. The default of 20
// (-10 dB) leaves a quiet room below the converter's smallest step, so everything quiet
// reads as the same hiss; +10 dB lifts it clear while a loud clap still fits.
const int MIC_GAIN = 60;

// Turns the mic level relative to full scale into an approximate sound pressure level:
// the mic puts out -26 dBFS at 94 dB SPL, and the +10 dB of gain comes off again.
const float MIC_DB_OFFSET = 94 + 26 - 10;

// Each band is sent as a byte: 0 at SPECTRUM_FLOOR_DB, 255 at SPECTRUM_TOP_DB, in the mic
// reading's rough dB SPL. SPECTRUM_BANDS is in day1.ino, next to the readings.
const int MIC_RATE = 16000;
const int FFT_SIZE = 256;  // one block from the mic
const float SPECTRUM_LOW_HZ = 100, SPECTRUM_HIGH_HZ = 8000;
const float SPECTRUM_FLOOR_DB = 20, SPECTRUM_TOP_DB = 80;

// Running sums of the microphone samples since last checked, kept by the PDM callback.
volatile int64_t micSum = 0, micSumSquares = 0;
volatile int32_t micCount = 0;
int16_t micSamples[FFT_SIZE];

// A full block of samples handed from the PDM callback to loop() for the spectrum. The
// callback fills it only after loop() has taken the last one.
volatile int16_t fftBlock[FFT_SIZE];
volatile bool fftBlockReady = false;
arm_rfft_fast_instance_f32 fft;
float hannWindow[FFT_SIZE];
int bandEdges[SPECTRUM_BANDS + 1];      // FFT bin where each band starts; the last is the end
uint8_t spectrumPeaks[SPECTRUM_BANDS];  // the loudest each band got since the last send

void setupMic() {
  setupSpectrum();
  PDM.onReceive(onMicData);
  if (!PDM.begin(1, MIC_RATE)) {
    warn("Mic failed to start");
  }
  PDM.setGain(MIC_GAIN);
}

// The RMS level of the microphone samples since the last call, in decibels.
float micDecibels() {
  noInterrupts();  // so the PDM callback cannot change the sums while we take them
  int64_t sum = micSum, sumSquares = micSumSquares;
  int32_t count = micCount;
  micSum = micSumSquares = micCount = 0;
  interrupts();
  if (count == 0) {
    return 0;
  }
  float mean = (float)sum / count;
  float rms = sqrt(max((float)sumSquares / count - mean * mean, 1.0f));  // 1 keeps log10 finite in silence
  return 20 * log10(rms / 32768) + MIC_DB_OFFSET;
}

// Called by the PDM library whenever a chunk of microphone samples is ready.
void onMicData() {
  int count = PDM.read(micSamples, sizeof(micSamples)) / sizeof(micSamples[0]);
  for (int i = 0; i < count; i++) {
    micSum += micSamples[i];
    micSumSquares += (int32_t)micSamples[i] * micSamples[i];
  }
  micCount += count;
  if (count == FFT_SIZE && !fftBlockReady) {
    for (int i = 0; i < FFT_SIZE; i++) {
      fftBlock[i] = micSamples[i];
    }
    fftBlockReady = true;
  }
}

// ---- Sound spectrum ----

// The window that fades each block in and out, so a note shows in its own band instead of
// smearing across all of them, and where each band's bins start.
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
}

// Turns the latest block into the loudness of each band, keeping the loudest each reaches
// until it is sent. Skipped while nobody is watching, since an FFT is real work.
void updateSpectrum() {
  static float samples[FFT_SIZE], bins[FFT_SIZE], power[FFT_SIZE / 2];
  if (!fftBlockReady) {
    return;
  }
  if (!spectrumChar.notifyEnabled()) {
    fftBlockReady = false;
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
    // Roughly the band's RMS in sample units: the FFT and the window (which adds up to
    // about FFT_SIZE / 2) each scale the bins up, so this undoes both.
    float rms = sqrtf(2 * sum) / (FFT_SIZE / 2);
    float dB = 20 * log10f(rms / 32768 + 1e-9f) + MIC_DB_OFFSET;
    float level = constrain((dB - SPECTRUM_FLOOR_DB) * 255 / (SPECTRUM_TOP_DB - SPECTRUM_FLOOR_DB), 0, 255);
    spectrumPeaks[b] = max(spectrumPeaks[b], (uint8_t)level);
  }
}

// Sends the loudest each band got since the last send, about 30 times a second, so a
// short sound between sends still shows.
void sendSpectrum() {
  memcpy(readings.spectrum, spectrumPeaks, sizeof(readings.spectrum));
  memset(spectrumPeaks, 0, sizeof(spectrumPeaks));
  publish(spectrumChar);
}
