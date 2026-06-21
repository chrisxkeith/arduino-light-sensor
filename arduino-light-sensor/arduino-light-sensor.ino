class Utils {
  private:
    static unsigned long lastPrintln;
  public:
    static bool debug;
    // Slows everything down to make Serial output readable.
    static void publish(String s) {
      if (millis() < lastPrintln + 1000) {
        delay(1000);
      }
      Serial.println(s);
    }
    static void checkSerial();
    static bool waitForSerial(String s);
    static String msToString(unsigned long ms) {
      int totalSeconds = ms / 1000;
      int secs = totalSeconds % 60;
      int minutes = (totalSeconds / 60) % 60;
      int hours = (totalSeconds / 60) / 60;
    
      char buf[100];
      sprintf(buf, "%02u:%02u:%02u", hours, minutes, secs);
      return String(buf);
    }
};
unsigned long Utils::lastPrintln = 0;
bool Utils::debug = false;

const int COLOR_WHITE = 0x65535;
const int COLOR_BLACK = 0x0;
#include "Arduino_GigaDisplay_GFX.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/Org_01.h"
#include "Fonts/Picopixel.h"
#include "Fonts/Tiny3x3a2pt7b.h"
#include "Fonts/TomThumb.h"

GigaDisplay_GFX display_;

class OLEDWrapper {
  private:
    uint16_t currentColor = COLOR_WHITE;
    const int DEFAULT_FONT_SIZE = 3;
  public:
    void clear() {
      display_.fillScreen(COLOR_BLACK);
    }
    void startup() {
      delay(1000);
      display_.begin(); //init library
      clear();
      display_.setRotation(1);
    }
    void display(String s, const GFXfont* font, int textSize, uint8_t x, uint8_t y) {
      display_.setCursor(x, y);
      display_.setFont(font);
      display_.setTextSize(textSize);
      display_.print(s);
    }
    void display(String s, int textSize, uint8_t x, uint8_t y) {
      display(s, nullptr, textSize, x, y);
    }
    void display(String s) {
      display(s, DEFAULT_FONT_SIZE, 10, 10);
    }
    void display(String s[], int nStrings) {
      for (int i = 0; i < nStrings; i++) {
        display(s[i], DEFAULT_FONT_SIZE, 10, 32 + (i * 32));
      }
    }
    void setDrawColor(int color) {
      currentColor = color;
    }
    void setFont(const GFXfont* font) {
      display_.setFont(font);
    }
    void getTextBounds(String string, const GFXfont* font, int textSize, int16_t* x1, int16_t* y1, uint16_t* x2, uint16_t* y2) {
      display_.setFont(font);
      display_.setTextSize(textSize);
      uint16_t w;
      uint16_t h;
      display_.getTextBounds(string, 0, 0, x1, y1, &w, &h);
      *x2 = *x1 + w;
      *y2 = *y1 + h;
    }
    void drawLine(int x0, int y0, int x1, int y1) {
      // Rotate not happening automatically?
      display_.drawLine(y0, x0, y1, x1, currentColor);
    }
    void fillRect(int x0, int y0, int x1, int y1, int color) {
      display_.fillRect(x0, y0, x1 - x0, y1 - y0, color);
    }  
    int getHeight() {
      return display_.height();
    }
    int getWidth() {
      return display_.width();
    }
    void dump() {
      String s("OLEDWrapper: getHeight(): ");
      s.concat(getHeight());
      s.concat(", getWidth(): ");
      s.concat(getWidth());
      Utils::publish(s);
    }
    void test2() {
      for (int r = 0; r < 4; r++) {
        clear();
        display_.setRotation(r);
        for (int i = 0; i < 24; i++) {
          int x = 0;
          int y = i * 20;
          String s(i);
          s.concat(",");
          s.concat(x);
          s.concat(",");
          s.concat(y);
          s.concat(",");
          s.concat(getWidth());
          s.concat(",");
          s.concat(getHeight());
          display(s, 2, x, y);
          Utils::publish(s);
          delay(1000);
        }
        delay(5000);
      }
    }
    void oneFontTest(const GFXfont* font, String fontName) {
      int16_t x1;
      int16_t y1;
      uint16_t w;
      uint16_t h;

      clear();
      setFont(font);
      display_.setTextSize(1);
      display_.getTextBounds(fontName, 0, 0, &x1, &y1, &w, &h);
      String s(fontName);
      s.concat(", w: ");
      s.concat(w);
      s.concat(", h: ");
      s.concat(h);
      display(s, 1, 10, 10);
      Utils::publish(s);
      delay(10000);
    }
    void fontTest() {
      oneFontTest(&Org_01, "Org_01");
      oneFontTest(&Picopixel, "Picopixel");
      oneFontTest(&Tiny3x3a2pt7b, "Tiny3x3a2pt7b");
      oneFontTest(&TomThumb, "TomThumb");
      oneFontTest(&FreeSans18pt7b, "FreeSans18pt7b");
      setFont(nullptr);
    }
};
OLEDWrapper* oledWrapper = nullptr;

#include <vector>
#include <cmath>
#include <tuple>
class Spinner {
  private:
    int middleX = oledWrapper->getWidth() / 2;
    int middleY = oledWrapper->getHeight() / 2;
    int lineLength = min(middleX, middleY);
    int color = COLOR_WHITE;
    int deg = 0;
    unsigned long msWhenOn = 0;
    unsigned long baseline = 0;
    unsigned long lastShift = 0;
    int incrementDegrees = 1;
    unsigned long lastDisplayTime = 0;
    const unsigned long DISPLAY_INTERVAL = 200; // milliseconds

    int prevBaseline = 0;
    int16_t getStartBaseline() {
/*      int16_t x1;
      int16_t y1;
      uint16_t x2;
      uint16_t y2;
      oledWrapper->getTextBounds("00:00:00", &FreeSans18pt7b, 1, &x1, &y1, &x2, &y2);
      return y2;
*/
      return 32;      
    }
    void drawElapsed() {
      unsigned long elapsed = millis() - msWhenOn;
      String s = Utils::msToString(elapsed);
      int16_t x1;
      int16_t y1;
      uint16_t x2;
      uint16_t y2;
      oledWrapper->getTextBounds(s, &FreeSans18pt7b, 1, &x1, &y1, &x2, &y2);
      oledWrapper->fillRect(x1, y1, x2, y2, COLOR_BLACK);
      oledWrapper->display(s, &FreeSans18pt7b, 1, 0, baseline);
      if (millis() - lastShift > 1000 * 10) {
        prevBaseline = baseline;
        baseline += 3;
        oledWrapper->getTextBounds(s, &FreeSans18pt7b, 1, &x1, &y1, &x2, &y2);
        if (y2 > oledWrapper->getHeight()) {
          baseline = getStartBaseline();
        }
        lastShift = millis();
      }
    }
  public:
    Spinner(int incrementDegrees) {
      this->incrementDegrees = incrementDegrees;
      baseline = getStartBaseline();
      prevBaseline = baseline;
    }
    void reset() {
      deg = 0;
      color = COLOR_WHITE;
      msWhenOn = millis();
    }
    void display() {
      if (millis() - lastDisplayTime < DISPLAY_INTERVAL) {
        return;
      }
      int xEnd = lineLength * cos(deg * M_PI / 180.0);
      int yEnd = lineLength * sin(deg * M_PI / 180.0);

      oledWrapper->setDrawColor(color);
      oledWrapper->drawLine(middleX, middleY, middleX + xEnd, middleY + yEnd);
      drawElapsed();
      deg += incrementDegrees;
      if (deg >= 360) {
        deg = 0;
        if (color == COLOR_WHITE) {
          color = COLOR_BLACK;
        } else {
          color = COLOR_WHITE;
        }
      }
      lastDisplayTime = millis();
    }
    void dump() {
      String s("Spinner: middleX: ");
      s.concat(middleX);
      s.concat(", middleY: ");
      s.concat(middleY);
      s.concat(", lineLength: ");
      s.concat(lineLength);
      s.concat(", baseline: ");
      s.concat(baseline);
      Utils::publish(s);
    }
};
Spinner spinner(5);

class Sensor {
  private:
    int     pin;
    String  name;
    int     nSamples;
    double  total;
    unsigned long lastPublish = 0;

  public:
    const int THRESHOLD = 15;
    bool on = false;
    bool publish = false;

    bool      testing = false;
    int       testValue = -1;
    int       lastTestSpinnerMillis = millis();
    const int TEST_SPINNER_DURATION_MS = 3000;

    Sensor(int pin, String name) {
      this->pin = pin;
      this->name = name;
      clear();
      pinMode(pin, INPUT);
    }    
    void sample() {
      if (pin >= A0 && pin <= A5) {
          total += analogRead(pin);
      } else {
          total += digitalRead(pin);
      }
      nSamples++;
    }  
    void clear() {
      nSamples = 0;
      total = 0.0;
    }
    void publishData() {
      if (publish && (millis() > lastPublish + 2000)) {
        String s(Utils::msToString(millis()));
        s.concat(" Sensor_value: ");
        s.concat(getValue());
        Utils::publish(s);
        lastPublish = millis();
      }
    }
    int getValue() {
      if (testing) {
        if (millis() > lastTestSpinnerMillis + TEST_SPINNER_DURATION_MS) {
          if (testValue < THRESHOLD) {
            testValue = THRESHOLD + 10;
          } else {
            testValue = THRESHOLD - 10;  
          }
          lastTestSpinnerMillis = millis();
        }
        return testValue;
      }
      return round(total / nSamples);
    }
};
Sensor lightSensor1(A0, "Arduino light sensor");

class Config {
  public:
    const String build = "~Sat Jun 20 04:15:24 PM PDT 2026";
    void dump() {
      String s("gitHubRepository: https://github.com/chrisxkeith/arduino-light-sensor");
      Utils::publish(s);
      oledWrapper->dump();
      spinner.dump();
      s.remove(0);
      s.concat("build: ");
      s.concat(build);
      Utils::publish(s);
      s.remove(0);
      s.concat("THRESHOLD: ");
      s.concat(String(lightSensor1.THRESHOLD));
      Utils::publish(s);
   }
};
Config config;

class App {
  private:
    void display_on_oled() {
      lightSensor1.sample();
      int value = lightSensor1.getValue();
      if ((value > lightSensor1.THRESHOLD) != lightSensor1.on) {
        lightSensor1.on = !lightSensor1.on;
        oledWrapper->clear();
        if (lightSensor1.on) {
          spinner.reset();
          spinner.display();
        }
      } else {
        if (lightSensor1.on) {
          spinner.display();
        }
      }
      lightSensor1.publishData();
      lightSensor1.clear();
    }
  public:
    void setup() {
      Serial.begin(115200);
      Utils::publish("setup() : started.");
      oledWrapper = new OLEDWrapper();
      oledWrapper->startup();
      showBuild();
      config.dump();
      Utils::publish("setup() : finished.");
    }
    void loop() {
      display_on_oled();
      Utils::checkSerial();
    }
    void showBuild() {
      oledWrapper->clear();
      String s[2] = { String("Build:"), config.build };
      oledWrapper->display(s, 2);
      delay(3000);
      oledWrapper->clear();
    } 
    void testSpinner() {
      lightSensor1.testing = true;
    } 
};
App app;

void Utils::checkSerial() {
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();
    String s("About to run: ");
    s.concat(command);
    Utils::publish(s);
    if (command.equals("?")) {
      config.dump();
    } else if (command.equals("debug on")) {
      Utils::debug = true;
      Utils::publish("Debugging enabled.");
    } else if (command.equals("debug off")) {
      Utils::debug = false;
      Utils::publish("Debugging disabled.");
     } else if (command.equals("publish on")) {
      lightSensor1.publish = true;
    } else if (command.equals("publish off")) {
      lightSensor1.publish = false;
    } else if (command.equals("showBuild")) {
      app.showBuild();
    } else if (command.equals("testSpinner")) {
      app.testSpinner();
    } else if (command.equals("dumpSpinner")) {
      spinner.dump();
    } else if (command.equals("dumpOled")) {
      oledWrapper->dump();
    } else {
      Utils::publish("Unknown command: " + command);
      config.dump(); 
    }
  }
}
bool Utils::waitForSerial(String s) {
  if (Serial.available() > 0) {
    String command = Serial.readString();
    command.trim();
    return (!command.equals(s));
  }
  return true;
}

void setup() {
  app.setup();
}

void loop() {
   app.loop();
}