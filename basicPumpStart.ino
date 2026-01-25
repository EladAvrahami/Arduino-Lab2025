const int pumpPin = 30;

void setup() {
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW); // ריליי כבוי
}

void loop() {
  digitalWrite(pumpPin, HIGH); // מפעיל משאבה
  delay(3000);                 // משאבה עובדת 3 שניות
  digitalWrite(pumpPin, LOW);  // מכבה משאבה
  delay(5000);
}