int mainaddress;
unsigned char Address;
unsigned char Register;

unsigned char Byte1;
unsigned char Byte2;

unsigned char mX;
unsigned char mX2;

unsigned char shiftValue = 8;  //0000100  /0x08
                              
  
int X;
int Y;


int en = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

  
  
  
  mainaddress = 82;
  Address  = (mainaddress/4)+1;
  Register = (mainaddress%4)-1;
  
  
  Byte1 = Address&63;
  Byte1 = Byte1+128;  // this is the first Byte
  
  Byte2 = 128;
  X = 0;
  Y = Address & 64;
  if(Y == 0){
    X = X + 64;
    
  }
  Y = Address & 128;
  
  if(Y == 0){
    X = X + 128;
    
  }
  Y = Address & 256;
  
  if(Y == 0){
    X = X + 256;
    
  }
  X = X >>2;
  
  Byte2 = Byte2 + X;
  
  
  
  Byte2 = Byte2 + (Register << 1);
  
  
  mX = (shiftValue ^ Byte2);     // | ^ &  ~
  
  mX2 = (shiftValue ^ mX);
  
  delay(100);
  
  if(en == 0){
    Serial.println("START");
    Serial.println("mainaddress: ");
    Serial.println(mainaddress);
    Serial.println("address: ");
    Serial.println(Address);
    Serial.println("Register: ");  
    Serial.println(Register);
    Serial.println("Byte1: ");  
    Serial.println(Byte1);
    Serial.println("Byte2: ");
    Serial.println(Byte2);
    Serial.println("mX: ");
    Serial.println(mX);
    Serial.println("mX2: ");
    Serial.println(mX2);
    Serial.println(" ");
    Serial.println("END");
    en = 1;
  }
     
}

