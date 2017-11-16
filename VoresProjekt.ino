#define DCC_PIN    4  // Arduino pin for DCC out 
#define soundpin   3  // sense for direction
#define dirpin     2  // sense for direction
#define skiftepinOne  5 
#define skiftepinTwo 6
#define skiftepinThree 7
#define skiftepinFour 8

#define togAdrEt 36
#define togAdrTo 40

unsigned char data, xdata;
const int trigPin = 9;
const int echoPin = 10;
// defines variables
long duration;
int distance;
boolean innerPassing;
boolean outerPassing;
boolean innerPassed;
boolean outerPassed;
boolean stopSend;
boolean changeTrainTracks = false;
boolean trainChanged;

int i,j,k,l,m,n;
int trainTrack; // 0 = outer 1 = inner

//Timer frequency is 2MHz for ( /8 prescale from 16MHz )
#define TIMER_SHORT 0x8D  // 58usec pulse length 141 255-141=114
#define TIMER_LONG  0x1B  // 116usec pulse length 27 255-27 =228

unsigned char last_timer=TIMER_SHORT;  // store last timer value
   
unsigned char flag=0;  // used for short or long pulse
unsigned char every_second_isr = 0;  // pulse up or down

// definitions for state machine 
#define PREAMBLE  0    
#define SEPERATOR 1
#define SENDBYTE  2

unsigned char state = PREAMBLE;
unsigned char preamble_count = 16;
unsigned char outbyte = 0;
unsigned char cbit = 0x80;

// variables for throttle
unsigned char locoSpeed=0;
unsigned char dir=1; //forward
unsigned char locoAdr=36;   // this is the (fixed) address of the loco
unsigned char skifteAdr=0x9A;
unsigned char sound=0;

boolean harSendt = false;
int skiftSpor = 0;

// buffer for command
struct Message 
{
   unsigned char data[7];
   unsigned char len;
};

#define MAXMSG  3
// for the time being, use only two messages - the idle msg and the loco Speed msg

struct Message msg[MAXMSG] = 
{ 
    { { 0xFF,     0, 0xFF, 0, 0, 0, 0}, 3},   // idle msg
    { { 0, 0,  0, 0, 0, 0, 0}, 3},    // locoMsg with 128 speed steps
    { { 0, 0,  0, 0, 0, 0, 0}, 3}
    
};               // loco msg must be filled later with speed and XOR data byte
                                
int msgIndex=0;  
int byteIndex=0;


//Setup Timer2.
//Configures the 8-Bit Timer2 to generate an interrupt at the specified frequency.
//Returns the time load value which must be loaded into TCNT2 inside your ISR routine.

void SetupTimer2()
{
  //Timer2 Settings: Timer Prescaler /8, mode 0
  //Timmer clock = 16MHz/8 = 2MHz oder 0,5usec
  // - page 206 ATmega328/P
  TCCR2A = 0;
  TCCR2B = 0<<CS22 | 1<<CS21 | 0<<CS20; 
  
/*         bit 2     bit 1     bit0
            0         0         0       Timer/Counter stopped 
            0         0         1       No Prescaling
            0         1         0       Prescaling by 8
            0         0         0       Prescaling by 32
            1         0         0       Prescaling by 64
            1         0         1       Prescaling by 128
            1         1         0       Prescaling by 256
            1         1         1       Prescaling by 1024
*/
 
  //Timer2 Overflow Interrupt Enable - page 211 ATmega328/P   
  TIMSK2 = 1<<TOIE2; 
  
  //load the timer for its first cycle
  TCNT2=TIMER_SHORT; 
}

//Timer2 overflow interrupt vector handler
ISR(TIMER2_OVF_vect) 
{
  //Capture the current timer value TCTN2. This is how much error we have
  //due to interrupt latency and the work in this function
  //Reload the timer and correct for latency.  
  unsigned char latency;
  
  // for every second interupt just toggle signal
  if (every_second_isr)  
  {
     digitalWrite(DCC_PIN,1);
     every_second_isr = 0;    
     
     // set timer to last value
     latency=TCNT2;
     TCNT2=latency+last_timer; 
     
  }
  else  
  {  // != every second interrupt, advance bit or state
     digitalWrite(DCC_PIN,0);
     every_second_isr = 1; 
     
     switch(state)  
     {
       case PREAMBLE:
           flag=1; // short pulse
           preamble_count--;
           if (preamble_count == 0)  
           {  // advance to next state
              state = SEPERATOR;
              // get next message
              msgIndex++;
              if (msgIndex >= MAXMSG)  
              {  
                msgIndex = 0; 
              }  
              byteIndex = 0; //start msg with byte 0
           }
           break;
        case SEPERATOR:
           flag=0; // long pulse
           // then advance to next state
           state = SENDBYTE;
           // goto next byte ...
           
           cbit = 0x80;  // send this bit next time first         
           outbyte = msg[msgIndex].data[byteIndex];
           break;
        case SENDBYTE:
           if (outbyte & cbit)  
           { 
              flag = 1;  // send short pulse
           }
           else  
           {
              flag = 0;  // send long pulse
           }
           cbit = cbit >> 1;
           if (cbit == 0)  
           {  // last bit sent, is there a next byte?
              //Serial.print(" ");
              byteIndex++;
              
              if (byteIndex >= msg[msgIndex].len)  
              {
                 // this was already the XOR byte then advance to preamble
                 state = PREAMBLE;
                 preamble_count = 16;
                 //Serial.println();
 
              }
              else  
              {
                 // send separtor and advance to next byte
                 state = SEPERATOR ;
              }
           }
           break;
     }   
 
     if (flag)  
     {  // if data==1 then short pulse
        latency=TCNT2;
        TCNT2=latency+TIMER_SHORT;
        last_timer=TIMER_SHORT;
        //Serial.print('1');
     }  
     else  
     {   // long pulse
        latency=TCNT2;
        TCNT2=latency+TIMER_LONG; 
        last_timer=TIMER_LONG;
        //Serial.print('0');
     } 
  }
}

void setup(void) 
{
  pinMode(DCC_PIN,OUTPUT);   // this is for the DCC Signal
  pinMode(dirpin,INPUT_PULLUP);
  pinMode(soundpin,INPUT_PULLUP);
  pinMode(skiftepinOne,INPUT_PULLUP);
  pinMode(skiftepinTwo,INPUT_PULLUP);
  pinMode(skiftepinThree,INPUT_PULLUP);
  pinMode(skiftepinFour,INPUT_PULLUP);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(9600); // Starts the serial communication
  
  assemble_dcc_msg();
    
  //Start the timer 
  SetupTimer2();   
}

void loop(void) 
{
  delay(200);
  Ultrasound();
  assemble_dcc_msg();
}



void assemble_dcc_msg() 
{  
   i = digitalRead(dirpin);
   j = digitalRead(soundpin);
   k = digitalRead(skiftepinOne);
   l = digitalRead(skiftepinTwo);
   m = digitalRead(skiftepinThree);
   n = digitalRead(skiftepinFour);

   Serial.print("Distance: ");
   Serial.println(distance);


   checkForPassing();

   decideTrainOrder();

   if (k == LOW)
   {
      skiftSpor = 1;
   }
   else if (l == LOW)
   {
      skiftSpor = 2;
   }
   else if (m == LOW)
   {
      skiftSpor = 3;
   }
   else if (n == LOW)
   {
      skiftSpor = 4;
   }
   else
   {
      skiftSpor = 0;
      harSendt = false;
   }
   
   
   noInterrupts();  // make sure that only "matching" parts of the message are used in ISR
    if (changeTrainTracks == true)
   {
      if (trainTrack == 0)
      {
          Serial.println("1");
          msg[1].data[0] = 0x9A;
          msg[2].data[0] = 0x9A; 
          msg[1].data[1] = 0xF8;
          msg[2].data[1] = 0xF0; 
  
          changeTrainTracks = false;
          trainTrack = 1;
      }
      else if (trainTrack == 1)
      {
          Serial.println("2");
          msg[1].data[0] = 0x9A;
          msg[2].data[0] = 0x9A; 
          msg[1].data[1] = 0xF9;
          msg[2].data[1] = 0xF1; 

          changeTrainTracks = false;
          trainTrack = 0;
      }
   }
   else if (skiftSpor == 0 || harSendt)
   {
      Serial.println("0");
      msg[1].data[0] = locoAdr;
      msg[2].data[0] = locoAdr; 
      msg[1].data[1] = data;
      msg[2].data[1] = data; 
   }
   /*else if (skiftSpor == 1) // 101 drej
   {
      Serial.println("1");
      msg[1].data[0] = 0x9A;
      msg[2].data[0] = 0x9A; 
      msg[1].data[1] = 0xF8;
      msg[2].data[1] = 0xF0;

      harSendt = true;
   }
   else if (skiftSpor == 2)// 101 ligeud
   {
      Serial.println("2");
      msg[1].data[0] = 0x9A;
      msg[2].data[0] = 0x9A; 
      msg[1].data[1] = 0xF9;
      msg[2].data[1] = 0xF1; 

      harSendt = true;
   }
   else if (skiftSpor == 3) // 102 drej
   {
      Serial.println("3");
      msg[1].data[0] = 0x9A;
      msg[2].data[0] = 0x9A; 
      msg[1].data[1] = 0xFA;
      msg[2].data[1] = 0xF2;

      harSendt = true;
   }
   else if (skiftSpor == 4)// 102 ligeud
   {
      Serial.println("4");
      msg[1].data[0] = 0x9A;
      msg[2].data[0] = 0x9A; 
      msg[1].data[1] = 0xFB;
      msg[2].data[1] = 0xF3;

      harSendt = true;
   } */
   
   Serial.println(msg[1].data[1]);
   Serial.println(msg[1].data[0]);
   msg[1].data[2] = msg[1].data[0] ^ msg[1].data[1];
   msg[2].data[2] = msg[2].data[0] ^ msg[2].data[1];
  
   interrupts();

   if (trainChanged)
   {
      changeTrainTracks = true;
      trainChanged = false;
   }
   
   if (stopSend == true) // if stop has been send we swap the train adress
   {
      if (locoAdr == togAdrEt)
      {
          locoAdr = togAdrTo;
          stopSend = false;
          trainChanged = true;
        
      }
      else if (locoAdr == togAdrTo)
      {
         locoAdr = togAdrEt;
         stopSend = false;
         trainChanged = true;
      }
   }
}

void Ultrasound()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance= duration*0.034/2;
  // Prints the distance on the Serial Monitor
}

void checkForPassing()
{
  
   if (innerPassing == false && outerPassing == false)
   {
     if (distance < 6)
     {
        innerPassing = true;
     }
     else if (distance < 15 && distance > 7)
     {
        outerPassing = true;
     }
   }
   else if (innerPassing)
   {
      if (distance > 6)
      {
        innerPassed = true;
        innerPassing = false;
      }
   }
   else if (outerPassing)
   {
      if (distance > 15 || distance < 7)
      {
        outerPassed = true;
        outerPassing = false;
      }
   }
}

void decideTrainOrder()
{
   if (outerPassed)
   {
      data = 0x41;  
      stopSend = true;
      outerPassed = false;
   }
   else if (innerPassed)
   {
      data = 0x41;
      stopSend = true;
      innerPassed = false;
   }
   else
   {
      if (i == HIGH) // Forwards if no pin
      {
         data = 0x66;
      }
      else
      {
        data = 0x46; // Backwards if pin
      }  
   }
}

