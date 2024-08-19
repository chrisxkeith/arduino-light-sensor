
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
}
 
