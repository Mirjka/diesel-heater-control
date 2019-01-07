///////////////////////////////////////
////        Offene Fragen          ////
///////////////////////////////////////
// warum sind Variablen-Änderungen oder analogRead nicht in den cases nutzbar
// Temperatursensor (Variable Tc) konnte bisher nicht eingebunden werden
// millis() konnte nicht als delay in cases genutz werden (if millis() > (resetTime2+5000))


///////////////////////////////////////
////        Offene Punkte          ////
///////////////////////////////////////
// Einbindung eines Knopfes zur Deaktivierung der Dieselpumpe in Vorheizphase, 
// falls nach mehreren Startversuchen zu viel Diesel in Brennkammer


///////////////////////////////////////
////   Include LCD 16x2 DISPLAY    ////
///////////////////////////////////////
#include<Wire.h>
#include<LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);


///////////////////////////////////////
////          Constants            ////
///////////////////////////////////////
const int FAN_LED = 2;                  // displays aktive fan
const int DIESEL_LED = 3;               // display open magnetic valve for DIESEL
const int PREHEATING_LED = 4;           // displays aktive preheater
const int HEATING_LED = 5;              // displays case HEATING
const int ERROR_LED =6;                 // desplay ERROR (blinking)
const int AKTIVE_LED = 7;               // displays CPU-Activity

const int MAIN_SWITCH = 12;             // On / Off Main switch
const int RESET = 11;                   // Softwarereset (go to case OFF)
const int TEMP_SENSOR = 13;             // displays combustion chamber = hot
const int DELAY_MAIN_SWITCH = 10;       // if switch is aktive, preheatingphases are on hold for manual intervention

const int DelayLuefter = 20;            // 1. Delay preheatingphases: 20 * 100 = wait 2 sec until preheating starts
const int DelayVorgluehenTrocken = 100; // 2. Delay preheatingphases: 100 * 100 = wait 10 sec until magnetig valve opens
const int DelayVorgluehenNass = 100;    // 3. Delay preheatingphases: 100 * 100 = wait 10 sec until combustion chamber is hot

int hot;                                // hot = digitalRead(TEMP_SENSOR) == 0;
int cold;                               // cold = digitalRead(TEMP_SENSOR) == 1;
int reset;                              // reset = digitalRead(RESET) == 0;

int mainSwitchState;                    // actual Input MainSwitch
int lastmainSwitchState = HIGH;         // previous Input MainSwitch

int mainSwitchInt = 0;                  // first State MainSwitch after Systemstart
int startInt = 1;                       // Variable only for first Systemstart
int i;                                  // Variable for for-loops
                                        // (they are used as a delay for preheatingphases)


///////////////////////////////////////
////          Variables            ////
///////////////////////////////////////
unsigned long lastDebounceTime = 0;     // last time stamp when main switch is on
unsigned long debounceDelay = 100;

unsigned long resetTime;
unsigned long resetDelay = 200;
unsigned long betriebsDelay = 500;


///////////////////////////////////////
////         State machine         ////
///////////////////////////////////////

// Lists of States
enum State {OFF, PREHEATING, HEATING, SHUT_DOWN, ERROR};

State currentState;
State lastState = OFF;

void setup() {
  lcd.init();
  pinMode (AKTIVE_LED, OUTPUT);
  pinMode (FAN_LED, OUTPUT);
  pinMode (DIESEL_LED, OUTPUT);
  pinMode (PREHEATING_LED, OUTPUT);
  pinMode (HEATING_LED, OUTPUT);
  pinMode (ERROR_LED, OUTPUT);
  
  pinMode (MAIN_SWITCH, INPUT_PULLUP);
  pinMode (TEMP_SENSOR, INPUT_PULLUP);    // Temp sensor (Flammenwächter), 0 or 24 V signal
  pinMode (RESET, INPUT_PULLUP);
  pinMode (DELAY_MAIN_SWITCH, INPUT_PULLUP);
  
  Serial.begin(9600);
}

void loop() {

// Serial.println(i);

  // (no usage in cases possible!)
  hot = digitalRead(TEMP_SENSOR) == 0;
  cold = digitalRead(TEMP_SENSOR) == 1;
  reset = digitalRead(RESET) == 0;

  // Aktivate Backlight of display
  lcd.backlight();

  // Debounce-Function with State-change 0 / 1 (safety)  
  int reading = digitalRead(MAIN_SWITCH);
  if (reading != lastmainSwitchState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != mainSwitchState) {
      mainSwitchState = reading;
      if (mainSwitchState == LOW) {
        mainSwitchInt = !mainSwitchInt;
      }
    }
  }
  lastmainSwitchState = reading;

  // Only for first start
    if (startInt == 1){
      lcd.setCursor(0,0);
      lcd.print(">> Gebootet");
      lcd.setCursor(0,1);
      lcd.print("Hallo 206!");
      startInt = 0;
      }


///////////////////////////////////////
////          ERROR STATES         ////
///////////////////////////////////////
// Diesel empty or comustion Chamber cold (during case == HEATING)
    if (currentState == HEATING && cold) {
      lcd.setCursor(0,1); // 
      lcd.print("                ");      
      lcd.setCursor(0,1); 
      lcd.print("Temp / Diesel ?");
     currentState = ERROR;}

// if RESET_BUTTON is presset, go to case == OFF
    if (currentState == ERROR && reset){currentState = OFF; mainSwitchInt = 0;}


///////////////////////////////////////
////       OPERATING STATES        ////
///////////////////////////////////////
    if(mainSwitchInt == 1) {
      if (currentState == OFF       && (cold || hot))  {currentState = PREHEATING;}    
      if (currentState == SHUT_DOWN)  {mainSwitchInt = 0;}      
    }
  
    if(mainSwitchInt == 0) {
      if(currentState == PREHEATING)           {currentState = OFF;}
      if(currentState == HEATING     && hot)   {currentState = SHUT_DOWN;}
      if(currentState == SHUT_DOWN && cold)  {currentState = OFF;}
    }


///////////////////////////////////////
////       CPU-AKTIVITY-Test       ////
///////////////////////////////////////
// diplays the CPU-AKTIVITY
if (currentState != ERROR){
          if(millis() >= (resetTime + betriebsDelay)) {
            resetTime = millis();
              if(digitalRead(AKTIVE_LED) == LOW){digitalWrite(AKTIVE_LED, HIGH);}
              else{digitalWrite(AKTIVE_LED, LOW);}
          }
}
else{
  digitalWrite(AKTIVE_LED, LOW);
}


///////////////////////////////////////
////         STATEMACHINE          ////
///////////////////////////////////////
// State-Change, if Main switch changes between 0 || 1
  if(currentState != lastState){
    lastState = currentState;
    switch(currentState) {


      case OFF:
        digitalWrite(FAN_LED, LOW);   
        digitalWrite(DIESEL_LED, LOW);
        digitalWrite(PREHEATING_LED, LOW);
        digitalWrite(HEATING_LED, LOW);
        digitalWrite(ERROR_LED, LOW);
          lcd.setCursor(0,0); lcd.print("                ");
          lcd.setCursor(0,1); lcd.print("                ");
          lcd.setCursor(0,0); lcd.print("Standby");
      break;

      
      case PREHEATING:
        // Startsequency PREHEATING: Fan is active
        digitalWrite(HEATING_LED, LOW);
        digitalWrite(FAN_LED, HIGH);
          lcd.setCursor(0,0); lcd.print("                ");
          lcd.setCursor(0,1); lcd.print("                ");
          lcd.setCursor(0,0); lcd.print(">> Vorheizen");        
        // Main query PREHEATING:
        // TEMP_SENSOR == 1 (combustion chamber cold =  0 Volt)
        // TEMP_SENSOR == 0 (combustion chamber hot  = 24 Volt)
        if (digitalRead(TEMP_SENSOR) == 1) {
          // Waiting-phase 1
          // Content: after fan is activatet, wait 4 sec, until rated speed is reached
          for(i=0; i<DelayLuefter; i++){
            // abort condition for-loop ist combustion chamber = warm
            if (digitalRead(TEMP_SENSOR) == 0) {i = DelayLuefter;} 
              delay(50);
            // CPU-Activity-Test within for-loop aswell
            if(millis() >= (resetTime + betriebsDelay)) {
            resetTime = millis();
              if(digitalRead(AKTIVE_LED) == LOW){digitalWrite(AKTIVE_LED, HIGH);}
              else{digitalWrite(AKTIVE_LED, LOW);}
            }
          }
          // if Waiting-phase 1 ended, aktivate preheating (MOSFET, Reichelt: STP 60NF06 STM)
          digitalWrite(PREHEATING_LED, HIGH);
          // Waiting-phase 2 "Preheating && magnetic valve closed"
          for(i=0; i<DelayVorgluehenTrocken; i++){
            // Waiting condition: if DELAY_MAIN_SWITCH is aktive, preheatingphases are on hold for manual intervention
            while(digitalRead(DELAY_MAIN_SWITCH) == 0){
              lcd.setCursor(12,1); lcd.print("man.");
              delay(100);
            }
            // abort condition der for-Schleife, wenn Flammenwächter = warm
            if (digitalRead(TEMP_SENSOR) == 0) {i = DelayVorgluehenTrocken;}
            // displays the remaining time until waiting-phase 2 is over
            if (i > (DelayVorgluehenTrocken*0.01) && i < (DelayVorgluehenTrocken*0.1)){lcd.setCursor(0,1); lcd.print("Diesel OFF     9");}
            if (i > (DelayVorgluehenTrocken*0.1)  && i < (DelayVorgluehenTrocken*0.2)){lcd.setCursor(12,1); lcd.print("   8");}
            if (i > (DelayVorgluehenTrocken*0.2)  && i < (DelayVorgluehenTrocken*0.3)){lcd.setCursor(12,1); lcd.print("   7");}
            if (i > (DelayVorgluehenTrocken*0.3)  && i < (DelayVorgluehenTrocken*0.4)){lcd.setCursor(12,1); lcd.print("   6");}
            if (i > (DelayVorgluehenTrocken*0.4)  && i < (DelayVorgluehenTrocken*0.5)){lcd.setCursor(12,1); lcd.print("   5");}
            if (i > (DelayVorgluehenTrocken*0.5)  && i < (DelayVorgluehenTrocken*0.6)){lcd.setCursor(12,1); lcd.print("   3");}
            if (i > (DelayVorgluehenTrocken*0.6)  && i < (DelayVorgluehenTrocken*0.7)){lcd.setCursor(12,1); lcd.print("   2");}
            if (i > (DelayVorgluehenTrocken*0.7)  && i < (DelayVorgluehenTrocken*0.8)){lcd.setCursor(12,1); lcd.print("   1");}
            if (i > (DelayVorgluehenTrocken*0.8)  && i < (DelayVorgluehenTrocken*0.9)){lcd.setCursor(12,1); lcd.print("   0");}
            delay(100);
            // CPU-Activity-Test within for-loop aswell
            if(millis() >= (resetTime + betriebsDelay)) {
            resetTime = millis();
              if(digitalRead(AKTIVE_LED) == LOW){digitalWrite(AKTIVE_LED, HIGH);}
              else{digitalWrite(AKTIVE_LED, LOW);}
            }
          } // Schlussklammer Waiting-phase 2
          // Waiting-phase 3 "Preheating && magnetic valve open"
          digitalWrite(PREHEATING_LED, HIGH);
          for(i=0; i<DelayVorgluehenNass; i++){
            // Waiting condition: if DELAY_MAIN_SWITCH is aktive, preheatingphases are on hold for manual intervention
            while(digitalRead(DELAY_MAIN_SWITCH) == 0){
              lcd.setCursor(12,1); lcd.print("man.");
              delay(500);
            }
            // abort condition der for-Schleife, wenn Flammenwächter = warm
            if (digitalRead(TEMP_SENSOR) == 0) {i = DelayVorgluehenNass;}
            // displays the remaining time until waiting-phase 3 is over
            if (i > (DelayVorgluehenNass*0.01) && i < (DelayVorgluehenNass*0.1)){lcd.setCursor(0,1); lcd.print("Diesel ON    10%");}
            if (i > (DelayVorgluehenNass*0.1)  && i < (DelayVorgluehenNass*0.2)){lcd.setCursor(12,1); lcd.print(" 20%");}
            if (i > (DelayVorgluehenNass*0.2)  && i < (DelayVorgluehenNass*0.3)){lcd.setCursor(12,1); lcd.print(" 30%");}
            if (i > (DelayVorgluehenNass*0.3)  && i < (DelayVorgluehenNass*0.4)){lcd.setCursor(12,1); lcd.print(" 40%");}
            if (i > (DelayVorgluehenNass*0.4)  && i < (DelayVorgluehenNass*0.5)){lcd.setCursor(12,1); lcd.print(" 50%");}
            if (i > (DelayVorgluehenNass*0.5)  && i < (DelayVorgluehenNass*0.6)){lcd.setCursor(12,1); lcd.print(" 60%");}
            if (i > (DelayVorgluehenNass*0.6)  && i < (DelayVorgluehenNass*0.7)){lcd.setCursor(12,1); lcd.print(" 70%");}
            if (i > (DelayVorgluehenNass*0.7)  && i < (DelayVorgluehenNass*0.8)){lcd.setCursor(12,1); lcd.print(" 80%");}
            if (i > (DelayVorgluehenNass*0.8)  && i < (DelayVorgluehenNass*0.9)){lcd.setCursor(12,1); lcd.print(" 90%");}
            delay(100);
            // CPU-Activity-Test within for-loop aswell
            if(millis() >= (resetTime + betriebsDelay)) {
            resetTime = millis();
              if(digitalRead(AKTIVE_LED) == LOW){digitalWrite(AKTIVE_LED, HIGH);}
              else{digitalWrite(AKTIVE_LED, LOW);}
            }
          } // closing parenthesis Waiting-phase 3
        } // closing parenthesis Mainframe Waiting-phases / Preheating-phases
        // if digitalRead(TEMP_SENSOR) == 0 (combustion chamber = hot), case == HEIZEN
        // else case == ERROR
        if (digitalRead(TEMP_SENSOR) == 0) {currentState = HEATING;}
        else{
          currentState = ERROR; 
          lcd.setCursor(0,1); lcd.print("                ");
          lcd.setCursor(0,1); lcd.print("Temp zu niedrig");      
        }
       break;


      // Difference to PREHEATING: PREHEATING_LED is deaktivatet
      // FAN and magnetic valve active
      case HEATING:
        digitalWrite(FAN_LED, HIGH);
        digitalWrite(DIESEL_LED, HIGH);
        digitalWrite(PREHEATING_LED, LOW);
        digitalWrite(HEATING_LED, HIGH);
          lcd.setCursor(0,0); lcd.print("                ");
          lcd.setCursor(0,1); lcd.print("                ");
          lcd.setCursor(0,0); lcd.print(">> Heizbetrieb");
          lcd.setCursor(0,1); lcd.print("Diesel ON");
        break;


      // DIFFERENCE TO HEATING: magnetic valve is deactiveatet
      // waits until combustion chamber = cold
      // if condition for MainSwitchState == 0 && cold, you can find above (OPERATING STATES)
      case SHUT_DOWN:
        digitalWrite(DIESEL_LED, LOW);
        if(cold){currentState = OFF;}
          lcd.setCursor(0,0); lcd.print("                ");
          lcd.setCursor(0,1); lcd.print("                ");
          lcd.setCursor(0,0); lcd.print(">> Abschalten");
          lcd.setCursor(0,1); lcd.print("warte...");
        break;

        
      // DIFFERENCE to OFF: ERROR-LED is blinking
      // Abort and go to case OFF is only possible via pressing RESET_BUTTON
      case ERROR:
        digitalWrite(FAN_LED, LOW);
        digitalWrite(DIESEL_LED, LOW);
        digitalWrite(PREHEATING_LED, LOW);
        digitalWrite(HEATING_LED, LOW);
          lcd.setCursor(0,0); lcd.print("                ");
          lcd.setCursor(0,0); lcd.print(">> FEHLER !!");
        while(digitalRead(RESET) == 1){
          if(millis() >= (resetTime + resetDelay)) {
            resetTime = millis();
              if(digitalRead(ERROR_LED) == LOW){digitalWrite(ERROR_LED, HIGH);}
              else{digitalWrite(ERROR_LED, LOW);}
          }
        }
       break; 
    } // closing parenthesis switch currentState
  } // closing parenthesis State-changes due to if-condition above (OPERATING STATES)


} //Schlussklammer
