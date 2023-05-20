#include <RotaryEncoder.h>
#include <FastLED.h>

#define SECONDS_PER_STEP 5

#define ENCODER_DT_PIN 3
#define ENCODER_CLK_PIN 4
#define ENCODER_SW_PIN 5
#define LED_RING_PIN 7
#define BUZZER_PIN 6
#define LED_COUNT 12

RotaryEncoder encoder(ENCODER_CLK_PIN, ENCODER_DT_PIN);
CRGB leds[LED_COUNT];

unsigned int ledLevel = 0;

unsigned long timerDuration = 0;
unsigned long timerStart = 0;
unsigned int colorIndex = 0;
CRGB colors[] = { CRGB::Green, CRGB::Blue, CRGB::Orange, CRGB::Purple, CRGB::Red };
unsigned int colorCount = sizeof(colors) / sizeof(colors[0]);

enum State {
  TIMER_SELECT = 0,
  COLOR_SELECT = 1,
  TIMER_RUNNING = 2,
  TIMER_END = 3
};

State currentState = TIMER_SELECT;
bool prevSwitchState = HIGH;

#define COLOR_CYCLE_TIME 1000

void setLEDs(int level, CRGB color, CRGB dark = CRGB::Black) {
  for (int i = 0; i < LED_COUNT; i++) {
    if (i < level) {
      leds[i] = color;  // Green color
    } else {
      leds[i] = dark;  // Off
    }
  }
  FastLED.show();
}

void alarm(CRGB color) {
  if (millis() % 500 > 300 ){
    setLEDs(LED_COUNT, color);
    tone(BUZZER_PIN, 2000, 300);  // 2000 Hz
  }
  else {
    setLEDs(LED_COUNT, CRGB::Black);
    noTone(BUZZER_PIN);
  }
}

void click() {
  tone(BUZZER_PIN, 300, 20);  // Play a short beep
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LED_RING_PIN, GRB>(leds, LED_COUNT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
  //check
  for (int i = 0; i <= LED_COUNT; i++) {
    setLEDs(i, CRGB::Green);
    click();
    delay(50);
  }
  setLEDs(0, CRGB::Green);
  encoder.setPosition(0);
}

void loop() {
  encoder.tick();
  int direction = (int)encoder.getDirection();
  int position = (int)encoder.getPosition();

  bool pressed = (prevSwitchState == HIGH && digitalRead(ENCODER_SW_PIN) == LOW);
  prevSwitchState = digitalRead(ENCODER_SW_PIN);

  // Check if rotary encoder has been pressed
  if (pressed) {
    ledLevel = 0;
    setLEDs(0, CRGB::Blue);
    switch (currentState) {
      case TIMER_SELECT:
        encoder.setPosition(0);
        currentState = COLOR_SELECT;
        break;
      case COLOR_SELECT:
        timerStart = millis() / 1000;
        currentState = TIMER_RUNNING;
        break;
      case TIMER_RUNNING:
        currentState = TIMER_SELECT;
        ledLevel = 0;
        break;
      case TIMER_END:
        currentState = TIMER_SELECT;
        ledLevel = 0;
        break;
    }
  }

  switch (currentState) {
    case TIMER_SELECT:
      {
        setLEDs(ledLevel, CRGB::Blue);
        if (direction == 0) break;
        ledLevel = ledLevel + direction;
        if (ledLevel < 0) ledLevel = 0;
        if (ledLevel > LED_COUNT) ledLevel = LED_COUNT;
        timerDuration = ledLevel * SECONDS_PER_STEP;
        click();
      }
      break;

    case COLOR_SELECT:
      {
        colorIndex = position % colorCount;
        setLEDs(LED_COUNT, colors[colorIndex]);
        if (direction == 0) break;
        click();
      }
      break;

    case TIMER_RUNNING:
      {
        uint8_t brightness = (.3) * 20;
        CRGB dimColor = colors[colorIndex];
        dimColor.nscale8(brightness);

        unsigned long elapsedSeconds = trunc(millis() / 1000 - timerStart);
        int progress = trunc(elapsedSeconds * LED_COUNT / timerDuration);
        Serial.println(progress, elapsedSeconds);
        setLEDs(progress, colors[colorIndex], dimColor);
        if (elapsedSeconds >= timerDuration) {
          currentState = TIMER_END;
          break;
        }
      }
      break;

    case TIMER_END:
      {
        alarm(colors[colorIndex]);
      }
      break;
  }
}