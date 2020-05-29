/*
27/05/2020
language: c++/arduino
developed for: ESP8266 WIFI development board
intended for use with: L298N H-Bridge, dc motors, one servo motor
thanks to ACROBOTIC for inspiration https://youtu.be/4gl7IZLo7yA.
check notes for how to load spiffs data in arduino ide.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
//SPI flash file system library to load websiteinto flash memory.
#include <FS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Servo.h>

// assign pin numbers for L298N enable and inputs.
int enA = 13;
int in1 = 12;
int in2 = 14;

int enB = 5;
int in3 = 2;
int in4 = 4;

//give servo data line a pin number.
int servo = 0;
// name of servo motor for reference
Servo steering;
// starting "forward position".
int pos = 90;

ESP8266WebServer server;
//change these for your own network credentials
//char* ssid = "INSERTSSID";
char* ssid = "Phone";
//char* password = "YOURWPANPASSWORD";
char* password = "bobbobbob";

// hold uploaded file
File fsUploadFile;

// websockets server for dealing with messages sent by the client
WebSocketsServer webSocket = WebSocketsServer(81);

void setup(){
  SPIFFS.begin();
  WiFi.begin(ssid,password);
  Serial.begin(115200);
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/",ControlDataFile);
  // list available files
  server.on("/list", HTTP_GET, FileList);
  // handle file upload
  server.on("/upload", HTTP_POST, [](){
    server.send(200, "text/plain", "{\"success\":1}");
  }, FileUpload);
  server.begin();
  webSocket.begin();
  // function to be called whenever there's a websocket event
  webSocket.onEvent(webSocketEvent);

  //set outputs.
  pinMode(enA,OUTPUT);
  pinMode(enB,OUTPUT);
  pinMode(in1,OUTPUT);
  pinMode(in2,OUTPUT);
  pinMode(in3,OUTPUT);
  pinMode(in4,OUTPUT);
  pinMode(servo,OUTPUT);
  
  steering.attach(0);


}

void loop(){
  webSocket.loop();
  server.handleClient();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length){
  if(type == WStype_TEXT){
    // handle the websocket messages with direction and speed
    // by parsing the parameters from a JSON string
    String payload_str = String((char*) payload);
    // using the ArduinoJson library
    StaticJsonDocument<200> doc;
    // deserialize the data
    DeserializationError error = deserializeJson(doc, payload_str);
    // parse the parameters we expect to receive (TO-DO: error handling)
    String dir = doc["direction"];
    
    if(dir == "STP") {
      digitalWrite(in1,LOW);
      digitalWrite(in2,LOW);
      digitalWrite(in3,LOW);
      digitalWrite(in4,LOW);

      Serial.println("STOP");
    } else {
      int speed = doc["speed"];
      // on the page speed goes from 0 to 100. scale it between 100 and 800 this will be dependant on your boards analog write resolution.
      speed = map(speed, 0, 100, 0, 800);
      analogWrite(enA, speed);
      analogWrite(enB, speed);
      if(dir == "FWD") {
        // set steering to forward position and set motors in forward direction using input lines.
        steering.write(pos);
        digitalWrite(in1,LOW);
        digitalWrite(in2,HIGH);
        digitalWrite(in3,LOW);
        digitalWrite(in4,HIGH);
        Serial.println("FORWARD");
      } else if(dir == "BWD") {
        // set steering to forward position and set motors in backward direction using input lines.
        steering.write(pos);
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        digitalWrite(in3, HIGH);
        digitalWrite(in4, LOW);
        Serial.println("BACKWARD");
      } else if(dir == "RGT") {
        // set steering to 120 degrees "right" position and set motors in forward direction using input lines.
        steering.write(130);
        digitalWrite(in1,LOW);
        digitalWrite(in2,HIGH);
        digitalWrite(in3,LOW);
        digitalWrite(in4,HIGH);
        Serial.println("RIGHT");
      } else if(dir == "LFT") {
        // set steering to 60 degrees "left" position and set motors to forward direction using input lines.
        steering.write(60);
        digitalWrite(in1,LOW);
        digitalWrite(in2,HIGH);
        digitalWrite(in3,LOW);
        digitalWrite(in4,HIGH);
        Serial.println("LEFT");
      }
    }
  }
}

void FileUpload(){
  // deal fith data file upload over httpupload.
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")){
      filename = "/"+filename;
    }
    Serial.print("FileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile){
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile){
      fsUploadFile.close();
    }
    Serial.print("FileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void FileList(){
  // updates list of  html pages for controling the car.
  String path = "/";
  // Assuming there are no subdirectories
  Dir dir = SPIFFS.openDir(path);
  String output = "[";
  while(dir.next())
  {
    File entry = dir.openFile("r");
    // Separate by comma if there are multiple files
    if(output != "[")
      output += ",";
    output += String(entry.name()).substring(1);
    entry.close();
  }
  output += "]";
  server.send(200, "text/plain", output);
}

void ControlDataFile(){
  // initiates html page on server to control the car.
  File file = SPIFFS.open("/index.html","r");
  server.streamFile(file, "text/html");
  file.close();
}
