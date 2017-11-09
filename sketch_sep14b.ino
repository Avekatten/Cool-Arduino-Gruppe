#define TIMER_SHORT 0x8D
unsigned char a = 58;
unsigned char b = 116;
unsigned char myCheckSum[8] = {0};
unsigned char currentMsg[42]= {0};
unsigned char buildMsg[42]= {0};

boolean isSending = false;
boolean rdyToSend = false;
boolean isBuilding = false;

unsigned char preample[] = {a, a, a, a, a, a, a, a, a, a, a, a, a};
unsigned char togAdress[] = {b, b, a, b, a, b, b, b};
unsigned char ordre[] = {b, a, a, b, b, a, b, b};

int indx = 0;
boolean lastSet = false;
boolean dlay = false;

int p1 = 5;
int p2 = 6;

void setup()
{
  Serial.begin(9600);
  pinMode(p1, OUTPUT);
  pinMode(p2, INPUT_PULLUP);
  constructMessage();
  SetupTimer2();
  
}

void SetupTimer2(){
 
  // Don't get interrupted while setting up the timer
  noInterrupts();
  ////
  //// Set timer2 interrupt at 58 microseconds interval
  ////
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2  = 0; // initialize counter value to 0

  // 58 microseconds interval is 17,241 kHz
  // set compare match register for 17 kHz increments:
  // register = (Arduino clock speed) / ((desired interrupt frequency) * prescaler) - 1
  // register = (16*10^6) / (17.241 * 8) - 1 
  // register = 115
  // (must be <256 because timer2 is only 8 bit (timer1 is 16 bit))
  OCR2A = 115; 
  // = 116 ticks before reset (because 0 counts)
  // = 58 microseconds for each tick at 2 MHz

  // Turn on CTC mode
  TCCR2A |= (1 << WGM21);

  // Set CS21 bit for 8 prescaler (and not CS20 or CS22)
  TCCR2B |= (1 << CS21); // = 2 MHz = 0,5 microseconds/tick
  // (0 << CS20) || (0 << CS21) == 0

  // Enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);

  interrupts(); // Reenable interrupts
}

void loop()
{
    
    int ab;
    ab = digitalRead(p2);
    if (ab == LOW)
    {
      ordre[2] = b;
      constructMessage();
    }
    else
    {
      ordre[2] = a;
      constructMessage();
    }
    
    
      
}

void constructMessage(){
  isBuilding = true;
  
  int j = 0;
  for(int i = 0; i<13; i++ )
  {
      buildMsg[j] = preample[i];
      j++;
  }
  
  buildMsg[j] = b;
  j++;
  
  
  for(int i = 0; i<8; i++ )
  {
      buildMsg[j] = togAdress[i];
      j++;
  }

  buildMsg[j] = b;
  j++;
  
  for(int i = 0; i<8; i++)
  {
      buildMsg[j] = ordre[i];
      j++;
  }
  buildMsg[j] = b;
  j++;
  checkSum();

  
  for(int i = 0; i<8; i++ )
  {
      buildMsg[j] = myCheckSum[i];
      j++;
  }

  buildMsg[j]=a;
  j++;
  buildMsg[j]=a;
  isBuilding = false;
  rdyToSend= true;

}

/*void mySend(unsigned char arr[], int sizeOfArray)
{
  for (int i = 0; i < sizeOfArray; i++)
  {
    digitalWrite(5, HIGH);
    delayMicroseconds(arr[i]); 
    digitalWrite(5, LOW);
    delayMicroseconds(arr[i]);  
    
  }
  
}

/*void sendBit(int myBit)
{
    digitalWrite(5, HIGH);
    delayMicroseconds(myBit); 
    digitalWrite(5, LOW);
    delayMicroseconds(myBit); 
}
*/

void checkSum()
{ 
  for(int i = 0; i < 8; i++)
  {
    if(togAdress[i] == ordre[i])
    {
      myCheckSum[i] = b;
    }
    else
    {
      myCheckSum[i] = a;
    }
  }
}

ISR(TIMER2_COMPA_vect)
{
  
  if(isBuilding || isSending){
    isSending = true;
      if(!dlay)
      {
         if(currentMsg[indx] == a)
         {
            if(!lastSet)
            {
              digitalWrite(p1,HIGH);
              lastSet = true;
            } 
            else
            {
              digitalWrite(p1,LOW);
              lastSet = false;
              indx++;
              
            }
         }
         else
         {
           dlay = true;
           if(!lastSet)
           {
             digitalWrite(p1, HIGH);
             lastSet = true;
           }
           else
           {
             digitalWrite(p1, LOW);
             lastSet = false;
             indx++;              
           }
           
         }
       
      }
      else
      {
        dlay = false;
      }

      if(indx >41)
      {
          isSending = false;
          indx = 0;
      }
  }
  else{

   if(rdyToSend)
   {
    for(int i = 0; i<42; i++)
    {
        currentMsg[i] = buildMsg[i];
    }
    //currentMsg = buildMsg;
    rdyToSend = false; 
   }
   isSending= true;
   
    
    }


    
}
