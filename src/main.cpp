#include <M5Cardputer.h>

static constexpr uint16_t BACKGROUNDS[] = {
    TFT_BLACK,
    TFT_NAVY,
    TFT_DARKGREEN,
    TFT_MAROON,
    TFT_PURPLE,
    TFT_DARKCYAN,
};
static constexpr size_t BACKGROUND_COUNT = sizeof(BACKGROUNDS) / sizeof(BACKGROUNDS[0]);
static constexpr char MESSAGE[] = "Jesus voltara!";

size_t backgroundIndex = 0;

void drawScreen() {
  auto &display = M5Cardputer.Display;
  display.fillScreen(BACKGROUNDS[backgroundIndex]);
  display.setTextColor(TFT_WHITE, BACKGROUNDS[backgroundIndex]);
  display.setTextDatum(middle_center);
  display.setTextSize(2);
  display.drawString(MESSAGE, display.width() / 2, display.height() / 2);
}

void setup() {
  auto config = M5.config();
  M5Cardputer.begin(config, true);
  M5Cardputer.Display.setRotation(1);
  drawScreen();
}

void loop() {
  M5Cardputer.update();

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    const auto keys = M5Cardputer.Keyboard.keysState();
    if (keys.fn && keys.down) {
      backgroundIndex = (backgroundIndex + 1) % BACKGROUND_COUNT;
      drawScreen();
    }
  }

  delay(5);
}
