// A tilt-switch "light cup" pour effect. Tilting the sensor slowly fills an
// 8-LED NeoPixel ring with a water-blue level while the cup's own bulb dims,
// so the two together read as liquid pouring into the cup. Hold it level and
// the level drains away.
//
// Wiring:
//   Tilt switch S pin -> Arduino digital pin 2
//   Tilt switch VCC/GND -> 5V / GND
//   Ring DIN           -> Arduino digital pin 4, through a 330 ohm resistor
//   Cup L pin (LED)    -> Arduino digital pin 3 (PWM, via a current-limiting
//                         resistor or transistor if the LED draws > 12 mA)

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define NEO_DATA_PIN   4
#define NUM_LEDS       8

#define CUP_SIGNAL_PIN 2  // Connects to the 'S' pin on the Light Cup
#define CUP_LED_PIN    3  // Connects to the 'L' pin on the Light Cup

Adafruit_NeoPixel ring(NUM_LEDS, NEO_DATA_PIN, NEO_GRB + NEO_KHZ800);

// Tracking variable for smooth fluid transitions
int currentLiquidLevel = 0;

void setup() {
#if defined(__AVR__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1);
#endif
  ring.begin();
  ring.show();
  ring.setBrightness(50); // Comfortable baseline brightness

  pinMode(CUP_SIGNAL_PIN, INPUT);
  pinMode(CUP_LED_PIN, OUTPUT);
}

void loop() {
  // 1. Read the tilt state from the rolling sensor ball.
  // Most KY-027 modules pull LOW when tilted forward.
  bool isTilted = (digitalRead(CUP_SIGNAL_PIN) == LOW);

  // 2. Animate the liquid line filling or draining
  if (isTilted) {
    // If tilted, smoothly increase the fluid level up to max
    if (currentLiquidLevel < NUM_LEDS) {
      currentLiquidLevel++;
    }
  } else {
    // If held level, let the fluid drain away down to 0
    if (currentLiquidLevel > 0) {
      currentLiquidLevel--;
    }
  }

  // 3. Render the physical ring colors based on the level
  ring.clear();
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < currentLiquidLevel) {
      // Light the active level with a glowing water-blue hue
      ring.setPixelColor(i, ring.Color(0, 160, 255));
    }
  }
  ring.show();

  // 4. Drive the on-board Light Cup LED inversely to create the pouring
  // illusion. As the ring fills, the cup's own bulb fades out completely.
  int cupBrightness = map(currentLiquidLevel, 0, NUM_LEDS, 255, 0);
  analogWrite(CUP_LED_PIN, cupBrightness);

  // Controls how fast the "fluid" flows (ms per animation step)
  delay(60);
}