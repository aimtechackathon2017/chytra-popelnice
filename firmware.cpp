#define ECHOPIN 13 // Echo pin z HC-SC04 na pin
#define TRIGPIN 12 // Trig pin z HC-SC04 na pin 3

void setup() {
 //Nastaví sériovou komunikaci
 Serial.begin(9600);
 //Nastaví pin 2 jako vstupní
 pinMode(ECHOPIN, INPUT);
 //Nastaví pin 3 jako výstupní
 pinMode(TRIGPIN, OUTPUT);
}

void loop() {
 // Vyšle impuls do modulu HC-SR04
 digitalWrite(TRIGPIN, LOW);
 delayMicroseconds(2);
 digitalWrite(TRIGPIN, HIGH);
 delayMicroseconds(10);
 digitalWrite(TRIGPIN, LOW);

// Spočítá vzdálenost float
 float distance = pulseIn(ECHOPIN, HIGH);
 distance= distance*0.017315f;
 // odešle informace na sérivý port
 Serial.print(distance);
 Serial.print("cm\n");
 //počká 1 sekundu
 delay(1000);
}