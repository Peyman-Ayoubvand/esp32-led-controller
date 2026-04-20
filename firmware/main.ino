
// ==========================
// Libraries (Networking + Server + File System)
// ==========================
#include <WiFi.h>          // WiFi connection
#include <WiFiClient.h>    // Network client
#include <WebServer.h>     // HTTP server
#include <ESPmDNS.h>       // Local domain (mDNS)
#include <WiFiManager.h>   // Auto WiFi + Captive Portal
#include <SPIFFS.h>        // Internal file system

// ==========================
// Web Server (port 80)
// ==========================
WebServer server(80);


// ==========================
// WiFi Credentials (fallback)
// ==========================
const char* ssid = "Honor X8";
const char* password = "Ms2-1234";


// ==========================
// GPIO Pins (LEDs)
// ==========================
int a = 26;
int b = 25;
int c = 33;


// ==========================
// LED States (for tracking)
// ==========================
bool redLEDState = false;
bool blueLEDState = false;
bool whiteLEDState = false;


void setup() {

  // ==========================
  // Serial (debug output)
  // ==========================
  Serial.begin(115200);


  // ==========================
  // SPIFFS Initialization
  // Mount internal storage
  // ==========================
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  }

  // ==========================
  // Print all files in memory
  // (Debug: see what is inside SPIFFS)
  // ==========================
  listFiles();


  // ==========================
  // WiFi Mode (Station)
  // ==========================
  WiFi.mode(WIFI_STA);


  // ==========================
  // WiFiManager (Auto connect or create AP)
  // ==========================
  WiFiManager wm;
  bool response = wm.autoConnect(ssid, password);

  if (!response){
    Serial.println("not connected!");
  } else {
    Serial.println("connected!");
  }


  // ==========================
  // Wait until WiFi connected
  // ==========================
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  // ==========================
  // Show network info
  // ==========================
  Serial.print("Connected to:");
  Serial.println(ssid);

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());


  // ==========================
  // mDNS (local domain access)
  // Example: http://controlLED.local
  // ==========================
  if (MDNS.begin("controlLED")){
    Serial.println("mDNS started");
    Serial.println("URL : http://controlLED.local");
  }


  // ==========================
  // API Routes (Endpoints)
  // ==========================
  server.on("/on", onHandle);               // turn LED ON
  server.on("/off", offHandle);             // turn LED OFF
  server.on("/brightness", brightnessHandle); // set brightness


  // ==========================
  // Auto File Server (VERY IMPORTANT)
  // Handles ALL HTML/CSS/JS/images
  // ==========================
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "File Not Found");
    }
  });


  // ==========================
  // GPIO Setup
  // ==========================
  pinMode(a, OUTPUT);
  pinMode(b, OUTPUT);
  pinMode(c, OUTPUT);


  // ==========================
  // PWM Setup (LEDC)
  // ==========================
  ledcSetup(0,5000,8);
  ledcAttachPin(a,0);

  ledcSetup(1,5000,8);
  ledcAttachPin(b,1);

  ledcSetup(2,5000,8);
  ledcAttachPin(c,2);


  ledcWrite(0,255);
  ledcWrite(1,255);
  ledcWrite(2,255);
  delay(200);
  
  ledcWrite(0,0);
  ledcWrite(1,0);
  ledcWrite(2,0);
  delay(200);
  
  ledcWrite(0,255);
  ledcWrite(1,255);
  ledcWrite(2,255);
  delay(200);

  ledcWrite(0,0);
  ledcWrite(1,0);
  ledcWrite(2,0);
  delay(200);

  ledcWrite(0,255);
  ledcWrite(1,255);
  ledcWrite(2,255);
  delay(200);
  
  ledcWrite(0,0);
  ledcWrite(1,0);
  ledcWrite(2,0);
  delay(200);
  

  // ==========================
  // Start Server
  // ==========================
  server.begin();
  Serial.println("server started");
}



// ==========================
// Main Loop (handle requests)
// ==========================
void loop() {
  server.handleClient();
}



// ==========================
// API: Turn ON LED
// ==========================
void onHandle(){

  int LEDNumber = server.arg("led").toInt();

  switch(LEDNumber){
    case 1:
      ledcWrite(0,255);
      redLEDState = true;
      break;

    case 2:
      ledcWrite(1,255);
      blueLEDState = true;
      break;

    case 3:
      ledcWrite(2,255);
      whiteLEDState = true;
      break;

    default:
      server.send(200,"text/plain","Invalid LED");
      return;
  }

  server.send(200,"text/plain","ON");
  Serial.println("ON API CALLED");
}



// ==========================
// API: Turn OFF LED
// ==========================
void offHandle(){

  int LEDNumber = server.arg("led").toInt();

  switch(LEDNumber){
    case 1:
      ledcWrite(0,0);
      redLEDState = false;
      break;

    case 2:
      ledcWrite(1,0);
      blueLEDState = false;
      break;

    case 3:
      ledcWrite(2,0);
      whiteLEDState = false;
      break;

    default:
      server.send(200,"text/plain","Invalid LED");
      return;
  }

  server.send(200,"text/plain","OFF");
  Serial.println("OFF API CALLED");
}



// ==========================
// API: Set Brightness
// ==========================
void brightnessHandle(){

  int LEDNumber = server.arg("led").toInt();
  int LEDBrightness = server.arg("value").toInt();

  switch (LEDNumber){
    case 1: 
      ledcWrite(0,map(LEDBrightness,0,100,0,255)); 
      break;
    case 2: 
      ledcWrite(1,map(LEDBrightness,0,100,0,255));
      break;
    case 3: 
      ledcWrite(2,map(LEDBrightness,0,100,0,255));
      break;
  }

  server.send(200,"text/plain",String(LEDBrightness));
}



// ==========================
// Detect file type (MIME)
// ==========================
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".webp")) return "image/webp";
  return "text/plain";
}


// ==========================
// Read file from SPIFFS
// and send to browser
// ==========================

bool handleFileRead(String path){
  if (path.endsWith("/")){
    path += "index.html"; 
  }
  String contentType = getContentType(path);

  File file = SPIFFS.open(path ,"r");
  if (!file) {
    return false;
  } else {
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
}


// ==========================
// List all files in SPIFFS
// ==========================
void listFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while(file){
    Serial.print("FILE: ");
    Serial.print(file.name());
    Serial.print("  SIZE: ");
    Serial.println(file.size());

    file = root.openNextFile();
  }
}
