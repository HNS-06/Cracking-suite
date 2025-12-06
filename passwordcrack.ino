/*
   ARDUINO PASSWORD CRACKER SIMULATOR — PACK A
   Optimized for Arduino UNO (low RAM)
   Supports:
   - Brute Force (numeric)
   - Dictionary Attack (dashboard sends dictionary)
   - Mask Attack (?d = digit, ?l = lowercase, ?u = uppercase)
   - JSON status output for dashboard
*/

String targetPassword = "";
bool cracking = false;
bool stopFlag = false;

unsigned long attempts = 0;
unsigned long startTime = 0;

// Dictionary storage (UNO limited)
#define MAX_DICT 40
String dict[MAX_DICT];
int dictCount = 0;

String mask = "";
bool maskMode = false;

// --------------------- UTILITY -----------------------
void sendJSON(const char *status, const char *mode) {
  Serial.print("{\"status\":\"");
  Serial.print(status);
  Serial.print("\",\"mode\":\"");
  Serial.print(mode);
  Serial.print("\",\"attempts\":");
  Serial.print((unsigned long)attempts);
  Serial.print(",\"target\":\"");
  Serial.print(targetPassword);
  Serial.println("\"}");
}

// Pads numbers (e.g., 7 → 0007)
String padNum(unsigned long n, int len) {
  String s = String(n);
  while (s.length() < len) s = "0" + s;
  return s;
}

// --------------------- BRUTE FORCE ---------------------
void runBruteForce() {
  cracking = true;
  stopFlag = false;
  attempts = 0;
  startTime = millis();

  int len = targetPassword.length();
  unsigned long maxRange = 1;
  for (int i = 0; i < len; i++) maxRange *= 10;

  for (unsigned long n = 0; n < maxRange; n++) {
    if (stopFlag) break;

    attempts++;
    String cand = padNum(n, len);

    if (cand == targetPassword) {
      sendJSON("FOUND", "BRUTEFORCE");
      cracking = false;
      return;
    }

    if (attempts % 200 == 0) sendJSON("RUNNING", "BRUTEFORCE");
  }

  sendJSON("NOT_FOUND", "BRUTEFORCE");
  cracking = false;
}

// --------------------- DICTIONARY ---------------------
void runDictionary() {
  cracking = true;
  stopFlag = false;
  attempts = 0;
  startTime = millis();

  for (int i = 0; i < dictCount; i++) {
    if (stopFlag) break;

    attempts++;

    if (dict[i] == targetPassword) {
      sendJSON("FOUND", "DICTIONARY");
      cracking = false;
      return;
    }

    if (attempts % 5 == 0) sendJSON("RUNNING", "DICTIONARY");
  }

  sendJSON("NOT_FOUND", "DICTIONARY");
  cracking = false;
}

// --------------------- MASK ATTACK ---------------------
char nextChar(char type, char current) {
  if (type == 'd') { // digits
    if (current < '0' || current > '9') return '0';
    if (current == '9') return 0;
    return current + 1;
  }
  if (type == 'l') { // lowercase
    if (current < 'a' || current > 'z') return 'a';
    if (current == 'z') return 0;
    return current + 1;
  }
  if (type == 'u') { // uppercase
    if (current < 'A' || current > 'Z') return 'A';
    if (current == 'Z') return 0;
    return current + 1;
  }
  return 0;
}

void runMask() {
  cracking = true;
  stopFlag = false;
  attempts = 0;

  int L = mask.length();
  char cand[10];
  char type[10];

  for (int i = 0; i < L; i++) {
    if (mask[i] == '?') {
      char t = mask[i + 1];
      type[i] = t;
      if (t == 'd') cand[i] = '0';
      else if (t == 'l') cand[i] = 'a';
      else if (t == 'u') cand[i] = 'A';
      i++;
    }
  }

  cand[L] = '\0';

  while (!stopFlag) {
    attempts++;

    if (String(cand) == targetPassword) {
      sendJSON("FOUND", "MASK");
      cracking = false;
      return;
    }

    if (attempts % 100 == 0) sendJSON("RUNNING", "MASK");

    // increment
    for (int p = L - 1; p >= 0; p--) {
      if (mask[p] == '?') {
        char t = type[p];
        char next = nextChar(t, cand[p]);
        if (next == 0) {
          if (t == 'd') cand[p] = '0';
          if (t == 'l') cand[p] = 'a';
          if (t == 'u') cand[p] = 'A';
        } else {
          cand[p] = next;
          break;
        }
      }
      if (p == 0) { stopFlag = true; break; }
    }
  }

  sendJSON("NOT_FOUND", "MASK");
  cracking = false;
}

// --------------------- COMMAND HANDLER ---------------------
void processCmd(String cmd) {
  cmd.trim();

  // SET TARGET
  if (cmd.startsWith("SET TARGET")) {
    targetPassword = cmd.substring(11);
    targetPassword.trim();
    sendJSON("TARGET_SET", "NONE");
    return;
  }

  // SET DICT
  if (cmd.startsWith("SET DICT")) {
    String list = cmd.substring(9);
    dictCount = 0;

    int start = 0;
    while (true) {
      int comma = list.indexOf(',', start);
      if (comma == -1) {
        dict[dictCount++] = list.substring(start);
        break;
      }
      dict[dictCount++] = list.substring(start, comma);
      start = comma + 1;
      if (dictCount >= MAX_DICT) break;
    }
    sendJSON("DICT_SET", "NONE");
    return;
  }

  // START BF
  if (cmd == "START BF") {
    runBruteForce();
    return;
  }

  // START DICT
  if (cmd == "START DICT") {
    runDictionary();
    return;
  }

  // START MASK
  if (cmd.startsWith("START MASK")) {
    mask = cmd.substring(10);
    mask.trim();
    runMask();
    return;
  }

  // STOP
  if (cmd == "STOP") {
    stopFlag = true;
    cracking = false;
    sendJSON("STOPPED", "NONE");
    return;
  }

  // STATUS
  if (cmd == "STATUS") {
    sendJSON("STATUS", "NONE");
    return;
  }
}

// --------------------- MAIN LOOP ---------------------
String buffer = "";

void setup() {
  Serial.begin(9600);
  sendJSON("READY", "NONE");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processCmd(buffer);
      buffer = "";
    } else {
      buffer += c;
    }
  }
}
