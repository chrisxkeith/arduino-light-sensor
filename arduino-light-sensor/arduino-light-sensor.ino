
const String gitHubRep = "https://github.com/chrisxkeith/arduino-light-sensor";
const String gitHubCommitHash = "to be filled in after push";

int publishRateInSeconds = 1;
unsigned long lastPublishInSeconds = 0;

#include <EEPROM.h>
class EEPROM_helper {
  private:
    int   nextIntLocation = 0;
  public:
    EEPROM_helper() {
    }
    void init() {
      nextIntLocation = 0;
      for (int i = 0 ; i < EEPROM.length(); i++) {
        EEPROM.write(i, 255);
      }
    }
    void report() {
      String s("EEPROM.length() : ");
      s.concat(EEPROM.length());  
      Serial.println(s);  
    }
    bool writeInt(int val) {
      if (nextIntLocation < EEPROM.length()) {
        EEPROM.put(nextIntLocation, val);
        nextIntLocation += sizeof(int);
        return true;
      }
      Serial.println("Reached end of EEPROM");
      return false;
    }
    void dump() {
      int val = 0;
      for (int i = 0; i < EEPROM.length() && val >= 0; i += sizeof(int)) {
        EEPROM.get(i, val);
        String s(i);
        s.concat(",");
        s.concat(val);
        Serial.println(s);
      }
    }
};
EEPROM_helper eeprom_helper;

class LED {
  public:
    LED() {
      pinMode(8, OUTPUT);
    }
    void on() {
      digitalWrite(8, HIGH);
    }
    void off() {
      digitalWrite(8, LOW);
    }
};
LED led;

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
      if (saving) {
        if (!eeprom_helper.writeInt(getValue())) {
          saving = false;
          led.off();
        }
      }
    }
};

Sensor lightSensor1(A0, "Arduino light sensor");

class Button {
  public:
    Button() {
      pinMode(2, INPUT);
    }
    int read() {
      return digitalRead(2);
    }
};
Button button;

void publish() {
  lastPublishInSeconds = millis() / 1000;
  lightSensor1.publishData();
  lightSensor1.clear();
}

void setup() {
  Serial.begin(115200);
  eeprom_helper.report();
  eeprom_helper.dump();
  Serial.println("setup() : finished.");
}

void loop() {
  lightSensor1.sample();
  if (button.read() == 0) {
    if (lightSensor1.saving) {
      eeprom_helper.writeInt(-1);
      lightSensor1.saving = false;
      led.off();
    } else {
      eeprom_helper.init();
      lightSensor1.saving = true;
      led.on();
    }
  }
  if ((lastPublishInSeconds + publishRateInSeconds) <= (millis() / 1000)) {
    publish();
  }
}
