unsigned char skiftAdresse = 101;
String sporByte1;
String sporStraight;
String sporTurn;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
 calcAdress();
}

void loop() {
  // put your main code here, to run repeatedly:
  //calcAdress();
 //Serial.println("test");
}

void calcAdress(){
  unsigned char address;
  unsigned char Register;
  unsigned char Byte1;
  unsigned char Byte2;
  
  address = (skiftAdresse/4)+1;
  Register = (skiftAdresse%4)-1;

  Byte1 = address&63;
  Byte1= Byte1+128;

  Byte2 = 128;

  unsigned char x = 0;
  unsigned char y = address & 64;  //bitwise operators en masse

  if(y == 0){ x = x+64;  } 
  
  y= address & 128;
  if(y == 0){ x = x+128; }

  y = address & 256;
  if(y==0){ x = x+256;}

  x = x>>2;

  Byte2 = Byte2 +x;
  Byte2 = Byte2 +(Register<<1);
  Serial.println(Byte2);
  String stringOne = String(Byte1,HEX);
  stringOne = String("0x" + stringOne);
  sporByte1= stringOne;

  String stringTwo = String((Byte2),HEX);
  stringTwo = String("0x" + stringTwo);
  sporTurn = stringTwo;

  Byte2++;
  String stringThree = String((Byte2),HEX);
  stringThree = String("0x" + stringThree);
  sporStraight = stringThree;
  
  Serial.println(sporByte1); 
  Serial.println(sporTurn); 
  Serial.println(sporStraight); 
}
