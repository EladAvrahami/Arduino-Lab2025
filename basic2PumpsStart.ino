const int pumpPinR = 48;
const int pumpPinL = 49;
void setup(){
  pinMode(pumpPinR, OUTPUT);
  digitalWrite(pumpPinR, HIGH); // ריליי כבוי
  pinMode(pumpPinL, OUTPUT);
  digitalWrite(pumpPinL, HIGH); // ריליי כבוי
}

void loop(){
  digitalWrite(pumpPinR, LOW); // מפעיל משאבה
  delay(2000);                 // משאבה עובדת 3 שניות
  digitalWrite(pumpPinR, HIGH);  // מכבה משאבה
  delay(5000);
  digitalWrite(pumpPinL, LOW); // מפעיל משאבה
  delay(2000);                 // משאבה עובדת 3 שניות
  digitalWrite(pumpPinL, HIGH);  // מכבה משאבה
  delay(5000);
}