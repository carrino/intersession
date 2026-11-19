// Beat.ino: finds the beat in music and flashes the back NeoPixel on it.
//
// What it adds: updateBeat() takes the loudness of each band from Sound.ino, 62.5 times a
// second, and works out whether there is a steady beat, how fast it is, and when the next
// one lands. On each beat it flashes the NeoPixel on the back in a new color and publishes
// readings.beat: the tempo in beats per minute (0 when there is no steady beat) and how
// many beats it has counted.
//
// Why it takes three steps:
//   1. Onsets. A drum hit makes several bands jump at once, so add up how much louder each
//      band got since the block before. That number spikes on every hit.
//   2. Tempo. Twice a second, slide the last six seconds of spikes against themselves. The
//      shift at which they line up best is the time from one beat to the next.
//   3. Phase. A tempo alone does not say where the beats are. Lay a comb with teeth one
//      beat apart over the spikes and slide it until the teeth sit on the most of them.
//      A metronome running at the tempo is nudged toward that spot, and it is the
//      metronome that flashes the light. So the flash lands on the beat, not after it.
// The flashing stops a second or two after the last drum hit.
// Music often fits half or double its tempo just as well, so the light may flash on every
// other beat of a fast song, or twice a beat on a slow one.
//
// What to expect: it is at home with dance music and pop, where the drums are loud and
// steady. On music with soft or loose drums it mostly stays dark. That is on purpose: the
// bar to start flashing is set high, because flashing at talk and clatter is worse than
// missing a quiet song. Doing better takes a sharper way to spot onsets and a longer look
// back, which would make a good project.

const float BEAT_FRAME_HZ = 62.5;      // blocks a second from the mic: 16000 samples in blocks of 256
const int BEAT_HISTORY = 384;          // blocks of onsets kept: about six seconds
const float BEAT_MIN_BPM = 60, BEAT_MAX_BPM = 180;
const float BEAT_FAVORITE_BPM = 120;   // when half or double time fit about as well, lean toward this
const float BEAT_MIN_MATCH = 0.4;      // how well the spikes must repeat, 0 to 1, to call it a beat,
const float BEAT_KEEP_MATCH = 0.3;     // and to keep a beat it already has: harder to start than to carry on
const int BEAT_CONFIRM = 3;            // tempo checks (half a second each) that must agree before a tempo is believed
const int BEAT_PATIENCE = 3;           // tempo checks without a match before giving up on the beat
const int BEAT_RECENT = 188;           // blocks, three seconds: the beat has to show in these too, to count as still going,
const float BEAT_MIN_RECENT = 0.2;     // at least this well: lower than BEAT_MIN_MATCH, as three seconds is a rougher measure
const float BEAT_STICKY = 1.3;         // how much the tempo already found is favored over a new one
const float BEAT_QUIET_DB = 35;        // a band under this counts as silent, so hiss makes no onsets
const float BEAT_STOP_SECONDS = 1.5;   // this long without a hit means the music stopped
const float BEAT_MIN_ONSET = 10;       // a hit is an onset of at least this many dB, summed over the bands
const float BEAT_LEAD = 0.5;           // blocks the flash goes early, to make up for the time it takes a hit to get here
const float BEAT_FADE = 0.85;          // the flash keeps this much brightness each block: gone in a third of a second

float onsets[BEAT_HISTORY];  // a ring: onsetAt is where the next one goes, so also the oldest
int onsetAt = 0;
float spikes[BEAT_HISTORY];  // the onsets tidied up for a tempo check, newest first: see tidyOnsets()
float beatPeriod = 0;        // blocks from one beat to the next; 0 while there is no steady beat
float beatPhase = 0;         // the metronome: 0 on a beat, climbing to 1 at the next
int beatMisses = 0;          // tempo checks in a row that found nothing
float beatGlow = 0;          // brightness of the flash, 1 fading to 0
uint16_t beatHue = 0;

// Called with every block's band loudness, in decibels.
void updateBeat(const float bandDb[SPECTRUM_BANDS]) {
  static float lastDb[SPECTRUM_BANDS];
  static int blocks = 0;
  float onset = 0;
  for (int b = 0; b < SPECTRUM_BANDS; b++) {
    float dB = max(bandDb[b], BEAT_QUIET_DB);
    onset += max(dB - lastDb[b], 0.0f);  // only getting louder counts
    lastDb[b] = dB;
  }
  onsets[onsetAt] = onset;
  onsetAt = (onsetAt + 1) % BEAT_HISTORY;

  if (++blocks >= BEAT_FRAME_HZ / 2) {
    blocks = 0;
    findBeat();
  }
  if (beatPeriod > 0) {
    beatPhase += 1 / beatPeriod;
    if (beatPhase >= 1) {
      beatPhase -= 1;
      onBeat();
    }
  }
  fadeFlash();
}

// The onset from this many blocks ago, 0 being the newest.
float onsetAgo(int blocksAgo) {
  return onsets[(onsetAt - 1 - blocksAgo + 2 * BEAT_HISTORY) % BEAT_HISTORY];
}

// Steps 2 and 3: looks for a tempo in the onsets and lines the metronome up with it. A
// tempo is believed, or replaces the one before, once BEAT_CONFIRM checks in a row agree.
void findBeat() {
  static float candidate = 0;  // the tempo waiting to be believed
  static int agreed = 0;       // checks in a row that found it
  float match = 0;
  float period = tidyOnsets() ? findTempo(match) : 0;
  bool carryingOn = beatPeriod > 0 && sameTempo(period, beatPeriod);
  bool found = match >= (carryingOn ? BEAT_KEEP_MATCH : BEAT_MIN_MATCH) &&
               matchLately(period) >= BEAT_MIN_RECENT;  // and still going, not just remembered
  bool stopped = !heardLately();
  if (stopped || !found) {
    agreed = 0;
    if (beatPeriod > 0 && (stopped || ++beatMisses >= BEAT_PATIENCE)) {
      beatPeriod = 0;  // the music stopped, or lost its beat
      readings.beat[0] = 0;
      publish(beatChar);
    }
    return;
  }
  beatMisses = 0;
  // Where the metronome should be now, from how long ago the comb says the last beat was.
  float phase = fmodf((sinceLastBeat(period) + BEAT_LEAD) / period, 1);
  if (beatPeriod > 0 && sameTempo(period, beatPeriod)) {
    agreed = 0;
    beatPeriod += 0.3f * (period - beatPeriod);
    float error = phase - beatPhase;
    error -= roundf(error);  // the short way round: -0.5 to 0.5 of a beat
    beatPhase += 0.5f * error;
    beatPhase -= floorf(beatPhase);
  } else {
    agreed = sameTempo(period, candidate) ? agreed + 1 : 1;
    candidate = period;
    if (agreed >= BEAT_CONFIRM) {
      beatPeriod = period;
      beatPhase = phase;
    }
  }
  if (beatPeriod > 0) {
    readings.beat[0] = 60 * BEAT_FRAME_HZ / beatPeriod;
  }
}

bool sameTempo(float a, float b) {
  return fabsf(a - b) < 0.06f * b;
}

// Whether a drum hit arrived lately: the quick way to tell the music has stopped.
bool heardLately() {
  for (int i = 0; i < BEAT_STOP_SECONDS * BEAT_FRAME_HZ; i++) {
    if (onsetAgo(i) >= BEAT_MIN_ONSET) {
      return true;
    }
  }
  return false;
}

// How well the newest three seconds of spikes repeat at this tempo, 0 to 1. When the music
// stops but the room stays noisy, the six seconds still hold a beat for a while; these
// three lose it within a second or two. The better of the two whole numbers of blocks
// either side of the period is used.
float matchLately(float period) {
  float best = 0;
  for (int shift = period; shift <= period + 1; shift++) {
    float sum = 0;
    for (int i = 0; i + shift < BEAT_RECENT; i++) {
      sum += spikes[i] * spikes[i + shift];
    }
    best = max(best, sum / (BEAT_RECENT - shift));
  }
  return best;
}

// Copies the onsets into spikes, newest first, tidied in two ways. Each is blurred into
// its neighbors, because beats are rarely a whole number of blocks apart, so a hit lands
// a block early or late of where the last one predicts; blurred, a near miss still counts.
// Then the average of the second around it is taken off. That makes the gaps between hits
// negative, so a tempo that puts its beats in the gaps scores badly. It is the average
// nearby, not of all six seconds, so that a noise switching on or off, which lifts every
// onset after it, does not look like a pattern that repeats. Returns false in silence.
bool tidyOnsets() {
  static float blurred[BEAT_HISTORY];
  const int reach = BEAT_FRAME_HZ / 2;  // half a second each way
  float power = 0;
  for (int i = 0; i < BEAT_HISTORY; i++) {
    blurred[i] = (onsetAgo(max(i - 1, 0)) + 2 * onsetAgo(i) + onsetAgo(min(i + 1, BEAT_HISTORY - 1))) / 4;
  }
  for (int i = 0; i < BEAT_HISTORY; i++) {
    int from = max(i - reach, 0), to = min(i + reach, BEAT_HISTORY - 1);
    float nearby = 0;
    for (int j = from; j <= to; j++) {
      nearby += blurred[j] / (to - from + 1);
    }
    spikes[i] = blurred[i] - nearby;
    power += spikes[i] * spikes[i] / BEAT_HISTORY;
  }
  if (power < 0.01f) {
    return false;
  }
  for (int i = 0; i < BEAT_HISTORY; i++) {
    spikes[i] /= sqrtf(power);  // so a perfect repeat scores 1 however loud the music is
  }
  return true;
}

// Step 2: how many blocks apart the spikes repeat best, and in match how well, from 0 (not
// at all) to 1 (exactly). Tries every whole number of blocks between the fastest and
// slowest tempo, then reads between the blocks from the scores either side of the best.
float findTempo(float &match) {
  const int shortest = BEAT_FRAME_HZ * 60 / BEAT_MAX_BPM, longest = BEAT_FRAME_HZ * 60 / BEAT_MIN_BPM + 1;
  static float scores[BEAT_HISTORY / 4];
  match = 0;
  int best = 0;
  float bestScore = 0;
  for (int shift = shortest - 1; shift <= longest + 1; shift++) {
    float sum = 0;
    for (int i = 0; i + shift < BEAT_HISTORY; i++) {
      sum += spikes[i] * spikes[i + shift];
    }
    scores[shift] = sum / (BEAT_HISTORY - shift);
    // A beat also lines up with itself at half and double speed. Favor the tempo nearest
    // BEAT_FAVORITE_BPM: full weight there, about a third at half or double it.
    float octaves = log2f(60 * BEAT_FRAME_HZ / shift / BEAT_FAVORITE_BPM);
    float score = scores[shift] * expf(-octaves * octaves);
    if (beatPeriod > 0 && sameTempo(shift, beatPeriod)) {
      score *= BEAT_STICKY;  // so two tempos that fit about as well do not take turns
    }
    if (shift >= shortest && shift <= longest && score > bestScore) {
      bestScore = score;
      best = shift;
    }
  }
  if (best == 0) {
    return 0;
  }
  match = scores[best];
  // The top of the curve through the best score and its two neighbors.
  float before = scores[best - 1], after = scores[best + 1];
  float bend = before - 2 * match + after;
  return bend < 0 ? best + 0.5f * (before - after) / bend : best;
}

// Step 3: how many blocks ago the last beat was. Tries every spot within one beat, adding up
// the spikes found there and at every whole beat before it; the beat is where the sum is
// biggest.
float sinceLastBeat(float period) {
  int best = 0;
  float bestSum = -1e9;
  for (int ago = 0; ago < period; ago++) {
    float sum = 0;
    for (float tooth = ago; tooth < BEAT_HISTORY - 1; tooth += period) {
      sum += spikes[lroundf(tooth)];
    }
    if (sum > bestSum) {
      bestSum = sum;
      best = ago;
    }
  }
  return best;
}

// The metronome ticked: start a flash in the next color and tell the page.
void onBeat() {
  beatGlow = 1;
  beatHue += 65536 / 6;  // six colors round the wheel
  readings.beat[1]++;
  publish(beatChar);
}

// Fades the flash a little each block, then hands the pixel back to the page's color.
void fadeFlash() {
  if (beatGlow == 0) {
    return;
  }
  beatGlow *= BEAT_FADE;
  if (beatGlow < 0.02f) {
    beatGlow = 0;
    showBackColor();
    return;
  }
  pixel.fill(pixel.ColorHSV(beatHue, 255, 255 * beatGlow));
  pixel.show();
}
