#include "devices/tca9548a.h"

TCA9548A::TCA9548A(int dev, int bus, int address) : I2C(dev,bus,address){
     this->current_channel = 0;
     I2C::_handle = attachPeripheral(I2C_PI, bus, address);
     // this->_handle = attachPeripheral(I2C_PI, bus, address);
}

TCA9548A::~TCA9548A(){
     this->close();
}

int TCA9548A::start(){
     return 0;
}

int TCA9548A::close(){
     // return I2C::close();
     return this->_close();
}

int TCA9548A::select_channel(uint8_t channel){
     if(channel > 7) return -1;
     uint8_t value = (0x01 << channel);
     printf("Writing Value 0x%02X to i2c mux\r\n",value);
     int err = this->write_raw_byte(value);
     if(err < 0) return -1;
     else{
          this->current_channel = channel;
          return 0;
     }
}


void TCA9548A::scan_bus(I2C* targetDev, bool verbose){
     int err;
     for(int ch = 0; ch < 8; ch++){
          this->select_channel(ch);
          for(int ad = 0; ad < 128; ad++){
               if(ad == this->_address) continue;
               // err = this->write_byte(ad,0);
               err = targetDev->write_raw_byte(0);
               if(err < 0){
                    // printf("[ERROR] TCA9548A::scan_bus() ---- Could not write to address [0x%02X] on channel \'%d\' with error code = %d\r\n", ad, ch, err);
               }else{
                    if(ad == targetDev->get_address())
                         printf("[INFO] TCA9548A::scan_bus() ---- Found Target device [0x%02X] at address 0x%02X on Channel %d\r\n",targetDev->get_address(), this->_address, ch);
               }
          }
     }
}
