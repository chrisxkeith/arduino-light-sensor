
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

/*
// If not defined, assumes QWIIC-cabled OLED.
// #define USE_OLED_SHIELD

#ifdef USE_OLED_SHIELD
#include <SparkFunMicroOLED.h>
#else
#include <SFE_MicroOLED.h>
#endif
#include <math.h>

class OLEDWrapper {
  public:
#ifdef USE_OLED_SHIELD
    MicroOLED* oled = new MicroOLED();
#else
    MicroOLED* oled = new MicroOLED(MODE_I2C, 9, 1, CS_DEFAULT);
#endif

    OLEDWrapper() {
        oled->begin();    // Initialize the OLED
        oled->clear(ALL); // Clear the display's internal memory
        oled->display();  // Display what's in the buffer (splashscreen)
        delay(1000);     // Delay 1000 ms
        oled->clear(PAGE); // Clear the buffer.
    }

    void display(String title, int font, uint8_t x, uint8_t y) {
        oled->clear(PAGE);
        oled->setFontType(font);
        oled->setCursor(x, y);
        oled->print(title);
        oled->display();
    }

    void display(String title, int font) {
        display(title, font, 0, 0);
    }

    void invert(bool invert) {
      oled->invert(invert);
    }

    void displayNumber(String s) {
        // To reduce OLED burn-in, shift the digits (if possible) on the odd minutes.
        int x = 0;
        if (Time.minute() % 2) {
            const int MAX_DIGITS = 5;
            if (s.length() < MAX_DIGITS) {
                const int FONT_WIDTH = 12;
                x += FONT_WIDTH * (MAX_DIGITS - s.length());
            }
        }
        display(s, 3, x, 0);
    }
    void clear() {
      oled->clear(ALL);
    }
};
OLEDWrapper oledWrapper;

class Spinner {
  private:
    int middleX = oledWrapper.oled->getLCDWidth() / 2;
    int middleY = oledWrapper.oled->getLCDHeight() / 2;
    int xEnd, yEnd;
    int lineWidth = min(middleX, middleY);
    int color;
    int deg;

  public:
    Spinner() {
      color = WHITE;
      deg = 0;
    }

    void display() {
      xEnd = lineWidth * cos(deg * M_PI / 180.0);
      yEnd = lineWidth * sin(deg * M_PI / 180.0);

      oledWrapper.oled->line(middleX, middleY, middleX + xEnd, middleY + yEnd, color, NORM);
      oledWrapper.oled->display();
      deg++;
      if (deg >= 360) {
        deg = 0;
        if (color == WHITE) {
          color = BLACK;
        } else {
          color = WHITE;
        }
      }
    }
};
Spinner spinner;
*/
class Sensor {
  private:
    int     pin;
    String  name;
    int     nSamples;
    double  total;

    int getValue() {
        return round(total / nSamples);
    }

  public:
    bool  saving = false;
    
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
  delay(2000);
}
 
