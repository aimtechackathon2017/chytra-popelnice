#include <TinyGPS.h>
#define ECHOPIN 10// Echo pin z HC-SC04 na pin
#define TRIGPIN 12 // Trig pin z HC-SC04 na pin 3
 
const byte interruptPin = 4;
volatile byte state = 0;

TinyGPS gps;
static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

void setup() {
  Serial.begin(9600);
  Serial.print("Testing TinyGPS library v. "); Serial.println(TinyGPS::library_version());
  Serial.println("by Mikal Hart");
  Serial.println();
  Serial.println("Sats HDOP Latitude  Longitude  Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum");
  Serial.println("          (deg)     (deg)      Age                      Age  (m)    --- from GPS ----  ---- to Moscow  ----  RX    RX        Fail");
  Serial.println("-------------------------------------------------------------------------------------------------------------------------------------");

  //Nastaví pin 2 jako vstupní
  pinMode(ECHOPIN, INPUT);
  //Nastaví pin 3 jako výstupní
  pinMode(TRIGPIN, OUTPUT);
  //pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, RISING);
  Serial1.begin(9600);
  Serial2.begin(9600);
}

void loop(){
  float flat, flon;
  unsigned long age, date, time, chars = 0;
  unsigned short sentences = 0, failed = 0;
  static const double LONDON_LAT = 50.090473, LONDON_LON = 14.401048;
  print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
  print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
  gps.f_get_position(&flat, &flon, &age);
  print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
  print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  print_date(gps);
  print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2);
  print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
  print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
  print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6);
  print_int(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0xFFFFFFFF : (unsigned long)TinyGPS::distance_between(flat, flon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
  print_float(flat == TinyGPS::GPS_INVALID_F_ANGLE ? TinyGPS::GPS_INVALID_F_ANGLE : TinyGPS::course_to(flat, flon, LONDON_LAT, LONDON_LON), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
  print_str(flat == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(TinyGPS::course_to(flat, flon, LONDON_LAT, LONDON_LON)), 6);
  gps.stats(&chars, &sentences, &failed);
  print_int(chars, 0xFFFFFFFF, 6);
  print_int(sentences, 0xFFFFFFFF, 10);
  print_int(failed, 0xFFFFFFFF, 9);
  Serial.println();
  smartdelay(1000);

  // Vyšle impuls do modulu HC-SR04
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);

  // Spočítá vzdálenost float
  float distance = pulseIn(ECHOPIN, HIGH);
  //distance= distance*0.017315f;
  Serial.print(distance); 
  //Serial.println("cm\n");
  delay(1000);

  if (state == 1) {
    Serial.println("Odesilam data do Sigfoxu");
    //Identify your self
    Serial2.print("AT$I=10\r\n");
    String identify = Serial2.readString();
    Serial.println("Identification done:" + identify);
    delay(1000);
    
    //Define variables
    char dist[12];
    char latitude[12];
    char longitude[12];
    dtostrf(flat, 9, 5, latitude);
    dtostrf(flon, 9, 5, longitude);
    dtostrf(distance, 9, 0, dist);
    Serial.println("Variables defined...");

    //Process latitude
    Serial2.print("AT$SF=");
    for(int i = 1; i < 9; i++){
      if(latitude[i] != '.')
        Serial2.print(latitude[i]);
    }
    Serial2.print('0');
    Serial2.print("\r\n");
    Serial.println("Latitude sent...");
    delay(1000);
    
    //Process longitude
    Serial2.print("AT$SF=");
    for(int i = 1; i < 9; i++){
      if(longitude[i] != '.')
        Serial2.print(longitude[i]);
    }
    Serial2.print('0');
    Serial2.print("\r\n");
    Serial.println("Longitude sent...");
    delay(1000);

    //Process distance
    Serial2.print("AT$SF=");
    for(int i = 1; i < 9; i++){
      if(dist[i] != ' ')
        Serial2.print(dist[i]);
      else
        Serial2.print('0');
    }
    Serial2.print("\r\n");
    Serial.println("Distance sent...");
    delay(1000);
    
    state = 0;
    delay(200);
  }
}

void blink() {state = 1;}

static void smartdelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (Serial1.available())
    gps.encode(Serial1.read());
  } while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec) {
  if (val == invalid) {
    while (len-- > 1) Serial.print('*');
    Serial.print(' ');
  }
  else {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i = flen; i < len; ++i) Serial.print(' ');
  }
  smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len) {
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

static void print_date(TinyGPS &gps) {
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print("********** ******** ");
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ", month, day, year, hour, minute, second);
    Serial.print(sz);
  }
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
    Serial.print(i < slen ? str[i] : ' ');
  smartdelay(0);
}

char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}