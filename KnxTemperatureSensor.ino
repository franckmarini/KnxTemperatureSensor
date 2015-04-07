// Sketch for the KNX Temperature sensor
// Hardware by Matthias Freudenreich
// Software by Franck Marini
// Visit http://liwan.fr/KnxWithArduino/ (post on on KNX Temperature sensor) to get all the infos! 


#include <avr/interrupt.h> 
#include <avr/power.h>
#include <avr/sleep.h>
#include <KnxDevice.h> // get the library at https://github.com/franckmarini/KnxDevice (use V0.3 or later)
#include <DHT22.h> // get the library at http://playground.arduino.cc/Main/DHTLib

int greenPin = 13; // green LED connected to pin 7 with active high
int SWen= 4;       // PIN UART Switch
int DHTpin = 8;    // DHT22 sensor pin

// DHT22 sensor
dht DHT;
#define DHT22_PIN 9
struct
{
    uint32_t total;
    uint32_t ok;
    uint32_t crc_error;
    uint32_t time_out;
    uint32_t connect;
    uint32_t ack_l;
    uint32_t ack_h;
    uint32_t unknown;
} stat = { 0,0,0,0,0,0,0,0};

// Definition of the Communication Objects attached to the device
KnxComObject KnxDevice::_comObjectsList[] =
{
 /* Index 0 = TEMPERATURE VALUE */    KnxComObject(G_ADDR(2,0,0), KNX_DPT_9_001 /* 2 byte float DPT */ , COM_OBJ_SENSOR  /* Sensor Output */ ) ,
 /* Index 1 = HUMIDITY VALUE    */    KnxComObject(G_ADDR(2,0,1), KNX_DPT_9_001 /* 2 byte float DPT */ , COM_OBJ_SENSOR  /* Sensor Output */ ) ,
};
const byte KnxDevice::_comObjectsNb = sizeof(_comObjectsList) / sizeof(KnxComObject); // do no change this code


// sleepNow function stops Timer0 and puts the device to sleep
void sleepNow() {
   TCCR0B &= 0xFC; // We stop TIMER0 to prevent TIMER0 interrupts, Arduino time functions get time frozen!
   set_sleep_mode(SLEEP_MODE_IDLE); // Select sleep mode
   sleep_enable(); // Set sleep enable bit
   sleep_mode(); // Put the device to sleep
   // Upon waking up, sketch continues from this point.
   sleep_disable();
   TCCR0B |= 0x03; // We restart TIMER0
}


// Set timer1 with 6 seconds period (Arduino clock speed 8MHz)
// NB : The ideal implementation would be to set timer1 to 60sec, but timer1 counter is not large enough for that
void timer1Init()
{
   noInterrupts(); // disable all interrupts
  TCCR1A = 0; // initialize timer1 
  TCCR1B = 0; // initialize timer1 
  TCNT1  = 0; // initialize timer1 
 // interrupt frequency (Hz) = (Arduino clock speed 8MHz) / (prescaler * (compare match register + 1))
  OCR1A = 46874;            // compare match register : 1/6Hz = 8MHz / 1024 / (46874 + 1)
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS10);   TCCR1B |= (1 << CS12); // clk I/O /1024 prescaling
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
}

static boolean oneMinElapsed = false;
static boolean oneDHTstart = false;


// TIMER1 compare interrupt service routine
// the function is executed at each TIMER1 interrupt (every 6 sec)
ISR(TIMER1_COMPA_vect) {
static byte counter = 0;
  counter ++;
  if (counter / 10) /* 1 min = 10 x 6 sec */ 
  { oneMinElapsed = true; counter = 0; }
}


// Callback function to handle com objects updates
void knxEvents(byte index) {};


void setup(){
   pinMode(SWen, OUTPUT);
   digitalWrite(SWen, HIGH); 
   pinMode(greenPin, OUTPUT); 
   pinMode(DHTpin, OUTPUT);
   digitalWrite(DHTpin, HIGH); 
   timer1Init();
   power_adc_disable(); // switch ADC module off (power savings)
   power_spi_disable(); // switch SPI module off (power savings)
   power_timer2_disable(); // switch TIMER2 off (power savings)
   power_twi_disable(); // switch TWI off (power savings)
   Knx.begin(Serial, P_ADDR(1,1,30)); // start a KnxDevice session with physical address 1.1.30 on Serial UART
}


void loop(){ 
  Knx.task();
  
  digitalWrite(greenPin, LOW);
     
  if (oneMinElapsed) 
  { // Every min we read humidity and temperature and send values to the KNX bus
    oneMinElapsed = false;
    digitalWrite(greenPin, HIGH);
    int chk = DHT.read22(DHT22_PIN);
    Knx.write(0,DHT.temperature);
    Knx.write(1,DHT.humidity);
  }
  if (!Knx.isActive()) sleepNow(); // No KNX activity so let's have some rest !
  // we get here when either a timer1 interrupt or an UART RX interrupt has occured
}

