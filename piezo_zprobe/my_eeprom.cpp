#include "my_variables.h"

#ifndef USE_EEPROM
  bool check_eeprom(){ return false; }
  bool load_eeprom(){ return false; }
  void save_eeprom(){}
#else
  #include <EEPROM.h>
  bool check_eeprom()
  {
    word id;
    int addr = 0;
    EEPROM.get(addr, id);
    addr+=sizeof(id);
    return id=='ab';
  }
  bool load_eeprom()
  {
    word id;
    int addr = 0;
    EEPROM.get(addr, id);
    addr+=sizeof(id);
    if(id!='ab')
      return false;
      
    if(!v1mean_init)
    {
      EEPROM.get(addr, v1mean);
      v1mean_init = true;
    }
    addr+=sizeof(v1mean);
    
    if(!finit)
    {
      EEPROM.get(addr, freq);
      finit = true;
    }
    addr+=sizeof(freq);
  
    if(!tinit)
    {
      EEPROM.get(addr, threshold);
      addr+=sizeof(threshold);
      tinit = true;
    }
    
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
#endif
