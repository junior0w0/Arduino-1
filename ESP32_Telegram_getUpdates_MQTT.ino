/*
ESP32 Telegram Bot with MQTT

Author : ChungYi Fu (Kaohsiung, Taiwan)  2022-4-6 18:00
https://www.facebook.com/francefu

ArduinoJson Library：
https://github.com/bblanchon/ArduinoJson
PubSubClient Library：
https://www.arduino.cc/reference/en/libraries/pubsubclient/

Telegram Bot API
https://core.telegram.org/bots/api
*/

// Enter your WiFi ssid and password
const char* ssid     = "teacher";   //your network SSID
const char* password = "87654321";   //your network password

String token = "163669573612:AAGLi8kHrFbRWd6gpsq1ToxBTmvxH-UQJ7s";   // Create your bot and get the token -> https://telegram.me/fatherbot
String chat_id = "90190184712";   // Get chat_id -> https://telegram.me/userinfobot

const char* mqtt_server = "broker.emqx.io";
const unsigned int mqtt_port = 1883;
#define MQTT_USER               ""
#define MQTT_PASSWORD           ""
#define MQTT_PUBLISH_TOPIC    "yourtopic/result"
#define MQTT_SUBSCRIBE_TOPIC    "yourtopic/command"

/*
If "sendHelp" variable is equal to "true", it will send the command list to Telegram Bot when the board boots. 
If you don't want to get the command list when the board restarts every time, you can set the value to "false".
But you need to input the command "help" or "/help" in Telegram APP to get the command list after the board booting.
*/
boolean sendHelp = true;   

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure client_tcp;
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client_mqtt(espClient);

long message_id_last = 0;

void executeCommand(String text) {
  if (!text||text=="") return;
    
  // Custom command
  if (text=="help"||text=="/help"||text=="/start") {
    String command = "/help Command list\n/on Turn on the led\n/off Turn off the led\n/restart Restart the board";
    
    //One row
    String keyboard = "{\"keyboard\":[[{\"text\":\"/on\"},{\"text\":\"/off\"},{\"text\":\"/restart\"}]],\"one_time_keyboard\":false}";
    
    //Two rows
    //String keyboard = "{\"keyboard\":[[{\"text\":\"/on\"},{\"text\":\"/off\"}], [{\"text\":\"/restart\"}]],\"one_time_keyboard\":false}";
    
    sendMessage2Telegram(command, keyboard);
  }        
  else if (text=="/on") {
    pinMode(16, OUTPUT);
    digitalWrite(16,HIGH);
    sendMessage2MQTT("Turn on the led");
    sendMessage2Telegram("Turn on the led", "");
  }
  else if (text=="/off") {
    pinMode(16, OUTPUT);
    digitalWrite(16,LOW);
    sendMessage2MQTT("Turn off the led");
    sendMessage2Telegram("Turn off the led", "");
  }
  else if (text=="/restart") {
    sendMessage2Telegram("Restart the board", "");
    ESP.restart();
  } 
  else if (text=="null") {   //Server sends this response unexpectedly. Don't delete the code.
    client_tcp.stop();
    getTelegramMessage();
  }
  else
    sendMessage2Telegram("Command is not defined", "");
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  randomSeed(micros());
  client_mqtt.setServer(mqtt_server,mqtt_port);
  client_mqtt.setCallback(callback);
  //client_mqtt.setBufferSize(1024);
  
  Serial.begin(115200);
  delay(10);
  
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  
  
  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    if ((StartTime+10000) < millis()) break;
  } 

  Serial.println("");
  Serial.println("STAIP address: ");
  Serial.println(WiFi.localIP());
    
  Serial.println("");

  if (WiFi.status() != WL_CONNECTED) {
    pinMode(2, OUTPUT);
    for (int i=0;i<2;i++)
    {
      digitalWrite(2,HIGH);
      delay(500);
      digitalWrite(2,LOW);
      delay(500);
    }   
    
    ESP.restart();
  }
  else {
    pinMode(2, OUTPUT);
    for (int i=0;i<5;i++)
    {
      digitalWrite(2,HIGH);
      delay(100);
      digitalWrite(2,LOW);
      delay(100);
    }     
  }

  randomSeed(micros());
  client_mqtt.setServer(mqtt_server, mqtt_port);
  client_mqtt.setCallback(callback);  

  //Get your latest message from Telegram Bot.
  getTelegramMessage();  
}

void loop()
{
}

void getTelegramMessage() {
  const char* myDomain = "api.telegram.org";
  String getAll="", getBody = ""; 
  JsonObject obj;
  DynamicJsonDocument doc(1024);
  String result;
  long update_id;
  String message;
  long message_id;
  String text;  
  
  client_tcp.setInsecure();   //run version 1.0.5 or above
  
  if (message_id_last == 0) Serial.println("Connect to " + String(myDomain));
  if (client_tcp.connect(myDomain, 443)) {
    if (message_id_last == 0) Serial.println("Connection successful");

    while (client_tcp.connected()) {            
      getAll = "";
      getBody = "";

      String request = "limit=1&offset=-1&allowed_updates=message";
      client_tcp.println("POST /bot"+token+"/getUpdates HTTP/1.1");
      client_tcp.println("Host: " + String(myDomain));
      client_tcp.println("Content-Length: " + String(request.length()));
      client_tcp.println("Content-Type: application/x-www-form-urlencoded");
      client_tcp.println("Connection: keep-alive");
      client_tcp.println();
      client_tcp.print(request);
      
      int waitTime = 5000;   // timeout 5 seconds
      long startTime = millis();
      boolean state = false;
      
      while ((startTime + waitTime) > millis())
      {
        //Serial.print(".");
        delay(100);      
        while (client_tcp.available()) 
        {
            char c = client_tcp.read();
            if (c == '\n') 
            {
              if (getAll.length()==0) state=true; 
              getAll = "";
            } 
            else if (c != '\r')
              getAll += String(c);
            if (state==true) getBody += String(c);
            startTime = millis();
         }
         if (getBody.length()>0) break;
      }
      
      deserializeJson(doc, getBody);
      obj = doc.as<JsonObject>();
      //result = obj["result"].as<String>();
      //update_id =  obj["result"][0]["update_id"].as<String>().toInt();
      //message = obj["result"][0]["message"].as<String>();
      message_id = obj["result"][0]["message"]["message_id"].as<String>().toInt();
      text = obj["result"][0]["message"]["text"].as<String>();

      if (message_id!=message_id_last&&message_id) {
        int id_last = message_id_last;
        message_id_last = message_id;
        if (id_last==0) {
          message_id = 0;
          if (sendHelp == true)   // Send the command list to Telegram Bot when the board boots.
            text = "/help";
          else
            text = "";
        }
        else {
          Serial.println(getBody);
          Serial.println();
        }
        
        if (text!="") {
          Serial.println("["+String(message_id)+"] "+text);
          executeCommand(text);
        }
      }

      if (!client_mqtt.connected()) {
        reconnect();
      }
      client_mqtt.loop();
  
      delay(1000);
    }
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connected failed.");
    WiFi.begin(ssid, password);  
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      if ((StartTime+10000) < millis()) break;
    } 
    if (WiFi.status() != WL_CONNECTED) {
      ESP.restart();
    }
    Serial.println("Reconnection is successful.");
  }
 
  getTelegramMessage();   // Client's connection time out after about 3 minutes.
}

void sendMessage2Telegram(String text, String keyboard) {
  const char* myDomain = "api.telegram.org";
  String getAll="", getBody = "";
  
  String request = "parse_mode=HTML&chat_id="+chat_id+"&text="+text;
  if (keyboard!="") request += "&reply_markup="+keyboard;
  
  client_tcp.println("POST /bot"+token+"/sendMessage HTTP/1.1");
  client_tcp.println("Host: " + String(myDomain));
  client_tcp.println("Content-Length: " + String(request.length()));
  client_tcp.println("Content-Type: application/x-www-form-urlencoded");
  client_tcp.println("Connection: keep-alive");
  client_tcp.println();
  client_tcp.print(request);
  
  int waitTime = 5000;   // timeout 5 seconds
  long startTime = millis();
  boolean state = false;
  
  while ((startTime + waitTime) > millis())
  {
    Serial.print(".");
    delay(100);      
    while (client_tcp.available()) 
    {
        char c = client_tcp.read();
        if (state==true) getBody += String(c);      
        if (c == '\n') 
        {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTime = millis();
     }
     if (getBody.length()>0) break;
  }
  Serial.println(getBody);
  Serial.println();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[");
  Serial.print(topic);
  Serial.print("] ");
  String data = "";
  for (int i = 0; i < length; i++) {
    char c = payload[i];
    data += String(c);
  }
  Serial.println(data);
  executeCommand(data);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client_mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client_mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish(MQTT_PUBLISH_TOPIC, "hello world");
      // ... and resubscribe
      client_mqtt.subscribe(MQTT_SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client_mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendMessage2MQTT(String text) {
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    if (client_mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      client_mqtt.publish(MQTT_PUBLISH_TOPIC, text.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client_mqtt.state());
    }
}
