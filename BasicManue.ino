// פינים
const int BTN_WHITE = 2;   // וודקה
const int BTN_BLUE  = 3;   // קולה + וודקה
const int BTN_RED   = 4;   // קולה

volatile bool whitePressed = false;
volatile bool bluePressed  = false;
volatile bool redPressed   = false;

bool systemBusy = false;   // נעילת אירוע

// ISR
void whiteISR() { if (!systemBusy) whitePressed = true; }
void blueISR()  { if (!systemBusy) bluePressed  = true; }
void redISR()   { if (!systemBusy) redPressed   = true; }

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(300);

  // *** תיקון כפילויות במסך ***
  Serial1.write(254);

  pinMode(BTN_WHITE, INPUT_PULLUP);
  pinMode(BTN_BLUE,  INPUT_PULLUP);
  pinMode(BTN_RED,   INPUT_PULLUP);

  attachInterrupt(BTN_WHITE, whiteISR, FALLING);
  attachInterrupt(BTN_BLUE,  blueISR,  FALLING);
  attachInterrupt(BTN_RED,   redISR,   FALLING);

  showDefault();
}

void loop() {

  if (systemBusy) return;  // אם עסוק — לא מקבל לחיצות

  // כפתור לבן – Vodka
  if (whitePressed) {
    handlePress("Vodka makes life easyer");
    whitePressed = false;
  }

  // כפתור כחול – Cola & Vodka
  if (bluePressed) {
    handlePress("Cola & Vodka");
    bluePressed = false;
  }

  // כפתור אדום – Cola
  if (redPressed) {
    handlePress("Pour Cola");
    redPressed = false;
  }
}

// ---------------------------------------------------------
// לוגיקה מרכזית — טיפול בלחיצה אחת בלבד
// ---------------------------------------------------------

void handlePress(const char* drinkMessage) {
  systemBusy = true;   // נועל את המערכת

  detachInterrupt(BTN_WHITE);
  detachInterrupt(BTN_BLUE);
  detachInterrupt(BTN_RED);

  // הודעת המשקה
  clearLCD();
  Serial1.print(drinkMessage);
  Serial.println(drinkMessage);
  delay(5000);

  // NA ZDAROVIA
  clearLCD();
  Serial1.print("NA ZDAROVIA");
  Serial.println("NA ZDAROVIA");
  delay(5000);

  // חזרה לברירת מחדל
  showDefault();

  // איפוס ונעילה מחדש
  attachInterrupt(BTN_WHITE, whiteISR, FALLING);
  attachInterrupt(BTN_BLUE,  blueISR,  FALLING);
  attachInterrupt(BTN_RED,   redISR,   FALLING);

  systemBusy = false;
}

// ---------------------------------------------------------
// פונקציות עזר למסך
// ---------------------------------------------------------

void clearLCD() {
  Serial1.write(254);
  Serial1.write(1);
  delay(20);
}

void showDefault() {
  clearLCD();
  Serial1.print(":Choose your drink");
  Serial.println(":Choose your drink");
}