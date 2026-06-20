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
#define USE_GFX
#ifdef USE_GFX
#include "Arduino_GigaDisplay_GFX.h"
const int COLOR_RED = 0xF800;

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
      display_.setCursor(x, y);
      display_.setTextSize(textSize);
      display_.print(s);
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
    void test1() {
      clear();
      for (int r = 0; r < 2; r++) {
        display_.setRotation(r);
        String s("r: ");
        s.concat(r);
        s.concat(" w: ");
        s.concat(getWidth());
        s.concat(" h: ");
        s.concat(getHeight());
        display(s, 3, 0, 0);
        if (r == 0) {
          display_.drawLine(0, 0, getWidth(), getHeight(), COLOR_WHITE);
        } else {
          display_.drawLine(0, 0, getWidth(), getHeight(), COLOR_RED);
        }
      }
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
    void handleTimer() {
    }
};
#else
#include "Arduino_H7_Video.h"

#include "lvgl.h"
#include "/home/ck/Arduino/libraries/lvgl/src/misc/lv_color.h" // why can't Visual Studio find this on its own?

#define WIDTH     800
#define HEIGHT    480
Arduino_H7_Video  Display(WIDTH, HEIGHT, GigaDisplayShield);

class OLEDWrapper {
  private:
    lv_obj_t*   gridCell = nullptr;
    lv_obj_t*   screen = nullptr;
    const int   DEFAULT_FONT_SIZE = 24;
    lv_style_t    black; // create and delete lines objects instead, e.g., lv_obj_del(my_line_object);
    lv_style_t    white;
    int         currentColor;
  public:
    void startup() {
      delay(3000);
      Display.begin();
      screen = lv_obj_create(lv_scr_act());
      lv_obj_set_size(screen, Display.width(), Display.height());
      setupGrid();
      setupLineStyle(&black, 2, lv_color_black());
      setupLineStyle(&white, 4, lv_color_white());
    }
    void displayOff() {
      pinMode(74, OUTPUT);
      digitalWrite(74, LOW);
    }
    void displayOn() {
      pinMode(74, OUTPUT);
      digitalWrite(74, HIGH);
    }
    int getWidth() {
      return Display.width();
    }
    int getHeight() {
      return Display.height();
    }
    void fillRect(int x0, int y0, int x1, int y1, int color) {
      // no op
    }
    void setupLineStyle(lv_style_t *line_style, int width, lv_color_t color) {
      lv_style_init(line_style);
      lv_style_set_line_width(line_style, width);
      lv_style_set_line_color(line_style, color);
      lv_style_set_line_rounded(line_style, true);
    }
    void setupGrid() {
      static lv_coord_t col_dsc[] = { WIDTH - 50, LV_GRID_TEMPLATE_LAST };
      static lv_coord_t row_dsc[] = { HEIGHT - 50, LV_GRID_TEMPLATE_LAST };

      lv_obj_t* grid = lv_obj_create(lv_scr_act());
      lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
      lv_obj_set_size(grid, Display.width(), Display.height());

      gridCell = lv_label_create(grid);
      lv_obj_set_grid_cell(gridCell, LV_GRID_ALIGN_STRETCH, 0, 1,  //column
                          LV_GRID_ALIGN_STRETCH, 0, 1);      //row
      lv_obj_set_style_text_font(gridCell, &lv_font_montserrat_28, 0);
    }
    void display(String s, int textSize, uint8_t x, uint8_t y) {
      lv_label_set_text(gridCell, s.c_str());
    }
    void display(String s) {
      display(s, DEFAULT_FONT_SIZE, 10, 10);
    }
    void setDrawColor(int color) {
      currentColor = color;
    }
    void drawLines(lv_point_precise_t line_points[], int nPoints, lv_style_t *line_style) {
      /*Create a line and apply the new style*/
      lv_obj_t * line1;
      line1 = lv_line_create(lv_scr_act());
      lv_line_set_points(line1, line_points, nPoints);
      lv_obj_add_style(line1, line_style, 0);
      lv_obj_center(line1);
    }
    void drawLine(int x0, int y0, int x1, int y1) {
      static lv_point_precise_t line_points[] = { {x0, y0}, {x1, y1} };
      if (currentColor == COLOR_BLACK) {
        drawLines(line_points, 2, &black);
      } else {
        drawLines(line_points, 2, &white);
      }
    }
    void handleTimer() {
      lv_timer_handler();
      delay(5);
    }
    void dump() {
    }
    void clear() {
    }
    void display(arduino::String [2], int) {
    }
    void display(arduino::String [2], int, uint8_t, uint8_t) {
    }
};
#endif

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

    void drawElapsed() {
      unsigned long elapsed = millis() - msWhenOn;
      String s = Utils::msToString(elapsed);
      oledWrapper->fillRect(0, 0, 145, oledWrapper->getHeight(), COLOR_BLACK);
      oledWrapper->display(s, 3, 0, baseline);
      if (millis() - lastShift > 1000 * 10) {
        baseline += 3;
        if (baseline > oledWrapper->getHeight() - 20) {
          baseline = 0;
        }
        lastShift = millis();
      }
    }
  public:
    Spinner(int incrementDegrees) {
      this->incrementDegrees = incrementDegrees;
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
    const String build = "Thu May 21 05:43:26 PM PDT 2026";
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
      oledWrapper->handleTimer();
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