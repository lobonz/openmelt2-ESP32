#include "arduino.h"
#include "melty_config.h"

#if USE_RGB_LED
#include <FastLED.h>

CRGB leds[NUM_RGB_LEDS];

// Define RGB color values
CRGB getColorValue(led_color_t color) {
  switch(color) {
    case RED:
      return CRGB::Red;
    case BLUE:
      return CRGB::Blue;
    case YELLOW:
      return CRGB::Yellow;
    case GREEN:
      return CRGB::Green;
    case PURPLE:
      return CRGB::Purple;
    case MAGENTA:
      return CRGB::Magenta;
    case ORANGE:
      return CRGB::Orange;
    default:
      return CRGB::Green;
  }
}
#endif

void init_led(void) {
#if USE_RGB_LED
  FastLED.addLeds<RGB_LED_TYPE, HEADING_LED_PIN, GRB>(leds, NUM_RGB_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear();
  FastLED.show();
#else
  pinMode(HEADING_LED_PIN, OUTPUT);
#endif
}

void heading_led_on(int shimmer) {
#if USE_RGB_LED
  //check to see if we should "shimmer" the LED to indicate something to user
  if (shimmer == 1) {
    if (micros() & (1 << 10)) {
      leds[0] = getColorValue(RGB_LED_COLOR);
      FastLED.show();
    } else {
      leds[0] = CRGB::Black;
      FastLED.show();
    }  
  } else {
    //just turn LED on with the configured color
    leds[0] = getColorValue(RGB_LED_COLOR);
    FastLED.show();
  }
#else
  //check to see if we should "shimmer" the LED to indicate something to user
  if (shimmer == 1) {
    if (micros() & (1 << 10)) {
      digitalWrite(HEADING_LED_PIN, HIGH);
    } else {
      digitalWrite(HEADING_LED_PIN, LOW);
    }  
  } else {
    //just turn LED on
     digitalWrite(HEADING_LED_PIN, HIGH);
  }
#endif
}

void heading_led_off() {
#if USE_RGB_LED
  leds[0] = CRGB::Black;
  FastLED.show();
#else
  digitalWrite(HEADING_LED_PIN, LOW);
#endif
}
