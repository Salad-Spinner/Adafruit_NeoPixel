// A six-mode showcase for an 8-LED NeoPixel ring, cycling every 6 seconds.
//
// Modes, in cycle order:
//   0. Loading spinner  - trailing comet, for UI busy states
//   1. Analog gauge    - green/yellow/red bar driven by a mock sensor value
//   2. Fireplace       - flickering warm amber, for ambient mood lighting
//   3. Rainbow wheel   - rotating HSV spectrum
//   4. Emergency strobe- alternating red/blue halves
//   5. Sleep mode      - slow brightness breathe
//
// Modes 0-4 are self-contained and need no extra hardware. A potentiometer
// on A0 is optional and only replaces the simulated value in mode 1.
//
// Wiring: DIN on the ring -> Arduino digital pin 4, with a 330 ohm resistor
// in series. Power the ring separately when drawing more than a few LEDs.

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define DATA_IN_PIN 4
#define NUM_LEDS 8

// Set to true to drive mode 1 from a potentiometer on A0 instead of the
// simulated sine wave.
#define USE_POTENTIOMETER false
#define POT_PIN A0

#define NUM_MODES 6

// Declared up front so the sketch also builds as plain C++ (e.g. in CI or a
// strict IDE) rather than relying on the Arduino preprocessor's auto-prototypes.
void runLoadingSpinner();
void runAnalogDialGauge();
void runFireplaceFlicker();
void runColorWheelClock();
void runEmergencyStrobe();
void runSleepMode();
void fillRing(uint32_t color);

Adafruit_NeoPixel ring(NUM_LEDS, DATA_IN_PIN, NEO_GRB + NEO_KHZ800);

// Tracking the active display mode
int activeMode = 0;
unsigned long modeStartTime = 0;
const unsigned long MODE_DURATION = 6000;  // Switch modes every 6 seconds

void setup() {
#if defined(__AVR__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1);
#endif
  ring.begin();
  ring.show();
  ring.setBrightness(60);  // Controlled baseline brightness
  modeStartTime = millis();
#if USE_POTENTIOMETER
  pinMode(POT_PIN, INPUT);
#endif
}

void loop() {
  // Check if it's time to cycle to the next demo use case
  if (millis() - modeStartTime >= MODE_DURATION) {
    activeMode = (activeMode + 1) % NUM_MODES;  // Cycle 0 through NUM_MODES - 1
    modeStartTime = millis();
    ring.clear();
  }

  // Execute the active demonstration function
  switch (activeMode) {
    case 0: runLoadingSpinner(); break;    // UI loading state
    case 1: runAnalogDialGauge(); break;   // Sensor/volume gauge dashboard
    case 2: runFireplaceFlicker(); break;  // Mood lighting and decor
    case 3: runColorWheelClock(); break;   // Chrono-time wheel / rainbow orbit
    case 4: runEmergencyStrobe(); break;   // Warning / notification alert
    case 5: runSleepMode(); break;         // Rest / sleep indicator
  }
}

// ==========================================
// MODE 0: SMOOTH LOADING SPINNER (Process Indicator)
// ==========================================
void runLoadingSpinner() {
  int head = (millis() / 40) % NUM_LEDS;  // Control spin speed via time
  ring.clear();
  for (int i = 0; i < 6; i++) {
    int pixel = (head - i + NUM_LEDS) % NUM_LEDS;
    float fade = 1.0 - ((float)i / 6.0);
    ring.setPixelColor(pixel, ring.Color(0, 200 * fade, 255 * fade));  // Cyan comet tail
  }
  ring.show();
  delay(10);
}

// ==========================================
// MODE 1: ANALOG DIAL GAUGE (Volume / Sensor Display)
// ==========================================
void runAnalogDialGauge() {
  // Simulates a sensor reading going up and down over time. Normalised 0.0-1.0.
  float mockSensorValue = (sin(millis() / 800.0) + 1.0) / 2.0;

  // An optional real potentiometer overrides the simulated value.
#if USE_POTENTIOMETER
  mockSensorValue = analogRead(POT_PIN) / 1023.0;
#endif

  int targetLeds = round(mockSensorValue * NUM_LEDS);

  // Thresholds are scaled to NUM_LEDS so every band stays reachable at any
  // ring size. With 8 LEDs this gives 4 green, 2 yellow, 2 red.
  int greenEnd = round(NUM_LEDS * 0.50);
  int yellowEnd = round(NUM_LEDS * 0.75);

  ring.clear();
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < targetLeds) {
      if (i < greenEnd) {
        ring.setPixelColor(i, ring.Color(0, 150, 0));         // Green
      } else if (i < yellowEnd) {
        ring.setPixelColor(i, ring.Color(150, 120, 0));      // Yellow
      } else {
        ring.setPixelColor(i, ring.Color(180, 0, 0));        // Red peak
      }
    }
  }
  ring.show();
  delay(20);
}

// ==========================================
// MODE 2: COZY FIREPLACE FLICKER (Ambient Mood Light)
// ==========================================
void runFireplaceFlicker() {
  // Small random variations create an organic fire flicker
  for (int i = 0; i < NUM_LEDS; i++) {
    int flickerValue = random(0, 60);
    // Compute in int, then clamp before narrowing to uint8_t so a negative
    // value cannot wrap around to 255 and flash white.
    int red = 200 - flickerValue;
    int green = 65 - (flickerValue / 2);
    int blue = 0;

    if (red < 0) red = 0;
    if (green < 0) green = 0;
    if (blue < 0) blue = 0;

    ring.setPixelColor(i, ring.Color((uint8_t)red, (uint8_t)green, (uint8_t)blue));
  }
  ring.show();
  delay(random(50, 120));  // Organic variable frame delay
}

// ==========================================
// MODE 3: THE CHRONO RAINBOW WHEEL (Color Gradient Orbit)
// ==========================================
void runColorWheelClock() {
  // Built-in 16-bit HSV calculations rotate the spectrum smoothly
  long currentHueOffset = (millis() * 5) % 65536;

  for (int i = 0; i < NUM_LEDS; i++) {
    // Spatial pixel shift plus time shift creates motion
    long pixelHue = currentHueOffset + (i * 65536L / NUM_LEDS);
    ring.setPixelColor(i, ring.gamma32(ring.ColorHSV(pixelHue)));
  }
  ring.show();
  delay(10);
}

// ==========================================
// MODE 4: EMERGENCY SIGNAL / NOTIFICATION ALERT
// ==========================================
void runEmergencyStrobe() {
  unsigned long timeCheck = millis() % 1000;
  ring.clear();

  // Alternate halves every half second for an emergency vehicle effect
  if (timeCheck < 500) {
    // Flash left half solid red
    for (int i = 0; i < NUM_LEDS / 2; i++) {
      ring.setPixelColor(i, ring.Color(255, 0, 0));
    }
  } else {
    // Flash right half solid blue
    for (int i = NUM_LEDS / 2; i < NUM_LEDS; i++) {
      ring.setPixelColor(i, ring.Color(0, 0, 255));
    }
  }
  ring.show();
  delay(30);
}

// =============================================
// MODE 5: SLEEP MODE
// Slowly fades up and down like a sleeping device.
// =============================================
void runSleepMode() {
  // Fade up
  for (int b = 10; b < 120; b += 2) {
    fillRing(ring.Color(0, 150, 255));  // Warm blue
    ring.setBrightness(b);
    ring.show();
    delay(15);
  }
  // Fade down
  for (int b = 120; b > 10; b -= 2) {
    fillRing(ring.Color(0, 150, 255));  // Warm blue
    ring.setBrightness(b);
    ring.show();
    delay(15);
  }
}

// Simple helper to fill the whole ring instantly
void fillRing(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    ring.setPixelColor(i, color);
  }
}