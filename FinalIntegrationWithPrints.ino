/*
 * פרויקט: בר מסוע אוטומטי - גרסה משולבת עם תצוגת LCD
 * לוגיקה: כל השלבים כולל הגנת Timeout, אימות חיישנים וסינון רעשי כפתורים
 * בקר: Arduino Due
 */

// --------- הגדרות פינים לכפתורים  ---------
const int BTN_WHITE = 52;     // פין לכפתור וודקה (לבן)
const int BTN_BLUE  = 51;     // פין לכפתור מיקס קולה וודקה (כחול)
const int BTN_RED   = 53;     // פין לכפתור קולה (אדום)

// ---------- הגדרות פינים למנוע DC -------------
const int MOTOR_IN1 = 5;      // פין כיוון 1 למנוע
const int MOTOR_IN2 = 4;      // פין כיוון 2 למנוע
const int MOTOR_ENABLE = 6;   // פין PWM לשליטה על מהירות המנוע

// ----------- הגדרות פינים לחיישני IR דיגיטליים ----------------
const int IR_START_END = 23;  // חיישן 1: ממוקם בנקודת ההתחלה והסיום
const int IR_PUMP_POINT = 24; // חיישן 2: ממוקם בדיוק מתחת לצינורות המזיגה

// ------------- הגדרות פינים למשאבות (Relays) -------------------
const int PUMP_COLA = 48;     // פין פיקוד למשאבת קולה
const int PUMP_VODKA = 49;    // פין פיקוד למשאבת וודקה

// ----------------- משתני זמן ובטיחות ------------------------------
unsigned long stateStartTime;             // משתנה לשמירת זמן תחילת פעולה (למדידת Timeout)
const unsigned long MOVE_TIMEOUT = 15000;  // הגדרת זמן מקסימלי של 15 שניות לתנועת המסוע

// --- משתני פסיקה (Interrupts) וניהול מצב --------------------------
volatile bool whitePressed = false;       // דגל פסיקה לכפתור לבן
volatile bool bluePressed  = false;       // דגל פסיקה לכפתור כחול
volatile bool redPressed   = false;       // דגל פסיקה לכפתור אדום
bool systemBusy = false;                  // משתנה חסימה למניעת לחיצות בזמן שהמערכת עובדת

// --- הגדרת מצבי המערכת (State Machine) ----------------------------
enum State { WAITING_FOR_CUP, CHOOSING_DRINK, MOVING_TO_PUMP, POURING, MOVING_TO_END, FINISHED, ERROR_STATE };
State currentState = WAITING_FOR_CUP;     // התחלת המערכת במצב המתנה לכוס
int selectedDrink = 0;                    // משתנה לשמירת סוג המשקה שנבחר (1=קולה, 2=וודקה, 3=מיקס)

// --- פונקציות פסיקה לכפתורים (ISRs) ---
void whiteISR() { if (!systemBusy) whitePressed = true; } 
void blueISR()  { if (!systemBusy) bluePressed  = true; } 
void redISR()   { if (!systemBusy) redPressed   = true; } 

void setup() {
  Serial.begin(9600);           
  Serial1.begin(9600);          // אתחול תקשורת למסך ה-LCD
  delay(500);                   

  pinMode(MOTOR_IN1, OUTPUT);   
  pinMode(MOTOR_IN2, OUTPUT);   
  pinMode(MOTOR_ENABLE, OUTPUT);
  stopDC();                     

  pinMode(PUMP_COLA, OUTPUT);   
  pinMode(PUMP_VODKA, OUTPUT);  
  digitalWrite(PUMP_COLA, HIGH); // כיבוי (Active Low)
  digitalWrite(PUMP_VODKA, HIGH);

  pinMode(IR_START_END, INPUT_PULLUP);  
  pinMode(IR_PUMP_POINT, INPUT_PULLUP); 

  pinMode(BTN_WHITE, INPUT_PULLUP);     
  pinMode(BTN_BLUE,  INPUT_PULLUP);     
  pinMode(BTN_RED,   INPUT_PULLUP);     
  
  attachInterrupt(digitalPinToInterrupt(BTN_WHITE), whiteISR, FALLING); 
  attachInterrupt(digitalPinToInterrupt(BTN_BLUE),  blueISR,  FALLING); 
  attachInterrupt(digitalPinToInterrupt(BTN_RED),   redISR,   FALLING); 

  showWelcome(); // שלב 1: הצגת הודעת ברוך הבא
}

void loop() {
  switch (currentState) {
    
    case WAITING_FOR_CUP:       
      if (checkSensorStable(IR_START_END, 12, 250)) { 
        showMenu(); // שלב 3: הצגת תפריט הבחירה מיד עם זיהוי הכוס
        currentState = CHOOSING_DRINK;                
      }
      break;

    case CHOOSING_DRINK:        
      systemBusy = false;       
      if (whitePressed || bluePressed || redPressed) { 
        delay(50); // Debounce
        
        if (redPressed && digitalRead(BTN_RED) == LOW) {
           selectedDrink = 1; 
        } else if (whitePressed && digitalRead(BTN_WHITE) == LOW) {
           selectedDrink = 2; 
        } else if (bluePressed && digitalRead(BTN_BLUE) == LOW) {
           selectedDrink = 3; 
        } else {
           whitePressed = bluePressed = redPressed = false;
           break; 
        }

        systemBusy = true;      
        whitePressed = bluePressed = redPressed = false; 
        
        drinkSelectedMsg();     // שלב 4: הודעת אישור בחירה
        
        showMoving();           // הדפסת הודעת "בתנועה" למסך
        moveDCLeft();           
        stateStartTime = millis(); 
        currentState = MOVING_TO_PUMP; 
      }
      break;

    case MOVING_TO_PUMP:        
      if (checkSensorStable(IR_PUMP_POINT, 3, 20)) { 
        stopDC();               
        delay(1000);
        currentState = POURING; 
      } else if (millis() - stateStartTime > MOVE_TIMEOUT) { 
        handleError("PUMP NOT FOUND"); 
      }
      break;

    case POURING:               
      showPouringMsg();         // הצגת הודעת "מוזג..." במסך
      
      if (selectedDrink == 1)      pourCola();  
      else if (selectedDrink == 2) pourVodka(); 
      else if (selectedDrink == 3) pourMix();   
      
      clearLCD();
      Serial1.print("   Finished!"); // הודעת סיום מזיגה
      delay(5000);              
      
      showMoving();             // הודעת "בתנועה" חזרה
      moveDCRight();            
      stateStartTime = millis(); 
      currentState = MOVING_TO_END; 
      break;

    case MOVING_TO_END:         
      if (checkSensorStable(IR_START_END, 5, 20)) { 
        stopDC();               
        currentState = FINISHED;
      } else if (millis() - stateStartTime > MOVE_TIMEOUT) { 
        handleError("HOME NOT FOUND"); 
      }
      break;

    case FINISHED:              
      showDoneMsg();            // שלב 12: הודעת סיום חגיגית
      delay(7000);              
      showWelcome();            
      currentState = WAITING_FOR_CUP; 
      break;

    case ERROR_STATE:           
      stopDC();                 
      break;
  }
}

// --- פונקציות תפעוליות למשאבות ---

void pourCola() {
  digitalWrite(PUMP_COLA, LOW); 
  delay(8000);                  
  digitalWrite(PUMP_COLA, HIGH);
}

void pourVodka() {
  digitalWrite(PUMP_VODKA, LOW); 
  delay(8000);                   
  digitalWrite(PUMP_VODKA, HIGH);
}

void pourMix() {
  digitalWrite(PUMP_COLA, LOW);  
  digitalWrite(PUMP_VODKA, LOW); 
  delay(4000);                   
  digitalWrite(PUMP_COLA, HIGH); 
  digitalWrite(PUMP_VODKA, HIGH);
}

// --- פונקציות שליטה במנוע DC ---

void moveDCLeft() {
  digitalWrite(MOTOR_IN1, HIGH); 
  digitalWrite(MOTOR_IN2, LOW);  
  analogWrite(MOTOR_ENABLE, 100);
}

void moveDCRight() {
  digitalWrite(MOTOR_IN1, LOW);  
  digitalWrite(MOTOR_IN2, HIGH); 
  analogWrite(MOTOR_ENABLE, 120);
}

void stopDC() {
  digitalWrite(MOTOR_IN1, LOW);  
  digitalWrite(MOTOR_IN2, LOW);  
  analogWrite(MOTOR_ENABLE, 0);  
}

// --- פונקציות עזר לבדיקת חיישנים ---

bool checkSensorStable(int pin, int count, int interval) {
  for (int i = 0; i < count; i++) {
    if (digitalRead(pin) == HIGH) return false; 
    delay(interval);                            
  }
  return true; 
}

// ==========================================
// פונקציות ההדפסה והניהול של מסך ה-LCD
// ==========================================

void clearLCD() {
  Serial1.write(254); 
  Serial1.write(1);   
  delay(50);          
}

void goToLine2() {
  Serial1.write(254); 
  Serial1.write(192); 
}

void showWelcome() { 
  clearLCD();
  Serial1.print("    Welcome! "); 
  goToLine2();
  Serial1.print(" Place your cup "); 
}

void showMenu() {
  clearLCD();
  Serial1.print("Select a drink:"); // קיצור כדי שייכנס בשורה
  goToLine2();
  Serial1.print("Vodka Cola Mix");   // רמז לפי צבעי הכפתורים
}

void drinkSelectedMsg() {
  clearLCD();
  Serial1.print("Drink selected!"); 
  goToLine2();
  Serial1.print("Preparing now...");
  delay(2000); 
}

void showMoving() {
  clearLCD();
  Serial1.print("Conveyor Moving");
  goToLine2();
  Serial1.print("Please wait...");
}

void showPouringMsg() {
  clearLCD();
  Serial1.print("  Pouring now ");
  goToLine2();
  Serial1.print("   Enjoy!!!   ");
}

void showDoneMsg() {
  clearLCD();
  Serial1.print("  Na Zdarovia!  ");
  goToLine2();
  Serial1.print(" Take your drink ");
}

void handleError(const char* errorMsg) {
  stopDC(); 
  digitalWrite(PUMP_COLA, HIGH);
  digitalWrite(PUMP_VODKA, HIGH);
  
  clearLCD();
  Serial1.print(" SYSTEM ERROR! "); 
  goToLine2();
  Serial1.print("Check Terminal"); // הודעה כללית למסך
  
  Serial.print("CRITICAL ERROR: "); 
  Serial.println(errorMsg);     // פירוט השגיאה למחשב
  currentState = ERROR_STATE; 
}