#include <EEPROM.h>

#include "my_variables.h"

bool check_eeprom()
{
  word id;
  int addr = 0;
  EEPROM.get(addr, id);        addr+=sizeof(id);
  return id=='ab';
}
bool load_eeprom()
{
  word id;
  int addr = 0;
  EEPROM.get(addr, id);        addr+=sizeof(id);
  if(id!='ab')
    return false;
  EEPROM.get(addr, v1mean);    addr+=sizeof(v1mean);
  v1mean_init = true;
  EEPROM.get(addr, freq);      addr+=sizeof(freq);
  finit = true;
  EEPROM.get(addr, threshold); addr+=sizeof(threshold);
  tinit = true;
  // Serial.println("load_eeprom()");
  return true;
}
void save_eeprom()
{
  // Serial.println("save_eeprom()");
  word id = 'ab';
  int addr = 0;
  EEPROM.put(addr, id);        addr+=sizeof(id);
  EEPROM.put(addr, v1mean);    addr+=sizeof(v1mean);
  EEPROM.put(addr, freq);      addr+=sizeof(freq);
  EEPROM.put(addr, threshold); addr+=sizeof(threshold);
}
