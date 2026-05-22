
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

#include "Arduino_GigaDisplay_GFX.h"
const int COLOR_WHITE = 0x65535;
const int COLOR_BLACK = 0x0;

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
    void display(String s, int textSize, uint8_t x, uint8_t y) {
      display_.setCursor(x,y); //x,y
      display_.setTextSize(textSize); //adjust text size
      display_.print(s); //print
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
    void drawLine(int x0, int y0, int x1, int y1) {
      // Rotate not happening automatically?
      display_.drawLine(y0, x0, y1, x1, currentColor);
    }
    int getHeight() {
      return display_.height();
    }
    int getWidth() {
      return display_.width();
    }
};
OLEDWrapper* oledWrapper = nullptr;

class Spinner {
  private:
    int middleX = oledWrapper->getWidth() / 2;
    int middleY = oledWrapper->getHeight() / 2;
    int lineLength = min(middleX, middleY);
    int color = COLOR_WHITE;
    int deg = 0;
    int incrementDegrees = 1;
    unsigned long lastDisplayTime = 0;
    const unsigned long DISPLAY_INTERVAL = 200; // milliseconds

  public:
    Spinner(int incrementDegrees) {
      this->incrementDegrees = incrementDegrees;
    }
    void reset() {
      deg = 0;
      color = COLOR_WHITE;
    }
    void display() {
      if (millis() - lastDisplayTime < DISPLAY_INTERVAL) {
        return;
      }
      int xEnd = lineLength * cos(deg * M_PI / 180.0);
      int yEnd = lineLength * sin(deg * M_PI / 180.0);

      oledWrapper->setDrawColor(color);
      oledWrapper->drawLine(middleX, middleY, middleX + xEnd, middleY + yEnd);
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
      Utils::publish("Spinner");
      String s("middleX: ");
      s.concat(middleX);
      Utils::publish(s);
      s.remove(0);
      s.concat("middleY: ");
      s.concat(middleY);
      Utils::publish(s);
      s.remove(0);
      s.concat("lineLength: ");
      s.concat(lineLength);
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
    const String build = "Thu May 21 05:43:26 PM PDT 2026";
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
        oledWrapper->display(String(totalSeconds - i));
        Utils::publish(String(value)); 
        delay(1000);
        total += value;
        lightSensor1.clear();
      }
      int avg = total / totalSeconds;
      String avgStr("Average: ");
      avgStr.concat(avg);
      Utils::publish(avgStr);
      oledWrapper->display(String(avg));
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
      oledWrapper = new OLEDWrapper();
      oledWrapper->startup();
      showBuild();
      config.dump();
      Utils::publish("setup() : finished.");
//      server_setup();
    }
    void loop() {
      display_on_oled();
//      server_loop();
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
