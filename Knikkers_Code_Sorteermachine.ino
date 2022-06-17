//Deze code gaat samen met de code voor het bakjessysteem, de 2 arduinos communiceren met elkaar
//door 2 pinnen die een PWM signaal krijgen of een Bool signaal
//De codes zijn opgedeeld in 2 arduinos zodat het als 2 losse delen beschouwd kan worden
//dit is makkelijker met debuggen en problemen oplossen
//De noodstop werkt tijdens de delays

#include <Servo.h> //Servo library toegevoegd

//Alle servo van dit systeem worden gedeclareerd:
Servo sorteerServoK; // de sorteerservo voor de kleine knikkers
Servo doseerServoK; // de doseerservo voor de kleine knikkers
Servo sorteerServoG; // de sorteerservo voor de grote knikkers
Servo doseerServoG; // de doseerservo voor de grote knikkers
Servo stopper_1K;
Servo stopper_2K;
Servo stopper_1G;
Servo stopper_2G;

// Alle pinnen worden in variabelen gezet:
const uint8_t stopper_1GPin = 12; //servo pin
const uint8_t stopper_2GPin = 11; //servo pin
const uint8_t sorteerServoGPin = 10; //servo pin
const uint8_t doseerServoGPin = 9; //servo pin
const uint8_t stopper_1KPin = 8; //servo pin
const uint8_t stopper_2KPin = 7; //servo pin
const uint8_t sorteerServoKPin = 6; //servo pin
const uint8_t doseerServoKPin = 5; //servo pin
const uint8_t noodstopPin = 2; //Noodstop pin
const uint8_t S0 = 44;  //kleurensensor pin
const uint8_t S1 = 30;  //kleurensensor pin
const uint8_t S2 = 31;  //kleurensensor pin
const uint8_t S3 = 32;  //kleurensensor pin
const uint8_t sensorOut = 33;  //kleurensensor pin
const uint8_t comPinOut = 25; //de pin die wordt gebruikt om te communiceren naar de andere arduino (out)
const uint8_t comPinIn = A0; //de pin die wordt gebruikt om te communiceren naar de andere arduino (in)

// Alle globale variabele worden hier gedefineerd:
int glas = 0;
int metaal = 0;
int plastic = 0;
int hout = 0;
int bakType;
bool flag = 0; //flag om de waarde te dragen of er een bakje voor de machine staat of niet
uint8_t glas_aantal;
uint8_t metaal_aantal;
uint8_t plastic_aantal;
uint8_t hout_aantal;
uint8_t ticker;
volatile bool noodstopPlaatsgevonden = 0;
int stopper_1Khoekopen = 105;
int stopper_1Khoekdicht = 180;
int stopper_2Khoekopen = 105;
int stopper_2Khoekdicht = 180;
int sorteerServoKhoeklinks = 102;
int sorteerServoKhoekrechts = 55;

//De hoeveelheid knikkers per bakje:
int aluminium[4] = {1 , 0, 0, 2}; //Glas,Metaal,Plastic,Hout
int doorzichtig[4] = {0, 2, 1, 0}; //Glas,Metaal,Plastic,Hout
int PVC[4] = {1, 1, 1, 0}; //Glas,Metaal,Plastic,Hout

//Callibratie Kleuren sensor hout en plastic en geen:
const int Hout_RedgemLaag = 300;
const int Hout_RedgemHoog = 300;

const int Hout_GreengemLaag = 300;
const int Hout_GreengemHoog = 300;

const int Plastic_RedgemLaag = 300;
const int Plastic_RedgemHoog = 300;

const int Plastic_GreengemLaag = 300;
const int Plastic_GreengemHoog = 300;

const int Geen_RedgemLaag = 300;
const int Geen_RedgemHoog = 300;

const int Geen_GreengemLaag = 300;
const int Geen_GreengemHoog = 300;

void setup() {
  //De setup:
  Serial.begin(9600); // Serial Communication is starting with 9600 of baudrate speed

  //De interupt voor de noodstop:
  attachInterrupt (digitalPinToInterrupt (noodstopPin), noodstop, CHANGE); 
  
  //Alle pinmodes worden hier gedefineerd:
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(S0, OUTPUT);
  pinMode(sensorOut, INPUT);
  pinMode(noodstop, INPUT);
  pinMode(comPinOut, OUTPUT);
  pinMode(comPinIn, INPUT);
  pinMode(stopper_1GPin, OUTPUT);
  pinMode(stopper_2GPin, OUTPUT);
  pinMode(sorteerServoGPin, OUTPUT);
  pinMode(doseerServoGPin, OUTPUT);
  pinMode(stopper_1KPin, OUTPUT);
  pinMode(stopper_2KPin, OUTPUT);
  pinMode(sorteerServoKPin, OUTPUT);
  pinMode(doseerServoKPin, OUTPUT);  
  
  //Alle Servos worden verbinden met de juiste pin:
  stopper_1G.attach(stopper_1GPin);
  stopper_2G.attach(stopper_2GPin);
  sorteerServoG.attach(sorteerServoGPin);
  doseerServoG.attach(doseerServoGPin);
  stopper_1K.attach(stopper_1KPin);
  stopper_2K.attach(stopper_2KPin);
  sorteerServoK.attach(sorteerServoKPin);
  doseerServoK.attach(doseerServoKPin);
  
  //aanzetten motoren kleurensensor:
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  //Alle actuatoren worden naar de beginpositie gebracht:
  //alle servo's dicht:
  stopper_1G.write(70); 
  stopper_2G.write(93);
  stopper_1K.write(95);
  stopper_2K.write(96);
  
  Serial.println("Setup Complete.");
}

void loop() {
  //de main code:
  knikker_Groot(); //meten grote knikkers boven en sorteren
  knikker_Klein(); //meten kleine knikkers onder en sorteren
  flag = bakjes(); //meten bakjes gedeelte en registreren of er een bakje staat
  if (check() == HIGH && flag == HIGH ) { //checken of er genoeg knikkers in de buffer zitten en of er een bakje staat
    doseer(); //doseren knikkers
    digitalWrite(comPinOut,HIGH); //communiceren dat het bakje vol is
  }
} 

void noodstop(){
  //de noodstop functie die vast staat aan een interrupt pin:
  bool noodstopStatus = digitalRead(noodstopPin);
  if(noodstopStatus == HIGH){
    noodstopPlaatsgevonden = 1; //variabele om te weten of er in het programma eventueel metingen opnieuw gedaan moeten worden
    Serial.println("Noodstop geactiveerd");
  }
  while(noodstopStatus == HIGH){
    noodstopStatus = digitalRead(noodstopPin); //loop om de arduino te freezen
  }
  if(noodstopPlaatsgevonden == 1){
    //alle voorheen geactiveerde actuatoren weer aanzetten
    Serial.println("Noodstop gedeactiveerd");
  }
}

//de functie voor het sorteren van de grote knikkers:
void knikker_Groot() {
  Serial.println("meten gote knikkers"); //hier moet nog een 3de positie inkomen :knikker pakken, laten scennen, loslaten
  delay(500);
  stopper_1G.write(122); //poortje open
  delay(145);
  stopper_1G.write(70); //poortje dicht
  delay(500);
  knikker_lezen();
  delay(2000);
}

// de functie voor het lezen van de kleurensensor en het bijvoegen van de digitale buffers:
void knikker_lezen() {
  int Bluegem;
  int Redgem;
  int Greengem;
//Een grote meting wordt gedaan en vervolgens wordt een gemiddelde bepaald:
foutmeting:
  do{
  noodstopPlaatsgevonden = 0; //noodstopplaatsgevonden resetten voor als er iets gebeurt tijdens de meting
  int Red;
  int Green;
  int Blue;
  uint32_t Rtot = 0;
  uint32_t Gtot = 0;
  uint32_t Btot = 0;
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  delay(200);
  for (int i = 0; i < 100; i++) { //meten 100 rode waardes voor gemiddelde
    Red = pulseIn(sensorOut, LOW);
    Rtot = Red + Rtot;
  }
  Redgem = Rtot / 100; //gemiddelde berekenen
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  delay(200);
  for (int i = 0; i < 100; i++) { //meten 100 groene waardes voor gemiddelde
    Green = pulseIn(sensorOut, LOW);
    Gtot = Green + Gtot;
    //Serial.println(Green);
    //Serial.println(Gtot);
  }
  Greengem = Gtot / 100; //gemiddelde berekenen digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  delay(200);
  for (int i = 0; i < 100; i++) { //meten 100 blauwe waardes voor gemiddelde
    Blue = pulseIn(sensorOut, LOW);
    Btot = Blue + Btot;
  }
  Bluegem = Btot / 100; //gemiddelde berekenen
  }while(noodstopPlaatsgevonden == 1); //loop herhalen als de noodstup ingedrukt is
  
  Serial.print("Rode Waarde: ");
  Serial.print(Redgem);
  Serial.print("Groene Waarde: ");
  Serial.print(Greengem);
  Serial.print("Blauwe Waarde: ");
  Serial.println(Bluegem);

//De waarden van de kleurensensor worden vergeleken met de waarden van verschillende knikkers:
      //de waarden voor als er een Houten knikker in zit:
      if (Redgem > Hout_RedgemLaag && Redgem < Hout_RedgemHoog && Greengem > Hout_GreengemLaag && Greengem < Hout_GreengemHoog) { //checken hout knikker
        hout++;
        sorteerServoG.write(0);
        stopper_2G.write(175);
        delay(500);
        stopper_2G.write(93);
        Serial.println("Houten knikker gesorteerd bij de Grote knikkers");
      }
      //de waarden voor als er een Plastic knikker in zit:
      else if (Redgem > Plastic_RedgemLaag && Redgem < Plastic_RedgemHoog && Greengem > Plastic_GreengemLaag && Greengem < Plastic_GreengemHoog) { //checken plastic knikker
        plastic++;
        stopper_2G.write(175);
        sorteerServoG.write(69);
        delay(500);
        stopper_2G.write(93);
        Serial.println("Plastic knikker gesorteerd bij de Grote knikkers");
      }
      //de waarden voor als er geen knikker in zit:
      else if (Redgem > Geen_RedgemLaag && Redgem < Geen_RedgemHoog && Greengem > Geen_GreengemLaag && Greengem < Geen_GreengemHoog) {
        Serial.println("Geen knikker gesorteerd bij de Grote knikkers");
      }
      else {
        Serial.println("Foutmeting bij de Grote knikkers");
        goto foutmeting; //hij meet de knikker opnieuw totdat er een goede meting is
      }
  }

//de functie voor het sorteren van de kleine knikkers:
void knikker_Klein() {
 //Poortje openen en sluiten, om de knikkers te doseren:
  delay(500);
  Serial.println("meten kleine knikkers");
  stopper_2K.write(stopper_2Khoekdicht); //servo 2 sluiten
  delay(500);
  stopper_1K.write(stopper_2Khoekopen); //eerste poortje open
  delay(1000);
  stopper_1K.write(stopper_2Khoekdicht); //eerste poortje dicht
  delay(500);
  
  //checken of er een knikker in het poortje zit:
  bool knikkerInPoortje = digitalRead(knikkerKleinDetectiePin);
  knikkerInPoortje = HIGH;
  bool sensor;
  if(knikkerInPoortje == HIGH){
    sorteerServoK.write(sorteerServoKhoeklinks);//standaard naar de galzen buffer sturen
    delay(200);
    stopper_2K.write(stopper_2Khoekopen); //servo 2 openen
    unsigned long Tijd1 = millis();
    unsigned long Tijd2;
    
    do{
      // loopje herhalen totdat een bepaalde tijd is bereikt
      sensor = digitalRead(knikkerKleinMetaalDetectiePin);
      Tijd2 = millis();
    }while(Tijd2-Tijd1 <= 2000 && sensor == LOW);
    
    if(sensor == LOW){
      //als de sensor laag is was het een glazen knikker
      Serial.println("Glazen knikker gesorteerd bij de kleine knikkers");
    }
    else if(sensor == HIGH){
      //als de sensor hoog is was het een metalen knikker
      sorteerServoK.write(sorteerServoKhoekrechts);//naar de metalen buffer sturen
      Serial.println("Metalen knikker gesorteerd bij de kleine knikkers");      
    }
  }
  else{
    Serial.println("Geen knikker gesorteerd bij de kleine knikkers");
  }

}

// kijken of er genoeg knikkers in de buffers zitten om de bakjes te vullen:
int check() {
  switch (bakType) { //glas,metaal,plastic.hout
    case 1:
      if (glas >= aluminium[0] && metaal >= aluminium[1] && plastic >= aluminium[2] && hout >= aluminium[3]) {
        return 1;
      }
      else {
        return 0;
      }
    case 2:
      if (glas >= doorzichtig[0] && metaal >= doorzichtig[1] && plastic >= doorzichtig[2] && hout >= doorzichtig[4]) {
        return 1;
      }
      else {
        return 0;
      }
    case 3:
      if (glas >= PVC[0] && metaal >= PVC[1] && plastic >= PVC[2] && hout >= PVC[3]) {
        return 1;
      }
      else {
        return 0;
      }
  }
}

//hier wordt gekeken en doorgevoerd welk bakje er voor de machine staat:
int bakjes() {
 int soortBakje = analogRead(A0); //lezen wat de andere arduino stuurt
    if ( soortBakje >= 200 && soortBakje <= 400) {
      Serial.println(" Doorzichtig bakje !! ");
      bakType = 1;
      return 1; //terugsturen naar loop dat er een bakje is
    }
    else if (soortBakje >= 500 && soortBakje <= 700) {
      Serial.println(" PVC bakje !! ");
      bakType = 2;
      return 1; //terugsturen naar loop dat er een bakje is
    }
    else if ( soortBakje >= 800 && soortBakje <= 1000) {
      Serial.println(" aluminium bakje !! ");
      bakType = 3;
      return 1; //terugsturen naar loop dat er een bakje is
    } 
    else if (soortBakje == 0){
      Serial.println(" Geen bakje ");
      digitalWrite(comPinOut,LOW); //communiceren dat er een nieuw bakje nodig is
      return 0; //terugsturen naar loop dat er geen bakje is
  }
}

//Hier worden alle knikkers naar het bakje gesorteerd:
void doseer() {
  switch (bakType) { //verwerken welke knikkers er in moeten vanuit de varibelen
    case 1:
      glas_aantal = aluminium[0];
      metaal_aantal = aluminium[1];
      plastic_aantal = aluminium[2];
      hout_aantal = aluminium[3];
      break;
    case 2:
      glas_aantal = doorzichtig[0];
      metaal_aantal = doorzichtig[1];
      plastic_aantal = doorzichtig[2];
      hout_aantal = doorzichtig[3];
      break;
    case 3:
      glas_aantal = PVC[0];
      metaal_aantal = PVC[1];
      plastic_aantal = PVC[2];
      hout_aantal = PVC[3];
      break;
  }
  //hier wordt per knikkersoort gedoseerd:
  Serial.println("Bakje aan het vullen");
  ticker = 1;
  knikkerCheck: //een goto om als er genoeg knikkers per soort gedoseerd zijn opnieuw door alle if statements te lopen
  //glazen knikkers:
  if (glas_aantal = !0 && ticker == 1) {
    doseerServoK.write(0);
    delay(1000);
    doseerServoK.write(84);
    glas_aantal--;
    if (glas_aantal == 0) {
      ticker = 2; //als er niet meer van deze soort knikkers nodig is wordt de volgende knikker gedaan
    }
    goto knikkerCheck;
  }
  //metalen knikkers:
  if (metaal_aantal = !0 && ticker == 2) {
    doseerServoK.write(180);
    delay(1000);
    doseerServoK.write(84);
    metaal_aantal--;
    if (metaal_aantal == 0)
      ticker = 3; //als er niet meer van deze soort knikkers nodig is wordt de volgende knikker gedaan
    goto knikkerCheck;
  }
  //plastic knikkers:
  if (plastic_aantal = !0 && ticker == 3) {
    doseerServoG.write(0);
    delay(1000);
    doseerServoG.write(92);
    plastic_aantal--;
    if (plastic_aantal == 0)
      ticker = 4; //als er niet meer van deze soort knikkers nodig is wordt de volgende knikker gedaan
    goto knikkerCheck;
  }
  //houten knikkers:
  if (hout_aantal = !0 && ticker == 4) {
    doseerServoG.write(180);
    delay(1000);
    doseerServoG.write(92);
    hout_aantal--;
    if (hout_aantal == 0)
      ticker = 0; //als er niet meer van deze soort knikkers nodig is wordt de volgende knikker gedaan
    goto knikkerCheck;
  }
  delay(1000); //delay om de knikker genoeg tijd te geven om naar het bakje te rollen
  digitalWrite(comPinOut,HIGH); //communiceren met andere arduino dat het bakje vol is
  Serial.println("Bakje gevuld");
} 
//wh24 groep 3
