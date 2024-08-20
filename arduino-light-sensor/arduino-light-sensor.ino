
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

class OLEDWrapper {
  public:
    QwiicMicroOLED* oled = new QwiicMicroOLED();

    OLEDWrapper() {
        oled->begin();    // Initialize the OLED
        oled->erase(); // Clear the display's internal memory
        oled->display();  // Display what's in the buffer (splashscreen)
        delay(1000);     // Delay 1000 ms
        oled->erase(); // Clear the buffer.
    }

    void display(String title, uint8_t x, uint8_t y) {
        oled->erase();
        oled->setFont(&QW_FONT_5X7);
        oled->text(x, y, title);
        oled->display();
    }
};
OLEDWrapper oledWrapper;

class Spinner {
  private:
    int middleX = oledWrapper.oled->getWidth() / 2;
    int middleY = oledWrapper.oled->getHeight() / 2;
    int xEnd, yEnd;
    int lineWidth = min(middleX, middleY);
    int color;
    int deg;

  public:
    Spinner() {
      color = COLOR_WHITE;
      deg = 0;
    }

    void display() {
      xEnd = lineWidth * cos(deg * M_PI / 180.0);
      yEnd = lineWidth * sin(deg * M_PI / 180.0);

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

void setup() {
  Serial.begin(115200);
  config.dump();
  Serial.println("setup() : finished.");
}

void loop() {
  lightSensor1.sample();
  lightSensor1.publish();
  // oledWrapper.display(String(lightSensor1.getValue()), 0, 0);
  delay(2000);
}
 
