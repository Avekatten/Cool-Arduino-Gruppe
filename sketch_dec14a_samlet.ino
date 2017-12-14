/*
 * 
 * Koden her er skrevet videre fra et udleveret kode eksempel fra undervisningen.
 * Det udleverede eksempel havde interruptet, samt komando og skrivning til dcc sadt op.
 * 
 * Seriel monitoren skal stå til Newline som afslutning på send kommandoen.
 */


#define DCC_PIN    4  // Arduino pin for DCC out 

boolean setupDone = false;
unsigned char data, xdata;

int tog1Adr = 0;  //36
int tog2Adr = 0;  //40
int tog1Pos = 0;  // 1-2
int tog2Pos = 0;  

int aktuelleSkifteSpor = 0; //101 eller 102

int confirmedSetup = 0;  //0 som start 99 når færdig

int  *ip;

const int trigPin = 9;
const int echoPin = 10;
long      duration;
int       distance;
boolean   innerPassing;
boolean   outerPassing;
boolean   innerPassed;
boolean   outerPassed;
boolean   stopSend;
boolean   changeTrainTracks = false;
boolean   trainChanged;
int       counter = 0; 


int locoAdr = 0;

//train ordrer
struct trainOrders
{
  int backwards = 0x46;
  int forwards = 0x66;
  int tStop = 0x60;

} train;

//sporskift
struct skifteSporStruct
{
  int address;
  int straightCommando1;
  int straightCommando2;  
  int turnCommando1;
  int turnCommando2;
};
skifteSporStruct skifteSpor101 = {.address = 0x9a, .straightCommando1 = 0xF9, .straightCommando2 = 0xF1, .turnCommando1 = 0xF8, .turnCommando2 = 0xF0};

skifteSporStruct skifteSpor102 = {.address = 0x9a, .straightCommando1 = 0xFb, .straightCommando2 = 0xF3, .turnCommando1 = 0xFa, .turnCommando2 = 0xF2};
//int aktuelleSkifteSpor = 102;


int trainTrack; // 0 = outer 1 = inner, holder styr på om vi sender straight eller turn til sporskifte

//Timer frequency is 2MHz for ( /8 prescale from 16MHz )
#define TIMER_SHORT 0x8D  // 58usec pulse length 141 255-141=114
#define TIMER_LONG  0x1B  // 116usec pulse length 27 255-27 =228

unsigned char last_timer = TIMER_SHORT; // store last timer value

unsigned char flag = 0;  // used for short or long pulse
unsigned char every_second_isr = 0;  // pulse up or down

// definitions for state machine
#define PREAMBLE  0
#define SEPERATOR 1
#define SENDBYTE  2

unsigned char state = PREAMBLE;
unsigned char preamble_count = 16;
unsigned char outbyte = 0;
unsigned char cbit = 0x80;

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

int msgIndex = 0;
int byteIndex = 0;

//Setup Timer2.
//Configures the 8-Bit Timer2 to generate an interrupt at the specified frequency.
//Returns the time load value which must be loaded into TCNT2 inside your ISR routine.

void SetupTimer2()
{
  //Timer2 Settings: Timer Prescaler /8, mode 0
  //Timmer clock = 16MHz/8 = 2MHz oder 0,5usec
  // - page 206 ATmega328/P
  TCCR2A = 0;
  TCCR2B = 0 << CS22 | 1 << CS21 | 0 << CS20;

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
  TIMSK2 = 1 << TOIE2;

  //load the timer for its first cycle
  TCNT2 = TIMER_SHORT;
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
    digitalWrite(DCC_PIN, 1);
    every_second_isr = 0;

    // set timer to last value
    latency = TCNT2;
    TCNT2 = latency + last_timer;

  }
  else
  { // != every second interrupt, advance bit or state
    digitalWrite(DCC_PIN, 0);
    every_second_isr = 1;

    switch (state)
    {
      case PREAMBLE:
        flag = 1; // short pulse
        preamble_count--;
        if (preamble_count == 0)
        { // advance to next state
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
        flag = 0; // long pulse
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
        { // last bit sent, is there a next byte?
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
    { // if data==1 then short pulse
      latency = TCNT2;
      TCNT2 = latency + TIMER_SHORT;
      last_timer = TIMER_SHORT;
      //Serial.print('1');
    }
    else
    { // long pulse
      latency = TCNT2;
      TCNT2 = latency + TIMER_LONG;
      last_timer = TIMER_LONG;
      //Serial.print('0');
    }
  }
}

void setup(void)
{
  pinMode(DCC_PIN, OUTPUT);  // this is for the DCC Signal

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(115200); // Starts the serial communication

  assemble_setup_msg();
  //Start the timer
  SetupTimer2();
}

void loop(void)
{
  //
  //
  switch (confirmedSetup) 
   {
    case 99:
      break;
    case 0:
      skrivDSB();    
      Serial.println("tast tog 1 adresse: ");
      confirmedSetup++;
      break;
    case 1:
      ip = &tog1Adr;
      getNumber(ip);
      break;
    case 2:
      Serial.println("tast tog 1 position (1 eller 2): ");
      confirmedSetup++;
      break;
    case 3:
      ip = &tog1Pos;        
      getNumberPos(ip);
      break;
    case 4:
      Serial.println("tast tog 2 Adresse");
      confirmedSetup++;
      break;
    case 5:
      ip = &tog2Adr;
      getNumber(ip);        
      break;
    case 6:
      Serial.println("tast tog 2 position 1 eller 2: ");
      confirmedSetup++;
      break;
    case 7:
      ip = &tog2Pos;        
      getNumberPos(ip);
      break;
    case 8:
      Serial.println("Adressen på toget der kører først?");
      confirmedSetup++;
      break;
    case 9:
      ip = &locoAdr;
      getNumber(ip);                
      break;
    case 10:
      Serial.println("Adressen på sporskiftet ");
      confirmedSetup++;
      break;
    case 11:
      ip = &aktuelleSkifteSpor;
      getNumber(ip);
      break;
    case 12:
      skrivInfo();
      Serial.println("Kan disse værdier bruges? J/N");
      confirmedSetup++;
      break;
    case 13:        
      getYesNo(); //sætter confirmedSetup til 0 eller 99
      break;    
   }  
  //
  //
  if(confirmedSetup == 99){
    delay(300);
    Ultrasound();
    Serial.print("Distance: ");
    Serial.println(distance);
    
    if (!setupDone)
    {
      assemble_setup_msg();
    }
    else
    {
      checkForPassing(); // Sætter booleans for om et tog er igang med at passere eller passeret
      decideTrainOrder(); // Sætter ordren til stop hvis et tog er passeret ellers sender vi kør
      assemble_dcc_msg();
    }
  }
}

void getNumber(int *ip){
  if (Serial.available())
  {
    char input = Serial.read();
    if(isDigit(input))
    {
      //tog1Adr = (tog1Adr * 10) + (input - '0'); //math
      *ip = (*ip * 10) + (input - '0'); //math
      //Serial.println(input);
    }else if(input == 10){
      confirmedSetup++;
    }
    else{
    } 
  }
}
void getNumberPos(int *ip){
  if (Serial.available())
  {
    //ip = &tog2Pos;
    char input = Serial.read();
    if(input == '1')
    {
      *ip = 1;
    }else if(input == '2'){
      *ip = 2;
    }
    else if(input == 10){
      confirmedSetup++;
    }else{
     
    }
    
  }
}
void getYesNo(){
    if (Serial.available())
    { 
      char input = Serial.read();
      if(input=='J'||input=='j'){
        Serial.println("Værdierne blev godkendt");
        confirmedSetup = 99;
      }else if(input=='N'||input=='n'){
        Serial.println("Værdierne blev ikke godkendt");
        if (Serial.available())
        {
          char removeAll = Serial.read(); //tømmer evt serial før går tilbage til start
        }
        tog1Adr = 0;  
        tog2Adr = 0;  
        tog1Pos = 0;  
        tog2Pos = 0;  
        locoAdr = 0; 
        aktuelleSkifteSpor = 0;
        
        confirmedSetup = 0;
      }else{
      }
    }  
}
void skrivDSB(){

  skrivPlads(4);
  // 22 linier høj
  Serial.println("         ** Tog Banen **     ");
  Serial.println("                             ");
  Serial.println("          _____________      Position: 2"); // Outer Track
  Serial.println("         /             \\     ");
  Serial.println("        /               \\    ");
  Serial.println("       /                 \\   ");
  Serial.println("      |   _____________   |  Position: 1");  // Inner Track
  Serial.println("      |  /             \\  |  ");
  Serial.println("      | /               \\ |  ");
  Serial.println("      |/                 \\|  ");
  Serial.println("      |                   |  ");
  Serial.println("      |                   |  ");
  Serial.println("      |                   |  ");
  Serial.println("      |                   |  ");
  Serial.println("      |                   |  ");
  Serial.println("      |                   |  ");
  Serial.println("      |                   |  ");
  Serial.println("      |                   |  ");
  Serial.println("      |                   |  ");
  Serial.println("       \\                 /   ");
  Serial.println("        \\               /    ");
  Serial.println("         \\_____________/     ");   
  skrivPlads(6); //18 i alt
}
void skrivInfo(){
  Serial.println("Tog 1 adresse: ");
  Serial.println(tog1Adr);
  Serial.println("Tog 1 position: ");
  Serial.println(tog1Pos);
  Serial.println("Tog 2 adresse: ");
  Serial.println(tog2Adr);
  Serial.println("Tog 2 position: ");
  Serial.println(tog2Pos);
  Serial.println("Toget der kører først: ");
  Serial.println(locoAdr);
  Serial.println("Adressen på sporskiftet: ");
  Serial.println(aktuelleSkifteSpor);
  Serial.println();
}
void skrivPlads(int antal){
  int i;
  for(i = 0; i < antal; i++)
  {
    Serial.println();    
  }    
}


void assemble_dcc_msg()
{
  noInterrupts();  // make sure that only "matching" parts of the message are used in ISR

  if (changeTrainTracks == true) // Send til skiftespor
  {
    if (aktuelleSkifteSpor == 101)
    {
      if (trainTrack == 0)
      {
        Serial.println("Skiftespor drej");
        msg[1].data[0] = skifteSpor101.address;
        msg[2].data[0] = skifteSpor101.address;
        msg[1].data[1] = skifteSpor101.turnCommando1;
        msg[2].data[1] = skifteSpor101.turnCommando2;
  
        changeTrainTracks = false;
        trainTrack = 1;
      }
      else if (trainTrack == 1)
      {
        Serial.println("Skiftespor ligud");
        msg[1].data[0] = skifteSpor101.address;
        msg[2].data[0] = skifteSpor101.address;
        msg[1].data[1] = skifteSpor101.straightCommando1;
        msg[2].data[1] = skifteSpor101.straightCommando2;
  
        changeTrainTracks = false;
        trainTrack = 0;
      }
    }
    else if (aktuelleSkifteSpor == 102)
    {
      if (trainTrack == 0)
      {
        Serial.println("Skiftespor drej");
        msg[1].data[0] = skifteSpor102.address;
        msg[2].data[0] = skifteSpor102.address;
        msg[1].data[1] = skifteSpor102.turnCommando1;
        msg[2].data[1] = skifteSpor102.turnCommando2;
  
        changeTrainTracks = false;
        trainTrack = 1;  
      }
      else if (trainTrack == 1)
      {
        Serial.println("Skiftespor ligud");
        msg[1].data[0] = skifteSpor102.address;
        msg[2].data[0] = skifteSpor102.address;
        msg[1].data[1] = skifteSpor102.straightCommando1;
        msg[2].data[1] = skifteSpor102.straightCommando2;
  
        changeTrainTracks = false;
        trainTrack = 0;
      }
    }
  }
  else // Send til tog
  {
    Serial.println("Sender til tog");
    msg[1].data[0] = locoAdr;
    msg[2].data[0] = locoAdr;
    msg[1].data[1] = data;
    msg[2].data[1] = data;
  }

  printOrdreAdresse();  // Serial prints for debugging

  msg[1].data[2] = msg[1].data[0] ^ msg[1].data[1]; // XOR bytes
  msg[2].data[2] = msg[2].data[0] ^ msg[2].data[1];

  interrupts();

  if (trainChanged) // Når vi har skiftet tog adressen
  {
    changeTrainTracks = true; // skifter vi sporskiftet
    trainChanged = false;
  }

  if (stopSend == true) // Når vi har stoppet toget skifter vi adressen
  {
    if (locoAdr == tog1Adr)
    {
      locoAdr = tog2Adr;
      stopSend = false;
      trainChanged = true;

    }
    else if (locoAdr == tog2Adr)
    {
      locoAdr = tog1Adr;
      stopSend = false;
      trainChanged = true;
    }
  }
}

void assemble_setup_msg()
{
  noInterrupts();
  if (counter == 0)
  {
    msg[1].data[0] = tog1Adr;
    msg[2].data[0] = tog1Adr;
    msg[1].data[1] = train.tStop;
    msg[2].data[1] = train.tStop;
  }
  else if (counter == 1)
  {
    msg[1].data[0] = tog2Adr;
    msg[2].data[0] = tog2Adr;
    msg[1].data[1] = train.tStop;
    msg[2].data[1] = train.tStop;
   
  }
  else if (counter == 2)
  {
    if (locoAdr == tog1Adr)
    {
      if (tog1Pos == 1)
      {
        msg[1].data[0] = skifteSpor101.address;
        msg[2].data[0] = skifteSpor101.address;
        msg[1].data[1] = skifteSpor101.turnCommando1;
        msg[2].data[1] = skifteSpor101.turnCommando2;

        trainTrack = 1; 
      }
      else if (tog1Pos == 2)
      {
        msg[1].data[0] = skifteSpor102.address;
        msg[2].data[0] = skifteSpor102.address;
        msg[1].data[1] = skifteSpor102.straightCommando1;
        msg[2].data[1] = skifteSpor102.straightCommando2;
        trainTrack = 0; 
      }
    }
    if (locoAdr == tog2Adr)
    {
      if (tog2Pos == 1)
      {
        msg[1].data[0] = skifteSpor101.address;
        msg[2].data[0] = skifteSpor101.address;
        msg[1].data[1] = skifteSpor101.turnCommando1;
        msg[2].data[1] = skifteSpor101.turnCommando2;

        trainTrack = 1; 
        
      }
      else if (tog1Pos == 2)
      {
        msg[1].data[0] = skifteSpor102.address;
        msg[2].data[0] = skifteSpor102.address;
        msg[1].data[1] = skifteSpor102.straightCommando1;
        msg[2].data[1] = skifteSpor102.straightCommando2;

        trainTrack = 0; 
      }
    }
    setupDone = true;
  }
  msg[1].data[2] = msg[1].data[0] ^ msg[1].data[1]; // XOR bytes
  msg[2].data[2] = msg[2].data[0] ^ msg[2].data[1];
  counter++;
  interrupts();
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
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
}

void checkForPassing()
{
  if (innerPassing == false && outerPassing == false)
  {
    if (distance < 8)
    {
      innerPassing = true;
    }
    else if (distance < 20 && distance > 12)
    {
      outerPassing = true;
    }
  }
  else if (innerPassing)
  {
    if (distance > 20)
    {
      innerPassed = true;
      innerPassing = false;
    }
  }
  else if (outerPassing)
  {
    if (distance > 20 && distance < 30)
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
    data = train.tStop;
    stopSend = true;
    outerPassed = false;
  }
  else if (innerPassed)
  {
    data = train.tStop;
    stopSend = true;
    innerPassed = false;
  }
  else
  {
    data = train.forwards;
  }
}

void printOrdreAdresse()
{
  Serial.print("Odre: ");
  Serial.println(msg[1].data[1]);
  Serial.print("Adresse: ");
  Serial.println(msg[1].data[0]);
}

