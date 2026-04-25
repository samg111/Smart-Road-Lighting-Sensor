#include "WS_DALI.h"

Dali dali;
uint8_t DALI_Addr[64] = {0};
uint8_t DALI_NUM = 0;

uint8_t bus_is_high() {
  return digitalRead(RX_PIN); //slow version
}

//use bus
void bus_set_low() {
  digitalWrite(TX_PIN,LOW); //opto slow version
}

//release bus
void bus_set_high() {
  digitalWrite(TX_PIN,HIGH); //opto slow version
}

void ARDUINO_ISR_ATTR onTimer() {
  dali.timer();
}

hw_timer_t *timer = NULL;
void DALI_Init() {
  printf("This is Wavshare's DALI board \r\n");
  //setup RX/TX pin
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  
  timer = timerBegin(9600000);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000, true, 0);

  dali.begin(bus_is_high, bus_set_high, bus_set_low);
  Scan_DALI_addr_ALL();
  if(DALI_NUM == 0)
    Assign_new_address_ALL();
}

void Blinking_ALL() {                                                        
  printf("Running: Blinking all lamps\r\n");
  dali.set_level(254);
  delay(500);
  dali.set_level(0);
  delay(500);
}
void Luminaire_Brightness(uint8_t Light, uint8_t addr) {                     
  printf("Running: Set the brightness of the fixture at address %d to %d %%\r\n", addr, Light);
  uint8_t Light_practical = (uint8_t)(2.55*Light);
  dali.set_level(Light_practical,addr);
}

void Lighten_ALL() {                                                          
  printf("Running: Turn on all lights on the DALI\r\n");
  dali.set_level(200);
}
void Extinguish_ALL() {                                                      
  printf("Running: Turn off all lights on the DALI\r\n");
  dali.set_level(0);
}

void Scan_DALI_addr_ALL() {                                                  
  printf("Running: Scan all addresses\r\n");
  uint8_t addr;
  for(addr = 0; addr<64; addr++) {
    int16_t rv = dali.cmd(DALI_QUERY_STATUS,addr);
    if(rv>=0) {
      DALI_Addr[DALI_NUM] = addr;
      DALI_NUM ++;
      printf("Address %d  status=0x%x  minLevel= %d \r\n", addr, rv, dali.cmd(DALI_QUERY_MIN_LEVEL,addr));
      dali.set_level(254,addr);
      delay(500);
      dali.set_level(0,addr);
    }
    else if (-rv != DALI_RESULT_NO_REPLY) {
      printf("short address= %d ERROR= %d  \r\n", addr,-rv);
    }
  }  
  printf("End scan,%d devices were scanned\r\n",DALI_NUM);
}
void Delete_DALI_addr_ALL() {                                                           
  printf("Running: Delete all short addresses\r\n");
  //remove all short addresses
  dali.cmd(DALI_DATA_TRANSFER_REGISTER0,0xFF);
  dali.cmd(DALI_SET_SHORT_ADDRESS, 0xFF);
  printf("DONE delete \r\n");
}

void Assign_new_address_ALL(){                                                  
  printf("Running: Assign new addresses to all devices\r\n");   
  printf("Might need a couple of runs to find all lamps ...\r\n");
  printf("Be patient, this takes a while ...\r\n");
  uint8_t cnt = dali.commission(0xff); //init_arg=0b11111111 : all without short address  
  printf("DONE, assigned %d new short addresses\r\n",cnt);
  Scan_DALI_addr_ALL();
}
