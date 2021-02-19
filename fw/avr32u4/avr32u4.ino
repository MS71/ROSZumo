#include <avr/sleep.h>
#include <Zumo32U4.h>
#include <Wire.h>
#include <avr/wdt.h>

#define CMD_BEEP                0x01
#define CMD_BATLEVEL            0x02
#define CMD_MOTORS_SET_SPEED    0x03
#define CMD_ENCODERS            0x04
#define CMD_LIDAR_SET_PWM       0x05
#define CMD_GET_STATUS          0x06
#define CMD_SET_LED             0x07
#define CMD_PUTCHAR             0x08
#define CMD_WATCHDOG            0x09

#undef ENABLE_WDT
#define ENABLE_SLEEP
#define ENABLE_LINE_SENSOR
#define ENABLE_PROX_SENSOR
Zumo32U4ButtonA           buttonA;
Zumo32U4ButtonB           buttonB;
Zumo32U4ButtonC           buttonC;
Zumo32U4Buzzer            buzzer;
Zumo32U4Motors            motors;
Zumo32U4Encoders          encoders;
#ifdef ENABLE_LINE_SENSOR
Zumo32U4LineSensors       lineSensors;
#endif
#ifdef ENABLE_PROX_SENSOR
Zumo32U4ProximitySensors  proxSensors;
#endif
#define NUM_SENSORS 5
uint16_t lineSensorValues[NUM_SENSORS];
bool useEmitters = true;

uint8_t proxSensorsValues[6] = {0};

volatile uint32_t loopcnt = 0;

volatile uint16_t bat = 0;

volatile int16_t enc_l = 0;
volatile int16_t enc_r = 0;

uint16_t buzzer_freq = 0;
uint16_t buzzer_duration = 0;
uint8_t buzzer_volume = 0;

uint8_t enable_prox_sensor = 0;
uint8_t enable_line_sensor = 0;

int16_t motor_l_speed = 0;
int16_t motor_r_speed = 0;
uint8_t motor_speed_flag = 0;

void setPwmDutyA(int val) {
  TC4H = val >> 8;
  OCR4A = 0xFF & val;
}

#if 0
uint8_t xxx = 0;
ISR(WDT_vect)
{
  if(xxx == 0 )
  {
    xxx = 1;
    ledYellow(1);
  }
  else
  {
    xxx = 0;
    ledYellow(0);
  }
}
#endif

#if 0
ISR(TIMER0_COMPA_vect) 
{
  xxx++;
  if(xxx < 128 )
  {
    ledYellow(1);
  }
  else
  {
    ledYellow(0);
  }
}
#endif

void setup()
{
  ledGreen(0);  
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial.write("ZUMO: starting ...\n");
  pinMode(13, OUTPUT);
  digitalWrite(13, 0);

  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);

#ifdef ENABLE_SENSORS
  lineSensors.initFiveSensors();
#endif

  //buzzer.playFrequency(55, 250, 8);
  //while (buzzer.isPlaying());

#ifdef USE_RPISLAVELIB
#else
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
  Wire.onReceive(receiveEvent); // register event
#endif

#if 1
  PLLFRQ = (PLLFRQ & 0xCF) | 0x30;
#else
// Configure PLL.
  PLLCSR  = (1 << PINDIV);  // set if using 16MHz clock.
  PLLFRQ  = (1 << PLLUSB);  // divide PLL output by two.
  PLLFRQ |= (1 << PLLTM0);  // postscale by 1.
  PLLFRQ |= (10 << PDIV0);  // 96MHz output frequency.

  // Enable PLL.
  PLLCSR |= (1 << PLLE);    // enable PLL.
  
  // Wait for PLL lock.
  while (!(PLLCSR & (1 << PLOCK)));
#endif  

#if 1
  // 10-bit operation
  TC4H = 0x03; OCR4C = 0xFF;
  //Configuration of Timer 4 Registers, OC4A (D13) + 0C4B (D10)
  TCCR4A = (TCCR4A & 0x00111100) | 0b10000010;
  //Prescaler
  TCCR4B = (TCCR4B & 0b1110000) | 1;

  setPwmDutyA(0);
#endif

#ifdef ENABLE_LINE_SENSOR
  lineSensors.initThreeSensors();
  lineSensors.emittersOff();
#endif
#ifdef ENABLE_PROX_SENSOR
  proxSensors.pullupsOn();
  proxSensors.initThreeSensors();
  proxSensors.lineSensorEmittersOff();
#endif  
  //buzzer.playFrequency(1000, 50, 10);

  //buzzer.playFrequency(220, 50, 100);
  //while (buzzer.isPlaying());

  Serial.write("ZUMO: starting ... done\n");
  ledGreen(0);  
#ifdef ENABLE_WDT
  wdt_enable(WDTO_2S);
#endif
#if 0
    // setup of the WDT
    MCUSR &= ~(1 << WDRF); // remove reset flag
    WDTCSR |= (1 << WDCE) | (1 << WDE); // set WDCE, access prescaler
    WDTCSR = 1 << WDP0 | 1 << WDP1 | 1 << WDP2; // set prescaler bits to to 2s
    WDTCSR |= 1 << WDIE; // access WDT interrupt
#endif  

  //TIMSK0 |= _BV(OCIE0A);
  
  buzzer_freq = 440;
  buzzer_duration = 250;
  buzzer_volume = 10;
}

void loop()
{
  //ledRed((millis()%1000)<500);
  int16_t tmp_l = encoders.getCountsAndResetLeft();
  int16_t tmp_r = encoders.getCountsAndResetRight();
  uint16_t tmp_bat = readBatteryMillivolts();

  if( motor_speed_flag != 0 )
  {
    motor_speed_flag = 0;
    motors.setLeftSpeed(motor_l_speed);
    motors.setRightSpeed(motor_r_speed);
  }

#ifdef ENABLE_LINE_SENSOR
  if( enable_line_sensor == 1 )
  {
    lineSensors.read(lineSensorValues, useEmitters ? QTR_EMITTERS_ON : QTR_EMITTERS_OFF);
  }
  else
  {
    lineSensors.emittersOff();
  }
#endif
#ifdef ENABLE_PROX_SENSOR
  if( enable_prox_sensor == 1 )
  {
    proxSensors.read();
    proxSensorsValues[0] = proxSensors.countsLeftWithLeftLeds();
    proxSensorsValues[1] = proxSensors.countsLeftWithRightLeds();
    proxSensorsValues[2] = proxSensors.countsFrontWithLeftLeds();
    proxSensorsValues[3] = proxSensors.countsFrontWithRightLeds();
    proxSensorsValues[4] = proxSensors.countsRightWithLeftLeds();
    proxSensorsValues[5] = proxSensors.countsRightWithRightLeds();
  }
  else
  {
    proxSensors.lineSensorEmittersOff();    
  }
#endif
  cli();
  loopcnt++;
  bat = tmp_bat;
  enc_l += tmp_l;
  enc_r += tmp_r;
  sei();

  if( buzzer_freq != 0 )
  {
      buzzer.playFrequency(buzzer_freq, buzzer_duration, buzzer_volume);
      while (buzzer.isPlaying())
      {
         wdt_reset();
      }
      buzzer_freq = 0;
  }


#if 0
  // read from port 1, send to port 0:
  if (Serial1.available()) {
    int inByte = Serial1.read();
    Serial.write(inByte);
  }

  // read from port 0, send to port 1:
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte);
  }
#endif

#if 0
  if (buttonC.isPressed())
  {
    ledRed(1);
    while(1); 
  }
#endif  
  
#if 1
  ledRed(0);  
#ifdef ENABLE_SLEEP
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
#else
  set_sleep_mode(SLEEP_MODE_IDLE);
#endif  
  sleep_enable();
  sleep_mode();
  /** Das Programm läuft ab hier nach dem Aufwachen weiter. **/
  /** Es wird immer zuerst der Schlafmodus disabled.        **/
  sleep_disable();
  ledRed(1);  
#endif

#ifdef ENABLE_WDT
 wdt_reset();
#endif
}

#define MSG_UINT8(_p_)  (uint8_t)((((_p_)[1])<<0))
#define MSG_UINT16(_p_) (uint16_t)(((uint16_t)((_p_)[0])<<8)|((uint16_t)((_p_)[1])<<0))
#define MSG_UINT32(_p_) (uint32_t)(((uint32_t)((_p_)[0])<<24)|((uint32_t)((_p_)[1])<<16)|((uint32_t)((_p_)[2])<<8)|((uint32_t)((_p_)[3])<<0))

#define MSG_INT8(_p_)  (int8_t)((((_p_)[1])<<0))
#define MSG_INT16(_p_) (int16_t)(((int16_t)((_p_)[0])<<8)|((int16_t)((_p_)[1])<<0))
#define MSG_INT32(_p_) (int32_t)(((int32_t)((_p_)[0])<<24)|((int32_t)((_p_)[1])<<16)|((int32_t)((_p_)[2])<<8)|((int32_t)((_p_)[3])<<0))

uint8_t cmd = 0;

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  ledGreen(1);
  cmd = Wire.read();
  howMany--;
  if ( howMany > 0 )
  {
    const int L = 32;
    int n = 0;
    uint8_t msg[L];

    while (1 < Wire.available() && ((n + 1) < L))
    {
      // loop through all but the last
      msg[n++] = Wire.read(); // receive byte as a character
    }
    msg[n++] = Wire.read();    // receive byte as an integer

    switch (cmd)
    {
      case CMD_BEEP:
        if ( n >= (2 + 2 + 1) )
        {
          buzzer_freq = MSG_UINT16(&msg[0]);
          buzzer_duration = MSG_UINT16(&msg[2]);
          buzzer_volume = MSG_UINT8(&msg[2 + 2]);
        }
        break;
      case CMD_MOTORS_SET_SPEED:
        {
          if ( n >= (2 + 2) )
          {
            motor_l_speed = MSG_INT16(&msg[0]);
            motor_r_speed = MSG_INT16(&msg[2]);
            motor_speed_flag = 1;
          }
        }
        break;
      case CMD_LIDAR_SET_PWM:
        {
          uint16_t v = MSG_UINT16(&msg[0]);
          setPwmDutyA(v);
        }
        break;
      case CMD_SET_LED:
        {
            ledRed((MSG_UINT8(&msg[0])>>0)&1);
            ledYellow((MSG_UINT8(&msg[0])>>1)&1);
            ledGreen((MSG_UINT8(&msg[0])>>2)&1);

            enable_prox_sensor = ((MSG_UINT8(&msg[0])>>4)&1);
            enable_line_sensor = ((MSG_UINT8(&msg[0])>>5)&1);
        }
        break;
      case CMD_PUTCHAR:
        {
          int k=0;          
          for(k=0;k<n;k++)
          {
            Serial1.write(MSG_UINT8(&msg[k]));
          }
        }
        break;      
      case CMD_WATCHDOG:
        {
          if( msg[0] == 1 )
          {
            wdt_enable(WDTO_500MS);
          }
          else if( msg[0] == 2 )
          {
            wdt_enable(WDTO_1S);
          }
          else if( msg[0] == 3 )
          {
            wdt_enable(WDTO_2S);
          }
          else if( msg[0] == 4 )
          {
            wdt_enable(WDTO_4S);
          }
          else if( msg[0] == 5 )
          {
            wdt_enable(WDTO_8S);
          }
          else if( msg[0] == 6 )
          {
            wdt_disable();
          }
          else if( msg[0] == 7 )
          {
            wdt_reset();
          }
        }
        break;               
    }
  }
  ledGreen(0);
}

void requestEvent()
{
  ledGreen(1);
  uint8_t idx = 0;
  uint8_t buf[4 + 2 + 4 + 2*NUM_SENSORS + 6 + 1];

  buf[idx++] = (uint8_t)(loopcnt >> 24) & 0xff;
  buf[idx++] = (uint8_t)(loopcnt >> 16) & 0xff;
  buf[idx++] = (uint8_t)(loopcnt >> 8) & 0xff;
  buf[idx++] = (uint8_t)(loopcnt >> 0) & 0xff;

  buf[idx++] = (uint8_t)(bat >> 8) & 0xff;
  buf[idx++] = (uint8_t)(bat >> 0) & 0xff;

  buf[idx++] = (uint8_t)(enc_l >> 8) & 0xff;
  buf[idx++] = (uint8_t)(enc_l >> 0) & 0xff;
  enc_l = 0;

  buf[idx++] = (uint8_t)(enc_r >> 8) & 0xff;
  buf[idx++] = (uint8_t)(enc_r >> 0) & 0xff;
  enc_r = 0;

  {
    int i;
    for(i=0;i<NUM_SENSORS;i++)    
    {
      buf[idx++] = (uint8_t)(lineSensorValues[i] >> 8) & 0xff;
      buf[idx++] = (uint8_t)(lineSensorValues[i] >> 0) & 0xff;
    }
  }

  {
    int i = 0;
    buf[idx++] = proxSensorsValues[i++];
    buf[idx++] = proxSensorsValues[i++];
    buf[idx++] = proxSensorsValues[i++];
    buf[idx++] = proxSensorsValues[i++];
    buf[idx++] = proxSensorsValues[i++];
    buf[idx++] = proxSensorsValues[i++];
  }

  buf[idx++] = (usbPowerPresent()?(1<<4):0)|
               (buttonA.isPressed()?(1<<0):0)|
               (buttonB.isPressed()?(1<<1):0)|
               (buttonC.isPressed()?(1<<2):0);

  Wire.write(buf, sizeof(buf));
  ledGreen(0);
}
