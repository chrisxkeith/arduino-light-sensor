
#include <U8g2lib.h>

#include <SparkFun_Qwiic_OLED.h>
#include <res/qw_fnt_5x7.h>       // &QW_FONT_5X7
#include <res/qw_fnt_8x16.h>      // &QW_FONT_8X16
#include <res/qw_fnt_7segment.h>  // &QW_FONT_7SEGMENT
#include <res/qw_fnt_31x48.h>     // &QW_FONT_31X48
#include <res/qw_fnt_largenum.h>  // &QW_FONT_LARGENUM
#include <math.h>
#include <wire.h>

class OLEDWrapper {
  public:
    QwiicMicroOLED* oled = new QwiicMicroOLED();

    void startup() {
        if (oled->begin() == false) {
          Serial.println("oled->begin() failed. Freezing...");
          while (true)
            ;
        }
        oled->erase(); // Clear the display's internal memory
        oled->display();
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
OLEDWrapper oledWrapper;

class Spinner {
  private:
    int middleX = oledWrapper.oled->getWidth() / 2;
    int middleY = oledWrapper.oled->getHeight() / 2;
    int lineWidth = min(middleX, middleY);
    int color = COLOR_WHITE;
    int deg = 0;

  public:
    void display() {
      int xEnd = lineWidth * cos(deg * M_PI / 180.0);
      int yEnd = lineWidth * sin(deg * M_PI / 180.0);

      oledWrapper.oled->line(middleX, middleY, middleX + xEnd, middleY + yEnd, color);
      oledWrapper.oled->display();
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
    const int THRESHOLD = 20;
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
    const String gitHubRepository = "https://github.com/chrisxkeith/arduino-light-sensor";

    void dump() {
      String s("gitHubRepository: ");
      s.concat(gitHubRepository);
      Serial.println(s);
      s.remove(0);
      s.concat("oledWrapper.oled->getWidth(): ");
      s.concat(String(oledWrapper.oled->getWidth()));
      Serial.println(s);
      s.remove(0);
      s.concat("oledWrapper.oled->getHeight(): ");
      s.concat(String(oledWrapper.oled->getHeight()));
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
    void display_on_oled() {
      int value = lightSensor1.getValue();
      String s(value);
      Serial.println(s);
      if (millis() < 30 * 1000) { // show values for first 30 seconds to help calibration
        oledWrapper.display(s, 0, 1); 
        delay(1000);
      } else {
        if ((value > lightSensor1.THRESHOLD) != lightSensor1.on) {
          lightSensor1.on = !lightSensor1.on;
          oledWrapper.clear();
          if (lightSensor1.on) {
            spinner.display();
          }
        } else {
          if (lightSensor1.on) {
            spinner.display();
          }
        }
      }
    }

  public:
    void setup() {
      Serial.begin(115200);
      config.dump();
      Wire.begin();
      oledWrapper.startup();
      Serial.println("setup() : finished.");
    }
    void loop() {
      lightSensor1.sample();
      display_on_oled();
      lightSensor1.clear();
    }
};
App app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
 
