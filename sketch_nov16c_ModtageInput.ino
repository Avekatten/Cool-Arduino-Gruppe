/*
 * Not done, yet.
 */

//modtager:   tal;    Adresse   36;40;102   // op til 3 cifre
//modtagerEVT:   tekst:  Sporskifte position   Straight / Turn    //skal kune vises, 
//                men styres fra kode ikke fra start da det er svært at læse om sporet står på den ene
//modtager:   tekst:  Retning   Clockwise / counter Clockwise   C vs CC
//modtagerIKKE:   forwards and backwards, vi starter med at togene altid kører fremefter.

//modtagerMÅSKE:    TogPrioritet: tal 1 eller 0. 
//                  Hvis begge tog står på inner/outer skal det defineres hvilket tog der må køre først?
//Modtager start komando
//modtager pause komando?

int tog1 = 0; //36
int tog2 = 0; //40
boolean tog1CC = true;
boolean tog2CC = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  
  
  if (Serial.available() )
  {
    char input = Serial.read();
    //skrivUd(input);

    //tjekker om input er tomt med en ny linie
    if(input == 10)
    {
      skrivDSB();
    }
    else if(isDigit(input))
    {
        tog1 = input; //her skal der være kode til at tjekke og holde styr på flere tal efter hinanden i en int, der skal gemmes Over denne if statement.
        Serial.println("tog1 er blevet sadt til");
        Serial.println(tog1);
        Serial.println();
    }else{
      Serial.println("input kan ikke læses korrekt");
    }
    
  }
  
}


void skrivDSB(){

  skrivPlads(4);
  // 22 linier høj
  Serial.println("         ** Tog Banen **     ");
  Serial.println("                             ");
  Serial.println("          _____________      Outer Track");
  Serial.println("         /             \\     ");
  Serial.println("        /               \\    ");
  Serial.println("       /                 \\   ");
  Serial.println("      |   _____________   |  Inner Track");
  Serial.println("      |  /             \\  |  ");
  Serial.println("      | /               \\ |  ");
  Serial.println("  S1  |/                 \\|  S2");
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
  Serial.println("         \\_____________/     Long Track");
  skrivPlads(8); //18 i alt

  skrivInfo(); // 3 lige nu
  
  skrivPlads(7);
  

}

void skrivInfo(){
  Serial.println("           Adresse       Retning");
  Serial.print(" Tog 1:    ");
  Serial.print(tog1);
  Serial.print("             ");
  
  if(tog1CC){
    Serial.println("CounterClockwise [CC]");
  }else{
    Serial.println("Clockwise [c]");
  }
  //Serial.println();
  //Serial.println();
  Serial.print(" Tog 2:    ");
  Serial.print(tog2);
  Serial.print("             ");
  if(tog2CC){
    Serial.println("CounterClockwise [CC]");
  }else{
    Serial.println("Clockwise [C]");
  }
}

void skrivPlads(int antal){

  int i;
  for(i = 0; i < antal; i++)
  {
    Serial.println();    
  }
    
}


//bruges til at finde antal linier på serial monitor i højden
//min mac viser 44 linier på monitoren med de nuværende font indstillinger.
void skrivTal(){
  int i;
  for(i = 0; i < 100; i++)
  {
    Serial.println(i);    
  }
  
}

