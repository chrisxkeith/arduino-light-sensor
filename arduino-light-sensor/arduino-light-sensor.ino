
#include <U8g2lib.h>

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
    static String getMinSecString(unsigned long ms) {
      unsigned long seconds = (ms / 1000) % 60;
      unsigned long minutes = (ms / 1000 / 60) % 60;
      char s[32];
      sprintf(s, "%02u:%02u", minutes, seconds);
      String elapsed(s);
      return elapsed;
    }
};
unsigned long Utils::lastPrintln = 0;
bool Utils::debug = false;

U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
const int COLOR_WHITE = 1;
const int COLOR_BLACK = 0;
class OLEDWrapper {
  private:
      const int START_BASELINE = 50;
      int       baseLine = START_BASELINE;
  public:
    void u8g2_prepare(void) {
      u8g2.setFont(u8g2_font_fur49_tn);
      u8g2.setFontRefHeightExtendedText();
      u8g2.setDrawColor(COLOR_WHITE);
      u8g2.setFontDirection(0);
    }
    void display(int x, int y, String s) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_fur11_tf);
      u8g2.drawUTF8(x, y, s.c_str());
      u8g2.sendBuffer();
    }
    void clear() {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.sendBuffer();
    }
    void shiftDisplay(int shiftAmount) {
      this->baseLine += shiftAmount;
      if (this->baseLine > 70) {
        this->baseLine = START_BASELINE;
      }
    }
    void startup() {
      pinMode(10, OUTPUT);
      pinMode(9, OUTPUT);
      digitalWrite(10, 0);
      digitalWrite(9, 0);
      u8g2.begin();
      u8g2.setBusClock(400000);
    }
    void startDisplay(const uint8_t *font) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.setFont(font);
    }
    void display(String s, uint8_t x, uint8_t y) {
      u8g2.setCursor(x, y);
      u8g2.print(s.c_str());
    }
    void endDisplay() {
      u8g2.sendBuffer();
    }
    void display(String s) {
      startDisplay(u8g2_font_fur11_tf);
      display(s, 0, 16);
      endDisplay();
    }
    void display(String s[], int nStrings) {
      startDisplay(u8g2_font_fur11_tf);
      for (int i = 0; i < nStrings; i++) {
        display(s[i], 0, 16 + (i * 16));
      }
      endDisplay();
    }
    void test1() {
      for (u8g2_uint_t h = u8g2.getHeight(); h > 95; h -= 1) {
        startDisplay(u8g2_font_fur11_tf);
        u8g2.drawFrame(0, 0, getWidth(), h);
        u8g2.drawUTF8(8, 32, String(h).c_str());
        endDisplay();
      }
    }
    void test2() {
      for (int h = 8; h > -8; h -= 1) {
        startDisplay(u8g2_font_fur11_tf);
        u8g2.drawLine(0, h, getWidth(), h);
        u8g2.drawUTF8(8, 32, String(h).c_str());
        endDisplay();
      }
    }
    void test3() {
      unsigned long start = millis();
      startDisplay(u8g2_font_fur11_tf);
      for (int y = 0; y < getHeight(); y++) {
        u8g2.drawPixel(0, y);
        endDisplay();
      }
      unsigned long duration = millis() - start;
      Utils::publish("test3() duration: " + String(duration) + " ms");
    }
    void test4() {
      unsigned long start = millis();
      for (int y = 0; y < getHeight(); y++) {
        startDisplay(u8g2_font_fur11_tf);
        u8g2.drawPixel(0, y);
        endDisplay();
      }
      unsigned long duration = millis() - start;
      Utils::publish("test4() duration: " + String(duration) + " ms");
    }
    void test() {
      test3();
      test4();
/*  test3() duration: 36840 ms
    test4() duration: 36868 ms
*/
    }
    int getHeight() {
      return 96; // ??? why does u8g2.getHeight() return 128 ???
    }
    int getWidth() {
      return u8g2.getWidth();
    }
};
OLEDWrapper* oledWrapper = new OLEDWrapper();

class Spinner {
  private:
    int middleX = oledWrapper->getWidth() / 2;
    int middleY = oledWrapper->getHeight() / 2;
    int lineWidth = min(middleX, middleY);
    int color = COLOR_WHITE;
    int deg = 0;
    int incrementDegrees = 1;

  public:
    Spinner(int incrementDegrees) {
      this->incrementDegrees = incrementDegrees;
    }
    void reset() {
      deg = 0;
      color = COLOR_WHITE;
    }
    void display() {
      int xEnd = lineWidth * cos(deg * M_PI / 180.0);
      int yEnd = lineWidth * sin(deg * M_PI / 180.0);

      oledWrapper->u8g2_prepare();
      u8g2.setDrawColor(color);
      u8g2.drawLine(middleX, middleY, middleX + xEnd, middleY + yEnd);
      u8g2.sendBuffer();
      deg += incrementDegrees;
      if (deg >= 360) {
        deg = 0;
        if (color == COLOR_WHITE) {
          color = COLOR_BLACK;
        } else {
          color = COLOR_WHITE;
        }
      }
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
        String s(Utils::getMinSecString(millis()));
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
    const String build = "Mon Dec  8 05:13:33 PM PST 2025";
    void dump() {
      String s("gitHubRepository: https://github.com/chrisxkeith/arduino-light-sensor");
      Utils::publish(s);
      s.remove(0);
      s.concat("oledWrapper->getWidth(): ");
      s.concat(String(oledWrapper->getWidth()));
      Utils::publish(s);
      s.remove(0);
      s.concat("oledWrapper->getHeight(): ");
      s.concat(String(oledWrapper->getHeight()));
      Utils::publish(s);
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
    bool gatheringData = false;
    void gatherValues() {
      int totalSeconds = 10;
      int total = 0;
      for (int i = 0; i < totalSeconds; i++) {
        lightSensor1.sample();
        int value = lightSensor1.getValue();
        oledWrapper->display(String(totalSeconds - i), 0, 1);
        Utils::publish(String(value)); 
        delay(1000);
        total += value;
        lightSensor1.clear();
      }
      int avg = total / totalSeconds;
      String avgStr("Average: ");
      avgStr.concat(avg);
      Utils::publish(avgStr);
      oledWrapper->display(String(avg), 0, 1);
      delay(5000);
    }
    void display_on_oled() {
      if (gatheringData) {
        gatherValues();
        gatherValues();
        gatheringData = false;
      } else {
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
    }
  public:
    void setup() {
      Serial.begin(115200);
      Utils::publish("setup() : started.");
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
      oledWrapper->startup();
      String s[2] = { config.build, config.build.substring(27, 31) };
      oledWrapper->display(s, 2);
      delay(3000);
      oledWrapper->clear();
    } 
    void test() {
      oledWrapper->test();
      Utils::publish("Waiting for '.'");
      while (Utils::waitForSerial(".")) {}
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
    } else if (command.equals("test")) {
      app.test();
    } else if (command.equals("showBuild")) {
      app.showBuild();
    } else if (command.equals("testSpinner")) {
      app.testSpinner();
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
