
#include <U8g2lib.h>

class Utils {
  private:
    static unsigned long lastPrintln;
  public:
    static void publish(String s) {
      if (millis() < lastPrintln + 1000) {
        delay(1000);
      }
      Serial.println(s);
    }
};
unsigned long Utils::lastPrintln = 0;

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
      u8g2.setDrawColor(1);
      u8g2.setFontDirection(0);
    }
    void drawInt(int val) {
      u8g2_prepare();
      u8g2.clearBuffer();
      u8g2.drawUTF8(2, this->baseLine, String(val).c_str());
      u8g2.setFont(u8g2_font_fur11_tf);
      u8g2.drawUTF8(6, this->baseLine + 20, "Fahrenheit");
      u8g2.sendBuffer();
    }
    void clear() {
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
};
OLEDWrapper* oledWrapper = new OLEDWrapper();

class Spinner {
  private:
    int middleX = u8g2.getWidth() / 2;
    int middleY = u8g2.getHeight() / 2;
    int lineWidth = min(middleX, middleY);
    int color = COLOR_WHITE;
    int deg = 0;

  public:
    void display() {
      int xEnd = lineWidth * cos(deg * M_PI / 180.0);
      int yEnd = lineWidth * sin(deg * M_PI / 180.0);

      u8g2.setDrawColor(color);
      u8g2.drawLine(middleX, middleY, middleX + xEnd, middleY + yEnd);
      u8g2.sendBuffer();
//      u8g2.display();
      deg++;
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
Spinner spinner;

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
      if (millis() > lastPublish + 2000) {
        String s("Sensor value: ");
        s.concat(getValue());
        Utils::publish(s);
        lastPublish = millis();
      }
    }
    int getValue() {
        return round(total / nSamples);
    }
};
Sensor lightSensor1(A0, "Arduino light sensor");

class Config {
  public:
    void dump() {
      String s("gitHubRepository: https://github.com/chrisxkeith/arduino-light-sensor");
      Utils::publish(s);
      s.remove(0);
      s.concat("u8g2.getWidth(): ");
      s.concat(String(u8g2.getWidth()));
      Utils::publish(s);
      s.remove(0);
      s.concat("u8g2.getHeight(): ");
      s.concat(String(u8g2.getHeight()));
      Utils::publish(s);
      s.remove(0);
      s.concat("build: ");
      s.concat("~ Fri Nov 21 04:52:21 PM PST 2025");
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
      config.dump();
      Utils::publish("setup() : finished.");
    }
    void loop() {
      display_on_oled();
    }
};
App app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
