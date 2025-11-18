
#include <U8g2lib.h>

U8G2_SSD1327_EA_W128128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

class LargeOLEDWrapper {
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
    void setup_OLED() {
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

#include <SparkFun_Qwiic_OLED.h>
#include <res/qw_fnt_5x7.h>       // &QW_FONT_5X7
#include <res/qw_fnt_8x16.h>      // &QW_FONT_8X16
#include <res/qw_fnt_7segment.h>  // &QW_FONT_7SEGMENT
#include <res/qw_fnt_31x48.h>     // &QW_FONT_31X48
#include <res/qw_fnt_largenum.h>  // &QW_FONT_LARGENUM
#include <math.h>
#include <Wire.h>

class OLEDWrapper {
  public:
    QwiicMicroOLED* oled = new QwiicMicroOLED();

    bool startup() {
        if (oled->begin() == false) {
          Serial.println("oled->begin() failed. Switching...");
          return false;
        }
        oled->erase(); // Clear the display's internal memory
        oled->display();
        return true;
    }

    void display(String title, uint8_t x, uint8_t y) {
        oled->erase();
        oled->setFont(&QW_FONT_LARGENUM);
        oled->text(x, y, title);
        oled->display();
    }
    void clear() {
        oled->erase();
        oled->display();
    }
};
OLEDWrapper* oledWrapper = new OLEDWrapper();

class Spinner {
  private:
    int middleX = oledWrapper->oled->getWidth() / 2;
    int middleY = oledWrapper->oled->getHeight() / 2;
    int lineWidth = min(middleX, middleY);
    int color = COLOR_WHITE;
    int deg = 0;

  public:
    void display() {
      int xEnd = lineWidth * cos(deg * M_PI / 180.0);
      int yEnd = lineWidth * sin(deg * M_PI / 180.0);

      oledWrapper->oled->line(middleX, middleY, middleX + xEnd, middleY + yEnd, color);
      oledWrapper->oled->display();
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
      Serial.println(String(getValue()));
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
      Serial.println(s);
      s.remove(0);
      s.concat("oledWrapper->oled->getWidth(): ");
      s.concat(String(oledWrapper->oled->getWidth()));
      Serial.println(s);
      s.remove(0);
      s.concat("oledWrapper->oled->getHeight(): ");
      s.concat(String(oledWrapper->oled->getHeight()));
      Serial.println(s);
      s.remove(0);
      s.concat("build: ");
      s.concat("~ Sun, Aug 25, 2024  1:06:41 PM");
      Serial.println(s);
      s.remove(0);
      s.concat("THRESHOLD: ");
      s.concat(String(lightSensor1.THRESHOLD));
      Serial.println(s);
    }
};
Config config;

class App {
  private:
    bool gatheringData = true;
    void gatherValues() {
      int totalSeconds = 10;
      int total = 0;
      for (int i = 0; i < totalSeconds; i++) {
        lightSensor1.sample();
        int value = lightSensor1.getValue();
        oledWrapper->display(String(totalSeconds - i), 0, 1);
        Serial.println(String(value)); 
        delay(1000);
        total += value;
        lightSensor1.clear();
      }
      int avg = total / totalSeconds;
      String avgStr("Average: ");
      avgStr.concat(avg);
      Serial.println(avgStr);
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
        lightSensor1.clear();
      }
    }

  public:
    void setup() {
      Serial.begin(115200);
      config.dump();
      Wire.begin();
      if (!oledWrapper->startup()) {
        // oledWrapper = new LargeOLEDWrapper();
        Serial.println("Temporarily freezing...");
        while (true) { ; }
      }
      Serial.println("setup() : finished.");
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
