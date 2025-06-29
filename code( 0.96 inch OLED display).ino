#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <EEPROM.h>

// WiFi credentials
const char* ssid = "Me";
const char* password = "mehedi113";

// ThingSpeak settings
const char* server = "api.thingspeak.com";
String writeAPIKey = "PQW4LV08LZVQANI4";
unsigned long channelID = 2971828;

// OLED setup (0.96" 128x64 I2C display)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// EEPROM addresses for vote storage
#define EEPROM_SIZE 512
#define ADDR_PRES_VOTE1 0
#define ADDR_PRES_VOTE2 4
#define ADDR_SEC_VOTE1 8
#define ADDR_SEC_VOTE2 12
#define ADDR_VP_VOTE1 16
#define ADDR_VP_VOTE2 20
#define ADDR_COUPON_START 24
#define ADDR_MAGIC_NUMBER 500  // To check if EEPROM has been initialized

// Vote counters
int presidentialVotes[2] = {0, 0};
int secretaryVotes[2] = {0, 0};
int vpVotes[2] = {0, 0};

// Coupon system
#define MAX_COUPONS 100
String couponCodes[MAX_COUPONS] = {
  "599107", "599108", "599109", "599110", "599111", "599112", "599113", "599114", "599115", "599116", 
  "599117", "599118", "599119", "599120", "599121", "599122", "599123", "599124", "599125", "599126", 
  "599127", "599128", "599129", "599130", "599131", "599132", "599133", "599134", "599135", "599136", 
  "599137", "599138", "599139", "599140", "599141", "599142", "599143", "599144", "599145", "599146", 
  "599147", "599148", "599149", "599150", "599151", "599152", "599153", "599154", "599155", "599156", 
  "599157", "599158", "599159", "599160", "599161", "599162", "599163", "599164", "599165", "599166", 
  "599167", "599168", "599169", "599170", "599171", "599172", "599173", "599174", "599175", "599176", 
  "599177", "599178", "599179", "599180", "599181", "599182", "599183", "599184", "599185", "599186", 
  "599187", "599188", "599189", "599190", "599191", "599192", "599193", "599194", "599195", "599196", 
  "599197", "599198", "599199", "599200", "599201", "599202", "599203", "599204", "599205", "599206"
};
bool couponUsed[MAX_COUPONS] = {false};

// Voting state
enum SystemState {
  WELCOME, COUPON_ENTRY, PRESIDENTIAL_VOTING, SECRETARY_VOTING,
  VP_VOTING, CONFIRM_VOTE, VOTE_CAST, ADMIN_MODE
};
SystemState currentState = WELCOME;

int selectedCandidate[3] = {-1, -1, -1};
unsigned long lastKeyPress = 0;
const unsigned long TIMEOUT = 30000;
String couponBuffer = "";
bool validCouponEntered = false;

// EEPROM Functions
void saveVotesToEEPROM() {
  EEPROM.put(ADDR_PRES_VOTE1, presidentialVotes[0]);
  EEPROM.put(ADDR_PRES_VOTE2, presidentialVotes[1]);
  EEPROM.put(ADDR_SEC_VOTE1, secretaryVotes[0]);
  EEPROM.put(ADDR_SEC_VOTE2, secretaryVotes[1]);
  EEPROM.put(ADDR_VP_VOTE1, vpVotes[0]);
  EEPROM.put(ADDR_VP_VOTE2, vpVotes[1]);
  EEPROM.commit();
}

void loadVotesFromEEPROM() {
  // Check if EEPROM has been initialized
  int magicNumber;
  EEPROM.get(ADDR_MAGIC_NUMBER, magicNumber);
  
  if (magicNumber != 12345) {
    // First time initialization
    presidentialVotes[0] = presidentialVotes[1] = 0;
    secretaryVotes[0] = secretaryVotes[1] = 0;
    vpVotes[0] = vpVotes[1] = 0;
    
    // Initialize coupon usage
    for (int i = 0; i < MAX_COUPONS; i++) {
      couponUsed[i] = false;
    }
    
    // Set magic number and save initial state
    EEPROM.put(ADDR_MAGIC_NUMBER, 12345);
    saveVotesToEEPROM();
    saveCouponStatusToEEPROM();
  } else {
    // Load existing data
    EEPROM.get(ADDR_PRES_VOTE1, presidentialVotes[0]);
    EEPROM.get(ADDR_PRES_VOTE2, presidentialVotes[1]);
    EEPROM.get(ADDR_SEC_VOTE1, secretaryVotes[0]);
    EEPROM.get(ADDR_SEC_VOTE2, secretaryVotes[1]);
    EEPROM.get(ADDR_VP_VOTE1, vpVotes[0]);
    EEPROM.get(ADDR_VP_VOTE2, vpVotes[1]);
    loadCouponStatusFromEEPROM();
  }
}

void saveCouponStatusToEEPROM() {
  for (int i = 0; i < MAX_COUPONS; i++) {
    EEPROM.put(ADDR_COUPON_START + i, couponUsed[i]);
  }
  EEPROM.commit();
}

void loadCouponStatusFromEEPROM() {
  for (int i = 0; i < MAX_COUPONS; i++) {
    EEPROM.get(ADDR_COUPON_START + i, couponUsed[i]);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("EVM System Starting");
  display.setCursor(0, 10);
  display.println("Loading data...");
  display.display();

  // Load previous vote data
  loadVotesFromEEPROM();
  
  display.setCursor(0, 20);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.setCursor(0, 10);
  display.println("Data Loaded!");
  display.display();
  delay(2000);
  showWelcomeScreen();
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    lastKeyPress = millis();
    handleKeyPress(key);
  }
  if (millis() - lastKeyPress > TIMEOUT && currentState != WELCOME) {
    resetVoting();
  }
}

void handleKeyPress(char key) {
  switch (currentState) {
    case WELCOME:
      if (key == '1') {
        currentState = COUPON_ENTRY;
        couponBuffer = "";
        showCouponPrompt();
      } else if (key == '*') {
        enterAdminMode();
      }
      break;

    case COUPON_ENTRY:
      if (key == '#') {
        if (couponBuffer.length() == 0) {
          // Empty coupon code - show error
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("ERROR: Empty Code");
          display.setCursor(0, 10);
          display.println("Please enter code");
          display.display();
          delay(2000);
          showCouponPrompt();
        } else if (validateCoupon(couponBuffer)) {
          validCouponEntered = true;
          startVoting();
        } else {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Invalid/Used Code");
          display.display();
          delay(2000);
          resetVoting();
        }
      } else if (key == '*') {
        couponBuffer = "";
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Enter Coupon Code");
        display.setCursor(0, 10);
        display.println("Then press #");
        display.display();
      } else {
        if (couponBuffer.length() < 10) {
          couponBuffer += key;
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Enter Coupon Code");
          display.setCursor(0, 10);
          display.println("Then press #");
          display.setCursor(0, 20);
          display.println("Code: " + couponBuffer);
          display.display();
        }
      }
      break;

    case PRESIDENTIAL_VOTING:
      if (key == '1') selectedCandidate[0] = 0;
      else if (key == '2') selectedCandidate[0] = 1;
      if (selectedCandidate[0] != -1) {
        currentState = SECRETARY_VOTING;
        showSecretaryVoting();
      }
      break;

    case SECRETARY_VOTING:
      if (key == '1') selectedCandidate[1] = 0;
      else if (key == '2') selectedCandidate[1] = 1;
      if (selectedCandidate[1] != -1) {
        currentState = VP_VOTING;
        showVPVoting();
      }
      break;

    case VP_VOTING:
      if (key == '1') selectedCandidate[2] = 0;
      else if (key == '2') selectedCandidate[2] = 1;
      if (selectedCandidate[2] != -1) {
        currentState = CONFIRM_VOTE;
        showConfirmation();
      }
      break;

    case CONFIRM_VOTE:
      if (key == '1') {
        castVote();
      } else if (key == '2') {
        resetVoting();
      }
      break;

    case VOTE_CAST:
      if (key == '#') resetVoting();
      break;

    case ADMIN_MODE:
      if (key == '1') showResults();
      else if (key == '2') syncWithCloud();
      else if (key == '3') resetAllData();
      else if (key == '#') resetVoting();
      break;
  }
}

void showWelcomeScreen() {
  currentState = WELCOME;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESRC VOTING");
  display.println("MACHINE");
  display.println("");
  display.println("Press 1 to Vote");
  display.println("Press * for Admin");
  display.display();
}

void showCouponPrompt() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Enter Coupon Code");
  display.setCursor(0, 10);
  display.println("Then press #");
  display.display();
}

bool validateCoupon(String code) {
  // Check for empty code first
  if (code.length() == 0) {
    return false;
  }
  
  for (int i = 0; i < MAX_COUPONS; i++) {
    if (couponCodes[i] == code && !couponUsed[i]) {
      couponUsed[i] = true;
      saveCouponStatusToEEPROM(); // Save immediately
      return true;
    }
  }
  return false;
}

void startVoting() {
  currentState = PRESIDENTIAL_VOTING;
  selectedCandidate[0] = selectedCandidate[1] = selectedCandidate[2] = -1;
  showPresidentialVoting();
}

void showPresidentialVoting() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("PRESIDENT");
  display.println("");
  display.println("1. Sami");
  display.println("2. Trisha");
  display.println("");
  display.println("Select candidate:");
  display.display();
}

void showSecretaryVoting() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("GENERAL SECRETARY");
  display.println("");
  display.println("1. Sadat");
  display.println("2. Nipa");
  display.println("");
  display.println("Select candidate:");
  display.display();
}

void showVPVoting() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("VICE PRESIDENT");
  display.println("");
  display.println("1. Nisa");
  display.println("2. Shuvojit");
  display.println("");
  display.println("Select candidate:");
  display.display();
}

void showConfirmation() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("CONFIRM YOUR VOTE");
  display.println("");
  display.print("P: ");
  display.println(selectedCandidate[0] == 0 ? "Sami" : "Trisha");
  display.print("S: ");
  display.println(selectedCandidate[1] == 0 ? "Sadat" : "Nipa");
  display.print("VP: ");
  display.println(selectedCandidate[2] == 0 ? "Nisa" : "Shuvojit");
  display.display();
  delay(3000);
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.println("1=YES  2=CANCEL");
  display.display();
}

void castVote() {
  presidentialVotes[selectedCandidate[0]]++;
  secretaryVotes[selectedCandidate[1]]++;
  vpVotes[selectedCandidate[2]]++;
  
  // Save to EEPROM immediately
  saveVotesToEEPROM();
  
  sendVoteToCloud();
  currentState = VOTE_CAST;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("VOTE SUCCESSFULLY");
  display.println("RECORDED!");
  display.println("");
  display.println("Thank you");
  display.println("");
  display.println("Press # to exit");
  display.display();
}

void sendVoteToCloud() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.thingspeak.com/update?api_key=" + writeAPIKey;
    url += "&field1=" + String(presidentialVotes[0]);
    url += "&field2=" + String(presidentialVotes[1]);
    url += "&field3=" + String(secretaryVotes[0]);
    url += "&field4=" + String(secretaryVotes[1]);
    url += "&field5=" + String(vpVotes[0]);
    url += "&field6=" + String(vpVotes[1]);
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Vote data sent to ThingSpeak successfully.");
    } else {
      Serial.println("Error sending vote: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void enterAdminMode() {
  currentState = ADMIN_MODE;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("ADMIN MODE");
  display.println("");
  display.println("1. Show Results");
  display.println("2. Sync Cloud");
  display.println("3. Reset");
  display.println("# Exit");
  display.display();
}

void showResults() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("RESULTS:");
  display.println("");
  display.print("Sami:");
  display.print(presidentialVotes[0]);
  display.print(" Trisha:");
  display.println(presidentialVotes[1]);
  display.print("Sadat:");
  display.print(secretaryVotes[0]);
  display.print(" Nipa:");
  display.println(secretaryVotes[1]);
  display.print("Nisa:");
  display.print(vpVotes[0]);
  display.print(" Shuvo:");
  display.println(vpVotes[1]);
  display.display();
}

void syncWithCloud() {
  sendVoteToCloud();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Syncing with Cloud");
  display.println("Please wait...");
  display.display();
  delay(2000);
  enterAdminMode();
}

void resetAllData() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Resetting all data");
  display.println("Please wait...");
  display.display();
  
  // Reset all vote counts
  presidentialVotes[0] = presidentialVotes[1] = 0;
  secretaryVotes[0] = secretaryVotes[1] = 0;
  vpVotes[0] = vpVotes[1] = 0;
  
  // Reset all coupons
  for (int i = 0; i < MAX_COUPONS; i++) {
    couponUsed[i] = false;
  }
  
  // Save to EEPROM
  saveVotesToEEPROM();
  saveCouponStatusToEEPROM();
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Reset complete!");
  display.display();
  delay(2000);
  enterAdminMode();
}

void resetVoting() {
  currentState = WELCOME;
  selectedCandidate[0] = selectedCandidate[1] = selectedCandidate[2] = -1;
  couponBuffer = "";
  validCouponEntered = false;
  showWelcomeScreen();
}
