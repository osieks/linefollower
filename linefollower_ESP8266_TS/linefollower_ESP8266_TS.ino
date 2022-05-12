/*
    Your code description here
    (c) you, 20XX
    All rights reserved.

    Functionality: 
    
    Version log:
    
    20XX-MM-DD:
        v1.0.0  - Initial release
        
*/
// ==== DEFINES ===================================================================================

// ==== Debug and Test options ==================
#define _DEBUG_
//#define _TEST_

//===== Debugging macros ========================
#ifdef _DEBUG_
#define SerialD Serial
#define _PM(a) SerialD.print(millis()); SerialD.print(": "); SerialD.println(a)
#define _PP(a) SerialD.print(a)
#define _PL(a) SerialD.println(a)
#define _PX(a) SerialD.println(a, HEX)
#else
#define _PM(a)
#define _PP(a)
#define _PL(a)
#define _PX(a)
#endif




// ==== INCLUDES ==================================================================================

// ==== Uncomment desired compile options =================================
// #define _TASK_SLEEP_ON_IDLE_RUN  // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
// #define _TASK_TIMECRITICAL       // Enable monitoring scheduling overruns
// #define _TASK_STATUS_REQUEST     // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
// #define _TASK_WDT_IDS            // Compile with support for wdt control points and task ids
// #define _TASK_LTS_POINTER        // Compile with support for local task storage pointer
// #define _TASK_PRIORITY           // Support for layered scheduling priority
// #define _TASK_MICRO_RES          // Support for microsecond resolution
// #define _TASK_STD_FUNCTION       // Support for std::function (ESP8266 and ESP32 ONLY)
// #define _TASK_DEBUG              // Make all methods and variables public for debug purposes
// #define _TASK_INLINE             // Make all methods "inline" - needed to support some multi-tab, multi-file implementations
// #define _TASK_TIMEOUT            // Support for overall task timeout
// #define _TASK_OO_CALLBACKS       // Support for dynamic callback method binding
// #define _TASK_DEFINE_MILLIS      // Force forward declaration of millis() and micros() "C" style
// #define _TASK_EXPOSE_CHAIN       // Methods to access tasks in the task chain
// #define _TASK_SCHEDULING_OPTIONS // Support for multiple scheduling options

#include <TaskScheduler.h>



// ==== GLOBALS ===================================================================================
// ==== Scheduler ==============================
Scheduler ts;

void task1Callback();
void task2Callback();
void GetLineFromSerial();
void PIDRegulaiton();
void WiFiServerControls();

// ==== Scheduling defines (cheat sheet) =====================
/*
  TASK_MILLISECOND
  TASK_SECOND
  TASK_MINUTE
  TASK_HOUR
  TASK_IMMEDIATE
  TASK_FOREVER
  TASK_ONCE
  TASK_NOTIMEOUT
  
  TASK_SCHEDULE     - schedule is a priority, with "catch up" (default)
  TASK_SCHEDULE_NC  - schedule is a priority, without "catch up"
  TASK_INTERVAL     - interval is a priority, without "catch up"
*/

// ==== Task definitions ========================
Task t1 (10 * TASK_MILLISECOND, TASK_FOREVER, &GetLineFromSerial, &ts, true);
Task t2 (10 * TASK_MILLISECOND, TASK_FOREVER, &PIDRegulaiton, &ts, true);
Task t3 (10 * TASK_MILLISECOND, TASK_FOREVER, &WiFiServerControls, &ts, true);
//Task t2 (TASK_IMMEDIATE, 100, &task2Callback, &ts, true);



// ==== CODE ======================================================================================

#include <SoftwareSerial.h>

//for ota
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

//#include <ESPArto.h>

SoftwareSerial serial(3,1);

const char *ssid     = "osiek";
const char *password = "osiekrulz";

//WifiServer
WiFiServer server(300);

#include <SparkFun_TB6612.h>

#define AIN1 D3
#define AIN2 D4
//pozamieniane w kierunku
#define BIN1 D2
#define BIN2 D1
#define PWMA D5
#define PWMB D6
#define STBY D0

const int offsetA = 1;
const int offsetB = 1;

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY);
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY);

bool no_back = LOW;

/**************************************************************************/
/*!
    @brief    Standard Arduino SETUP method - initialize sketch
    @param    none
    @returns  none
*/
/**************************************************************************/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //Serial.begin(19200);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    //delay(5000);
    //ESP.restart();
  }
  
  // OTA
  ArduinoOTA.setHostname("esp8266_linefollower_Dziezok_Piorko");
  
  // No authentication by default
  ArduinoOTA.setPassword("linefollower");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //END OF OTA

  //WIFI SERVER
      
  server.begin(300);
  Serial.println("Server started");
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

}


uint8_t p[2];

String request; // for server
unsigned long currentMillis = millis();
unsigned long previousMillis = millis();
//unsigned long elapsedTime = 0;

bool automat=LOW;
bool debug=LOW;

#define MAX_MOTOR_SPEED 255 //wynika z ograniczenia

int MOTOR_SPEED_MANU= 40;
int MOTOR_SPEED_AUTO= 60;
int offset_A=0;
int offset_B=0;

double Position = 0;
double error = 0;
double last_error = 0;
double PID_P = 0;
double PID_I = 0;
double PID_D = 0;
double PID_output = 0;

double  Kp = 0.05;
double  Ki = 0.0003;
double  Kd = 0.05;

double Speed_Limit = 100;// I DONT KNOW YET
double Speed = 100;// I DONT KNOW YET

int posi = 3500;
int prev_posi;
int posi_mid=3500;
int motor_speed_a = 0;
int motor_speed_b = 0;

int get_input=0;
int get_input_end=0;

/**************************************************************************/
/*!
    @brief    Standard Arduino LOOP method - using with TaskScheduler there 
              should be nothing here but ts.execute()
    @param    none
    @returns  none
*/
/**************************************************************************/
void loop() {
  ArduinoOTA.handle();
  ts.execute();
}


/**************************************************************************/
/*!
    @brief    Callback method of task1 - explain
    @param    none
    @returns  none
*/
/**************************************************************************/
void task1Callback() {
_PM("task1Callback()");
//  task code
}


/**************************************************************************/
/*!
    @brief    Callback method of task2 - explain
    @param    none
    @returns  none
*/
/**************************************************************************/
void task2Callback() {
_PM("task2Callback()");
//  task code
}


void GetLineFromSerial(){
_PM("GetLineFromSerial()");
    Serial.read(p,2);
    posi = (p[0]<<8)+p[1];
    if(posi >7000 || posi < 0){
      posi = prev_posi;
    }
    prev_posi = posi;
    if(debug==HIGH)Serial.println(posi);//cheat

}

void PIDRegulaiton(){
_PM("PIDRegulaiton()");
    if(automat==HIGH){
      error = posi_mid - posi;
      PID_P = error;   
      PID_I = PID_I + error;  
      PID_D = error - last_error ;
      last_error = error;
      
      //PID_I += Ki * (error * (elapsedTime)); 
      //PID_D = Kd * (error - last_error)/(elapsedTime);               
      if(PID_I>3500){
        PID_I=3500;
      }else if(PID_I<-3500){
        PID_I=-3500;  
      }  
      
      PID_output = Kp*PID_P  + Ki*PID_I  + Kd*PID_D;
      
      if(debug==HIGH){
        Serial.print(" error");
        Serial.print(error);
        Serial.print("|| PID_P ");
        Serial.print(PID_P);
        Serial.print(" + PID_I ");
        Serial.print(PID_I);
        Serial.print(" + PID_D ");
        Serial.print(PID_D);
        Serial.print(" = PID_output ");
        Serial.println(PID_output);
      }

      motor_speed_a = MOTOR_SPEED_AUTO - PID_output + offset_A;
      motor_speed_b = MOTOR_SPEED_AUTO + PID_output + offset_B;

      if(debug==HIGH){
        Serial.print("motor_speed_a raw =");
        Serial.print(motor_speed_a);
        Serial.print("motor_speed_b raw =");
        Serial.println(motor_speed_b);
      }
      //for motor a
      if(motor_speed_a>MAX_MOTOR_SPEED){
        motor_speed_a=MAX_MOTOR_SPEED;
      }else if(motor_speed_a<-MAX_MOTOR_SPEED){
        motor_speed_a=-MAX_MOTOR_SPEED;
      }

      //for motor b
      if(motor_speed_b>MAX_MOTOR_SPEED){
        motor_speed_b=MAX_MOTOR_SPEED;
      }else if(motor_speed_b<-MAX_MOTOR_SPEED){
        motor_speed_b=-MAX_MOTOR_SPEED;
      }
      if(no_back == HIGH){
        if(motor_speed_a<0)motor_speed_a=0;
        if(motor_speed_b<0)motor_speed_b=0;
      }
      if(debug==HIGH){
        Serial.print("motor_speed_a =");
        Serial.print(motor_speed_a);
        Serial.print("motor_speed_b =");
        Serial.println(motor_speed_b);
      }
      motor1.drive(motor_speed_a);
      motor2.drive(motor_speed_b);
  }
}

void WiFiServerControls(){
_PM("WiFiServerControls()");
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      WiFi.begin(ssid, password);
    }
    
    // SERVER WIFI
    WiFiClient client = server.available();
    if (client) {
      //Serial.print("client");
      request = client.readStringUntil('\r');
      client.flush();

      if (request.indexOf("/WIFI_CONTROL=UP") != -1) {
        if(debug==HIGH)Serial.println("forward");
        motor1.drive(MOTOR_SPEED_MANU+offset_A);
        motor2.drive(MOTOR_SPEED_MANU+offset_B);
      }else if (request.indexOf("/WIFI_CONTROL=DOWN") != -1) {
        if(debug==HIGH)Serial.println("back");
        motor1.drive(-MOTOR_SPEED_MANU-offset_A);
        motor2.drive(-MOTOR_SPEED_MANU-offset_B);
      }else if (request.indexOf("/WIFI_CONTROL=STRAFE_LEFT") != -1) {
        if(debug==HIGH)Serial.println("strafe left");
        motor1.brake();
        motor2.drive(MOTOR_SPEED_MANU);
      }else if (request.indexOf("/WIFI_CONTROL=STRAFE_RIGHT") != -1) {
        if(debug==HIGH)Serial.println("strafe right");
        motor1.drive(MOTOR_SPEED_MANU+offset_A);
        motor2.brake();
      }else if (request.indexOf("/WIFI_CONTROL=STOP") != -1) {
        if(debug==HIGH)Serial.println("break");
        motor1.brake();
        motor2.brake();
      }else if (request.indexOf("/WIFI_CONTROL=LEFT") != -1) {
        if(debug==HIGH)Serial.println("left");
        motor1.drive(-MOTOR_SPEED_MANU-offset_A);
        motor2.drive(MOTOR_SPEED_MANU+offset_B);
      }else if (request.indexOf("/WIFI_CONTROL=RIGHT") != -1) {
        if(debug==HIGH)Serial.println("right");
        motor1.drive(MOTOR_SPEED_MANU+offset_A);
        motor2.drive(-MOTOR_SPEED_MANU-offset_B);
      }else if (request.indexOf("/WIFI_CONTROL=BACK_LEFT") != -1) {
        if(debug==HIGH)Serial.println("back left");
        motor1.brake();
        motor2.drive(-MOTOR_SPEED_MANU-offset_B);
      }else if (request.indexOf("/WIFI_CONTROL=BACK_RIGHT") != -1) {
        if(debug==HIGH)Serial.println("back right");
        motor1.drive(-MOTOR_SPEED_MANU-offset_A);
        motor2.brake();
      }else if (request.indexOf("/WIFI_AUTOMAT=ON") != -1) {
        automat=HIGH;
      }else if (request.indexOf("/WIFI_AUTOMAT=OFF") != -1) {
        automat=LOW;
        motor1.brake();
        motor2.brake();
      }else if (request.indexOf("/WIFI_DEBUG=ON") != -1) {
        debug=HIGH;
      } else if (request.indexOf("/WIFI_DEBUG=OFF") != -1) {
        debug=LOW;
      } else if (request.indexOf("/WIFI_NO_BACK=ON") != -1) {
        no_back=HIGH;
      } else if (request.indexOf("/WIFI_NO_BACK=OFF") != -1) {
        no_back=LOW;
      }  else if (request.indexOf("/get?input1") != -1) {
        get_input = request.indexOf("=")+1;
        get_input_end = request.indexOf("HTTP/1.1");
        Serial.println(request.substring(get_input,get_input_end));
        Kp=request.substring(get_input,get_input_end).toFloat();
      }   else if (request.indexOf("/get?input2") != -1) {
        get_input = request.indexOf("=")+1;
        get_input_end = request.indexOf("HTTP/1.1");
        Serial.println(request.substring(get_input,get_input_end));
        Ki=request.substring(get_input,get_input_end).toFloat();
      }   else if (request.indexOf("/get?input3") != -1) {
        get_input = request.indexOf("=")+1;
        get_input_end = request.indexOf("HTTP/1.1");
        Serial.println(request.substring(get_input,get_input_end));
        Kd=request.substring(get_input,get_input_end).toFloat();
      }else if (request.indexOf("/get?input4") != -1) {
        get_input = request.indexOf("=")+1;
        get_input_end = request.indexOf("HTTP/1.1");
        Serial.println(request.substring(get_input,get_input_end));
        MOTOR_SPEED_MANU=request.substring(get_input,get_input_end).toFloat();
      }else if (request.indexOf("/get?input5") != -1) {
        get_input = request.indexOf("=")+1;
        get_input_end = request.indexOf("HTTP/1.1");
        Serial.println(request.substring(get_input,get_input_end));
        MOTOR_SPEED_AUTO=request.substring(get_input,get_input_end).toFloat();
      }else if (request.indexOf("/get?input6") != -1) {
        get_input = request.indexOf("=")+1;
        get_input_end = request.indexOf("HTTP/1.1");
        Serial.println(request.substring(get_input,get_input_end));
        offset_A=request.substring(get_input,get_input_end).toFloat();
      }else if (request.indexOf("/get?input7") != -1) {
        get_input = request.indexOf("=")+1;
        get_input_end = request.indexOf("HTTP/1.1");
        Serial.println(request.substring(get_input,get_input_end));
        offset_B=request.substring(get_input,get_input_end).toFloat();
      } 

      // Return the response
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println(""); //  do not forget this one
      client.println("<!DOCTYPE HTML>");
      client.println("<html><head>");
      client.println("<title> LINE FOLLOWER </title><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>");
      client.println("<style>.tab,.tab td{border:1px solid black;}</style>");
      if (request.indexOf("=") != -1){
        request="";
        client.print("<meta http-equiv='refresh' content='0;URL=/'");
        client.print(WiFi.localIP());
        client.println(":300/''>");
        client.println("</head>");
      }else{
        client.println("</head><body>");
        

        if(automat==LOW){
          client.println("<table>");
          client.println("<tr>");
          client.println("<td>");
          client.println("<span style='font-size:100px;'><a style='text-decoration:none;' href=\"/WIFI_CONTROL=STRAFE_LEFT\">&#8598;</a></span>");
          client.println("</td>");
          client.println("<td>");
          client.println("<span style='font-size:100px;'><a style='text-decoration:none;' href=\"/WIFI_CONTROL=UP\">&uarr;</a></span>");
          client.println("</td>");
          client.println("<td>");
          client.println("<span style='font-size:100px;'><a style='text-decoration:none;' href=\"/WIFI_CONTROL=STRAFE_RIGHT\">&#8599;</a></span>");
          client.println("</td>");
          client.println("</tr>");
          client.println("<tr>");
          client.println("<td>");
          client.println("<span style='font-size:100px;'><a  style='text-decoration:none;' href=\"/WIFI_CONTROL=LEFT\">&#8592;</a></span>");
          client.println("</td>");
          client.println("<td>");
          client.println("<span style='font-size:100px;'><a style='text-decoration:none;' href=\"/WIFI_CONTROL=STOP\">&#9633;</a></span>");
          client.println("</td>");
          client.println("<td>");
          client.println("<span style='font-size:100px;'><a style='text-decoration:none;' href=\"/WIFI_CONTROL=RIGHT\">&#8594;</a></span>");
          client.println("</td>");
          client.println("</tr>");
          client.println("<tr>");
          client.println("<td>");
          client.println("<span style='font-size:100px;'><a style='text-decoration:none;' href=\"/WIFI_CONTROL=BACK_LEFT\">&#8601;</a></span>");
          client.println("</td>");           
          client.println("<td>");  
          client.println("<span style='font-size:100px;'><a style='text-decoration:none;' href=\"/WIFI_CONTROL=DOWN\">&darr;</a></span>");
          client.println("</td>");
          client.println("<td>");
          client.println("<span style='font-size:100px;'><a style='text-decoration:none;' href=\"/WIFI_CONTROL=BACK_RIGHT\">&#8600;</a></span>");
          client.println("</td>");
          client.println("</tr>");
          client.println("</table>");
        }
        client.println("<br/>");
        client.println("<form action='/get'>");
        client.println("  Prędkość Manu (teraz=");
        client.println(MOTOR_SPEED_MANU);
        client.println("): <input type='text' name='input44'>");
        client.println("  <input type='submit' value='Submit'>");
        client.println("</form><br>");
        client.println("<form action='/get'>");
        client.println("  Prędkość Auto (teraz=");
        client.println(MOTOR_SPEED_AUTO);
        client.println("): <input type='text' name='input5'>");
        client.println("  <input type='submit' value='Submit'>");
        client.println("</form><br>");
        client.println("<a ");
        if(automat==LOW){
          client.println("href='/WIFI_AUTOMAT=ON' style='background-color:red;padding:10px;color:white;'> AUTOMAT OFF");
        }else{
          client.println("href='/WIFI_AUTOMAT=OFF' style='background-color:green;padding:10px;color:white;'> AUTOMAT ON");
        }
        client.println("</a>");
        client.println("<a ");
        if(debug==LOW){
          client.println("href='/WIFI_DEBUG=ON' style='background-color:red;padding:10px;color:white;'> DEBUG OFF");
        }else{
          client.println("href='/WIFI_DEBUG=OFF' style='background-color:green;padding:10px;color:white;'> DEBUG ON");
        }
        client.println("</a>");
        client.println("<a ");
        if(no_back==LOW){
          client.println("href='/WIFI_NO_BACK=ON' style='background-color:red;padding:10px;color:white;'> NO_BACK OFF");
        }else{
          client.println("href='/WIFI_NO_BACK=OFF' style='background-color:green;padding:10px;color:white;'> NO_BACK ON");
        }
        client.println("</a>");
        client.println("<br>");
        client.println("<br>");
        /*
        if(automat==LOW){
          client.println("<a href='/WIFI_KALIBRACJA=ON' style='background-color:YELLOW;padding:10px;color:black;'> KALIBRACJA </a>");
        }
        */        
        client.println("<table class='tab'>");
        client.println("<tr>");
        client.println("<td>Kp</td>");
        client.println("<td>Ki</td>");
        client.println("<td>Kd</td>");
        client.println("</tr>");
        client.println("<tr>");
        client.println("<td>");
        client.println(Kp,5);
        client.println("</td>");
        client.println("<td>");
        client.println(Ki,5);
        client.println("</td>");
        client.println("<td>");
        client.println(Kd,5);
        client.println("</td>");
        client.println("</tr>");        
        client.println("<tr>");
        client.println("<td>P</td>");
        client.println("<td>I</td>");
        client.println("<td>D</td>");
        client.println("<td>OUT</td>");
        client.println("</tr>");        
        client.println("<tr>");
        client.println("<td>");
        client.println(PID_P);
        client.println("</td>");
        client.println("<td>");
        client.println(PID_I);
        client.println("</td>");
        client.println("<td>");
        client.println(PID_D);
        client.println("</td>");
        client.println("<td>");
        client.println(PID_output);
        client.println("</td>");
        client.println("</tr>");
        client.println("</table>");

        
        client.println("<form action='/get'>");
        client.println("  Kp: <input type='text' name='input1'>");
        client.println("  <input type='submit' value='Submit'>");
        client.println("</form><br>");
        client.println("<form action='/get'>");
        client.println("  Ki: <input type='text' name='input2'>");
        client.println("  <input type='submit' value='Submit'>");
        client.println("</form><br>");
        client.println("<form action='/get'>");
        client.println("  Kd: <input type='text' name='input3'>");
        client.println("  <input type='submit' value='Submit'>");
        client.println("</form><br>");
        
        client.println("<table class='tab'>");
        client.println("<tr>");
        client.println("<td>motor_speed_a</td>");
        client.println("<td>motor_speed_b</td>");
        client.println("</tr>");
        client.println("<tr>");
        client.println("<td>");
        client.println(motor_speed_a);
        client.println("</td>");
        client.println("<td>");
        client.println(motor_speed_b);
        client.println("</td>");
        client.println("</tr>");
        client.println("</table>");
        client.println("</br>");
        
        client.println("<form action='/get'>");
        client.println("  offset a (lewy): <input type='text' name='input6'>");
        client.println("  <input type='submit' value='Submit'>");
        client.println("</form><br>");
        client.println("<form action='/get'>");
        client.println("  offset b (prawy): <input type='text' name='input7'>");
        client.println("  <input type='submit' value='Submit'>");
        client.println("</form><br>");
        client.println("<table class='tab'>");
        client.println("<tr>");
        client.println("<td>offset a</td>");
        client.println("<td>offset b</td>");
        client.println("</tr>");
        client.println("<tr>");
        client.println("<td>");
        client.println(offset_A);
        client.println("</td>");
        client.println("<td>");
        client.println(offset_B);
        client.println("</td>");
        client.println("</tr>");
        client.println("</table>");
        client.println("</br>");
        
        client.println(request.substring(get_input,get_input_end));
        client.println("</body></html>");
      }
    }
}
