#include <arduino.h>

#include "my_variables.h"
#include "my_eeprom.h"
#include "my_adc_interrupt.h"


byte threshold_dec_pin = 3;
byte threshold_inc_pin = 4;
byte recalibrate_pin   = 5;
bool threshold_dec_pin_state0 = HIGH;
bool threshold_inc_pin_state0 = HIGH;
bool recalibrate_pin_state0   = HIGH;
long button_pressed_t0 = 0;

void buttons_init()
{
  pinMode(recalibrate_pin, INPUT_PULLUP);
  pinMode(threshold_inc_pin, INPUT_PULLUP);
  pinMode(threshold_dec_pin, INPUT_PULLUP);
}

void button_pressed()
{
  bool threshold_dec_pin_state1 = digitalRead(threshold_dec_pin);
  bool threshold_inc_pin_state1 = digitalRead(threshold_inc_pin);
  bool recalibrate_pin_state1   = digitalRead(recalibrate_pin);
  long button_pressed_t1 = micros();
  if( threshold_dec_pin_state0==HIGH && 
      threshold_dec_pin_state1==LOW &&
      button_pressed_t1 - button_pressed_t0 > 10000
  ){
    disable_adc_interrupt();
      threshold = float(threshold)*.9;
      Serial.print("threshold- [");
      Serial.print(threshold);
      Serial.print("/");
      Serial.print(65535);
      Serial.println("]");
      save_eeprom();
    enable_adc_interrupt();
    button_pressed_t0 = button_pressed_t1;
  }
  if( threshold_inc_pin_state0==HIGH && 
      threshold_inc_pin_state1==LOW &&
      button_pressed_t1 - button_pressed_t0 > 10000
  ){
    disable_adc_interrupt();
      threshold = float(threshold)/.9;
      Serial.print("threshold+ [");
      Serial.print(threshold);
      Serial.print("/");
      Serial.print(65535);
      Serial.println("]");
      save_eeprom();
      button_pressed_t0 = button_pressed_t1;
    enable_adc_interrupt();
  }
  if( recalibrate_pin_state0==HIGH && 
      recalibrate_pin_state1==LOW &&
      button_pressed_t1 - button_pressed_t0 > 10000
  ){
    prepare_calibration();
    button_pressed_t0 = button_pressed_t1;
  }
  threshold_dec_pin_state0 = threshold_dec_pin_state1;
  threshold_inc_pin_state0 = threshold_inc_pin_state1;
  recalibrate_pin_state0   = recalibrate_pin_state1;
}
