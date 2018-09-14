/* 
Digital Winding Clock - Arduino Clock for beginners   
No RTC, 
Just set it to exact time using the push buttons each time you switch on the clock 
Made by Techno (sǝɹoɟ ǝǝןuuɐ) 
Feel free to modify 

currently humidity sensor is too slow to work with janky time keeping method. try later with an rtc
IDEA: attach ac speaker and play simon says based on tone, then single display character can display number of rounds
*/ 
#include <LiquidCrystal.h>
#include "DHT.h" 
//#define DHTPIN 6
#define DHTTYPE DHT22
#define ALARM_BTN 2
#define BUZZER 7
#define HR_BTN 8
#define MIN_BTN 9
#define SET_BTN 10
#define Y_PIN 0
#define X_PIN 1
#define LED_PIN 13

LiquidCrystal 
lcd(12,11,6,5,4,3);
//DHT dht(DHTPIN, DHTTYPE); 
int h=12; 
int m; 
int s; 
int flag;
int a_h=8;
int a_m;
int a_flag=8;
bool time_mode = false; 
int TIME; 
int state1; 
int state2;
int state3;
int alarm_ring=0; 
volatile int btnState = 0;
const int SECOND = 996;
const int MIN = 50;
const int MAX = 970;
const int ALARM_TIME = 120;

enum Code {
  none,
  up,
  down,
  left,
  right,
  a,
  b,
  start
};

/**Specific variables for the current alarm type: konami requires the input of the konami code on a polling basis, simon says is self-explanatory**/
const Code konami[] = {none, up, up, down, down, left, right, left, right, a, b, start};
static const bool SIMON_ON = false;
const int simonLength = 5;
Code simon[simonLength];
static int simonDisplay;  //with CODE, determines whether display is showing player input or simon says sequence

//when in konami mode, tracks progress through the konami code, when in simon says mode, tracks which stage the player is on (-1 is failed/start, sequence starts at 0)
static int CODE = 0;
static bool ALARM_ON = true;

static Code codeState = none; //current input state
static Code currentDisplay = none;  //state to be displayed on lcd
/** In certain situations, such as when the alarm is not active, the display should only show no state, regardless of input to the device
**/

void setup() 
{ 
  codeState = none;
  lcd.begin(16,2);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(ALARM_BTN, INPUT);
  attachInterrupt(0, toggle_alarm, RISING);
  digitalWrite(LED_PIN, HIGH);
  //dht.begin();
  Serial.begin(115200);
  randomSeed(analogRead(3));
  randomizeCode(simon);
} 
void loop() 
{ 
  //display clock
  lcd.setCursor(0,0); 
  s=s+1; 
  lcd.print("TIME:"); 
  lcd.print(h); 
  lcd.print(":"); 
  lcd.print(m); 
  lcd.print(":"); 
  lcd.print(s); 
  if(flag<12)lcd.print("AM"); 
  if(flag==12)lcd.print("PM"); 
  if(flag>12)lcd.print("PM"); 
  if(flag==24)flag=0;

  //display alarm
  lcd.setCursor(0,1); 
  lcd.print("ALARM:");
  lcd.print(a_h);
  lcd.print(":");
  lcd.print(a_m);
  if (a_flag < 12) lcd.print("AM");
  else lcd.print("PM");

  lcd.print(" ");
  lcd.print(displayCode(currentDisplay));
  //lcd.print(CODE);
  //Serial.print(CODE);
   
  //display select mode
  if(time_mode)lcd.setCursor(14,0);
  else lcd.setCursor(14,1);
  lcd.print(" <");
  
  //1 second delay
  delay(SECOND/2);
  if(alarm_ring >=0){
    digitalWrite(BUZZER, LOW);
  }
  delay(SECOND/2);
   
  lcd.clear();

  
  if(s==60){ 
    s=0; 
    m=m+1; 
  } 
  if(m==60) 
  { 
    m=0; 
    h=h+1; 
    flag=flag+1; 
  } 
  if(h==13) 
  { 
   h=1; 
  }

  //------Alarm-----//
  if (flag==a_flag && m==a_m && alarm_ring==0 && ALARM_ON == true){
    //ring alarm
    alarm_ring = ALARM_TIME;
    
  }

  //---Alarm ringing---//
  if(alarm_ring > 0){
    alarm_ring-=1;
        digitalWrite(BUZZER, HIGH);

    if(alarm_ring == 0){
      digitalWrite(BUZZER, LOW);    
    }
  }

  //directional input
    readJoystick();
    if (codeState == none){ //if no joystick movement, check others
      //read hour and min buttons only if during konami code, also only trigger h/min if not
      if (digitalRead(HR_BTN) == 1){
        //a button
        codeState = a;
      }else if (digitalRead(MIN_BTN) == 1){
        //b button
        codeState = b;
      }else if (digitalRead(SET_BTN) == 1){ //originally sw_btn but doesn't work
        //start
        codeState = start;
      }
    }

    if (SIMON_ON){
      
      simonDisplay = 2*(CODE+1);
    }else{
      if (codeState == konami[CODE+1]){
        CODE++;
      }else if (codeState != none){
        CODE = 0;
      }
  
      if (CODE >= 11){
        //completed code, turn off alarm
        alarm_ring = 1;
        ALARM_ON = false;
        digitalWrite(LED_PIN, LOW);
        CODE ++;
      }
      if (CODE > 11){
        CODE = 0;
        codeState = none;
      }

      //set display value to current location in code;
      currentDisplay = konami[CODE];
    }
  
  //-------Time 
  // setting-------// 
  state3=digitalRead(SET_BTN);
  if(state3==1 && CODE <= 0){
    time_mode = !time_mode;
  }

  //---Set Hour---//
  state1=digitalRead(HR_BTN); 
  if(state1==1 && CODE <= 0) 
  { 
    if(time_mode){
      //set time
      h=h+1; 
      flag=flag+1; 
      /*if(flag<12)lcd.print("AM"); 
      if(flag==12)lcd.print("PM"); 
      if(flag>12)lcd.print("PM"); */
      if(flag==24)flag=0; 
      if(h==13)h=1; 
    }else{
      //set alarm
      a_h+=1;
      a_flag+=1;
      if(a_flag==24)a_flag=0;
      if(a_h==13)a_h=1;
    }
  } 
  //---Set Minute---//
  state2=digitalRead(MIN_BTN); 
  if(state2==1 && CODE <= 0){ 
    if(time_mode){
      //set time
      s=0; 
      m=m+1; 
    }else{
      //set alarm
      a_m+=1;
      if(a_m==60){
        a_m=0;
        a_h+=1;
        a_flag+=1;
      }
    }
  } 
} 

/*
 * Returns character to display on lcd
 */
char displayCode(Code code){
  switch(code){
    case up:
      return '^';
    case down:
      return 'v';
    case left:
      return '<';
    case right:
      return '>';
    case a:
      return 'A';
    case b:
      return 'B';
    case start:
      return '*';
    case none:
    default:
    return 'x';
  }
}

/*
 * Reads joystick directional input, assigns to the code state (up, down, left, right, or none)
 */
void readJoystick(){
  int x = analogRead(X_PIN);
  int y = analogRead(Y_PIN);
  Serial.print(x);

  if (y > MIN && y < MAX){
    //possible horizontal
    if (x < MIN){
      codeState = left;
    }else if (x > MAX){
      codeState = right;
    }else{
      codeState = none;
    }
  }else if (x > MIN && x < MAX){
    //possible vertical
    if (y < MIN){
      codeState = up;
    }else if(y > MAX){
      codeState = down;
    }else{
      codeState = none;
    }
  }
}

/**
 * Toggles on board LED indicating whether the alarm is currently turned on or off
**/
void toggle_alarm(){
  btnState = digitalRead(ALARM_BTN);
  ALARM_ON = !ALARM_ON;
  if (ALARM_ON){
    digitalWrite(LED_PIN, HIGH);
  }else{
    digitalWrite(LED_PIN, LOW);
  }
}

/**
 * Generates a sequence of random inputs
 * Uses static int simonLength for length of array, changes parameter array by pass by pointer
**/
void randomizeCode(Code* arr){
  for (int i = 0; i < simonLength; ++i){
    arr[i] = (Code)random(1,7);
  }
}

