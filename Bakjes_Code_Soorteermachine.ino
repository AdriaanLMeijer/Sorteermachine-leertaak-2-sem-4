//Deze code gaat samen met de code voor het knikkersysteem, de 2 arduinos communiceren met elkaar
//door 2 pinnen die een PWM signaal krijgen of een Bool signaal
//De codes zijn opgedeeld in 2 arduinos zodat het als 2 losse delen beschouwd kan worden
//dit is makkelijker met debuggen en problemen oplossen
//De noodstop werkt tijdens de delays
//test

#include <Servo.h> //Servo library toegevoegd

//Alle servo van dit systeem worden gedeclareerd:
Servo draaiServo;
Servo klepServo1; // linker servo vanaf de trechter
Servo klepServo2; // rechter servo vanaf de trechter

// Alle pinnen worden in variabelen gezet:
const int echoPin = 47; //de echopin van de ultrasoon voor de bakjes orientatie
const int trigPin = 49; //de trigpin van de ultrasoon voor de bakjes orientatie
const int bandMotorPin = 35; //de pin die aangesloten staat op de MOSFET van de bandmotor
const int trechterMotorPin = 43; //de pin die aangesloten staat op de MOSFET van de trechtermotor
const int draaiServoPin = 10; //de pin die is aangesloten op de servo van de draaischijf voor het doseren
const int klepServo1Pin = 10; //de pin die is aangesloten op de linker klep
const int klepServo2Pin = 10; //de pin die is aangesloten op de rechter klep
const int LDRPin = 10; //de pin die is aangesloten op de LDR
const int comPinOut = 10; //de pin die wordt gebruikt om te communiceren naar de andere arduino (out)
const int comPinIn = 10; //de pin die wordt gebruikt om te communiceren naar de andere arduino (in)
const int noodstopPin = 2; //de noodstoppin

// Alle globale variabele worden hier gedefineerd:
float afstand; // variable for the distance measurement of the ultrasonic sensor
int LDRWaarde;
unsigned long tijdSindsBakje = millis();
volatile bool noodstopPlaatsgevonden = 0;
bool trechterMotorStatus;
bool bandMotorStatus;

// de wachttijd voot het transporteren van een bakje naar het knikkersysteem:
const int wachttijdBakje = 5000; // 5 seconden

//Callibratie Ultrasoon sensor:
const int ultrasoon_BakjeLaag = 6; // de lage afstand in cm voor wanneer een bakje met de open kant naar de sensor ligt
const int ultrasoon_BakjeHoog = 10; // de hoge afstand in cm voor wanneer een bakje met de open kant naar de sensor ligt

//Callibratie LDR sensor:
const int LDR_bakjeDoorzichtig = 400;
const int LDR_bakjePVC = 300;
const int LDR_bakjeAluminium = 450;

void setup() {
  //De setup:
  Serial.begin(9600); // Serial Communication is starting with 9600 of baudrate speed

  //De interupt voor de noodstop:
  attachInterrupt (digitalPinToInterrupt (noodstopPin), noodstop, CHANGE); 
  
  //Alle pinmodes worden hier gedefineerd:
  pinMode(trigPin, OUTPUT); // Sets the Pin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the Pin as an INPUT
  pinMode(bandMotorPin, OUTPUT);
  pinMode(trechterMotorPin, OUTPUT);
  pinMode(draaiServoPin, OUTPUT);
  pinMode(klepServo1Pin, OUTPUT);
  pinMode(klepServo2Pin, OUTPUT);
  pinMode(LDRPin, INPUT);
  pinMode(comPinOut, OUTPUT);
  pinMode(comPinIn, INPUT);

  //Alle Servos worden verbinden met de juiste pin:
  draaiServo.attach(draaiServoPin);
  klepServo1.attach(klepServo1Pin);
  klepServo2.attach(klepServo2Pin);

  //Alle actuatoren worden naar de beginpositie gebracht:
  digitalWrite(trechterMotorPin, LOW);
  digitalWrite(bandMotorPin, LOW);
  analogWrite(comPinOut, 0);
  draaiServo.write(160); //dit is de posietie waarbij het gat bij de trechter zit
  klepServo1.write(140); //dit is de positie waarbij de klepjes naar beneden staan
  klepServo2.write(20); //dit is de positie waarbij de klepjes naar beneden staan
}

void loop() {
  //de main code:
  //Eerst wordt er gekeken of er al een bakje op de band staat en of deze vol is of niet
  LDRWaarde = analogRead(LDRPin); //LDR wordt uitgelezen
  //if statements voor kijken of er een bakje voor de ldr staat en welke dit is:
  if(LDRWaarde >= LDR_bakjeDoorzichtig - 10 && LDRWaarde <= LDR_bakjeDoorzichtig + 10 ){
    digitalWrite(bandMotorPin, LOW);
    analogWrite(comPinOut, 300); //communiceren dat er een Doorzichtig bakje staat
    afvoerBakje();
    }
  else if(LDRWaarde >= LDR_bakjePVC - 10 && LDRWaarde <= LDR_bakjePVC + 10 ){
    digitalWrite(bandMotorPin, LOW);
    analogWrite(comPinOut, 600); //communiceren dat er een PVC bakje staat
    afvoerBakje();
    }  
  else if(LDRWaarde >= LDR_bakjeAluminium - 10 && LDRWaarde <= LDR_bakjeAluminium + 10 ){
    digitalWrite(bandMotorPin, LOW);
    analogWrite(comPinOut, 900); //communiceren dat er een Aluminium bakje staat
    afvoerBakje();
    }
  else{
    analogWrite(comPinOut, 0); //communiceren dat er geen bakje staat
    
    //checken of de tijd sinds het bakjes programma langer is geweest dan de wachttijd
    if(millis()- tijdSindsBakje >= wachttijdBakje){
      bakjesCode(); // een bakje op de rolband leggen en de tijdSindsBakje resetten
      }
    else{
      digitalWrite(bandMotorPin, HIGH); //motor van de band aanzetten omdat het bakje niet direct voor het gat staat
    }
    }
}

void noodstop(){
  //de noodstop functie die vast staat aan een interrupt pin:
  bool noodstopStatus = digitalRead(noodstopPin);
  if(noodstopStatus == HIGH){
    trechterMotorStatus = digitalRead(trechterMotorPin); //status van motoren registreren
    bandMotorStatus = digitalRead(bandMotorPin);
    digitalWrite(trechterMotorPin, LOW); //motoren uitzetten
    digitalWrite(bandMotorPin, LOW);
    noodstopPlaatsgevonden = 1; //variabele om te weten of er in het programma eventueel metingen opnieuw gedaan moeten worden
  }
  while(noodstopStatus == HIGH){
    noodstopStatus = digitalRead(noodstopPin); //loop om de arduino te freezen
  }
  if(noodstopPlaatsgevonden == 1){
    //alle voorheen geactiveerde actuatoren weer aanzetten
    digitalWrite(trechterMotorPin, trechterMotorStatus);
    digitalWrite(bandMotorPin, bandMotorStatus);
  }
}

void bakjesCode(){
  //de functie van het plaatsen van een bakje op de band:
  digitalWrite(trechterMotorPin, HIGH); //vibratie motor op trechter wordt aangezet
  klepServo1.write(42); //dit is de positie waarbij het bakje op de klepjes moet landen
  klepServo2.write(92); //dit is de positie waarbij het bakje op de klepjes moet landen
  delay(3000);//wachttijd voor trechter
  
  // doseerschijf  laten draaien:
  draaiServo.write(57); //gatpositie
  delay(1000);
  draaiServo.write(52); //iets verder
  delay(1000);
  draaiServo.write(57); //gatpositie
  delay(1000);
  draaiServo.write(160); //beginpositie

  //Ultrasoon uitlezen en bepalen orientatie bakje
  do{
  ultrasoonOrientatie();
  }while(noodstopPlaatsgevonden == 1);
  
  Serial.print("afstand bakje: ");
  Serial.print(afstand);
  Serial.println(" cm");
  
  if (afstand >= ultrasoon_BakjeLaag && afstand <= ultrasoon_BakjeHoog) { 
    //bakje laten zakken met opening naar ultrasoon:
    klepServo1.write(140);
    klepServo2.write(92);
    delay(200);
    klepServo1.write(140);
    klepServo2.write(70);
    delay(200);
    klepServo2.write(60);
    delay(200);
    klepServo2.write(50);
    delay(200);
    klepServo2.write(40);
    delay(200);
    klepServo2.write(30);
    delay(200);
    klepServo2.write(20);
  }
  else { 
    //servo laten zakken met dichte kant naar ultrasoon:
    klepServo1.write(42);
    klepServo2.write(20);
    delay(200);
    klepServo1.write(50);
    klepServo2.write(20);
    delay(200);
    klepServo1.write(60);
    delay(200);
    klepServo1.write(70);
    delay(200);
    klepServo1.write(80);
    delay(200);
    klepServo1.write(90);
    delay(200);
    klepServo1.write(100);
    delay(200);
    klepServo1.write(110);
    delay(200);
    klepServo1.write(121);
    delay(200);
    klepServo1.write(140);
    delay(200);
  }
  tijdSindsBakje = millis(); //omdat er net een bakje opgezet is moet dit geregistreerd worden
  }

void afvoerBakje(){
  bool bakjeVol;
  do {
  bakjeVol = digitalRead(comPinIn);
  } while (bakjeVol == LOW); //terwijl de andere arduino aangeeft dat het bakje leeg is blijven loopen
  digitalWrite(bandMotorPin, HIGH); //transportband aan
  delay(1000); //wachten tot bakje is afgevoerd
  digitalWrite(bandMotorPin, LOW); //transportband uit
}

void ultrasoonOrientatie(){ 
  //de functie om de ultrasoon van de bakjes orientatie uit te lezen:
  noodstopPlaatsgevonden = 0; //reset de noodstopplaatsgevonden, zodat als er tijdens de meting een noodstop plaats vind de meting opnieuw gaat
  // Clears the trigPin condition
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  float duration = pulseIn(echoPin, HIGH);
  // Calculating the distance in cm
  afstand = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  }
