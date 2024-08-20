
#include <U8g2lib.h>

class Config {
  public:
    const String gitHubRepository = "https://github.com/chrisxkeith/arduino-light-sensor";

    void dump() {
      String s("gitHubRepository : ");
      s.concat(gitHubRepository);
      Serial.println(s);
    }
};
Config config;

#include <SparkFun_Qwiic_OLED.h>
#include <res/qw_fnt_5x7.h>
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
        oled->setFont(&QW_FONT_5X7);
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
    const int THRESHOLD = 175;
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

    void publish() {
      publishData();
      clear();
    }
    int getValue() {
        return round(total / nSamples);
    }
};
Sensor lightSensor1(A0, "Arduino light sensor");

class App {
  private:
    void display_on_oled() {
      int value = lightSensor1.getValue();
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
    }
};
App app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}
 
