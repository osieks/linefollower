
#include <SoftwareSerial.h>

//for ota
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

SoftwareSerial serial(3,1);

const char *ssid     = "osiek";
const char *password = "osiekrulz";

const char* PARAM_INPUT_1 = "input1";

//WifiServer
WiFiServer server(300);

// This is the library for the TB6612 that contains the class Motor and all the
// functions
#include <SparkFun_TB6612.h>

// Pins for all inputs, keep in mind the PWM defines must be on PWM pins
// the default pins listed are the ones used on the Redbot (ROB-12097) with
// the exception of STBY which the Redbot controls with a physical switch
#define AIN1 D2
#define BIN1 D7
#define AIN2 D4
#define BIN2 D8
#define PWMA D5
#define PWMB D6
#define STBY D0

// these constants are used to allow you to make your motor configuration 
// line up with function names like forward.  Value can be 1 or -1
const int offsetA = 1;
const int offsetB = 1;

// Initializing motors.  The library will allow you to initialize as many
// motors as you have memory for.  If you are using functions like forward
// that take 2 motors as arguements you can either write new functions or
// call the function more than once.
Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY);
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY);


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


byte p[2];
//bool p_bool[8];
//byte p_max = 0;
//byte p_min = 255;

//int const ilosc_probek_do_kalibracji=10;
//byte p_granica[ilosc_probek_do_kalibracji];
//uint16_t p_granica_sum = 0;
//byte p_granica_final = 125;

//int i,j;
String request; // for server
unsigned long currentMillis = millis();
unsigned long previousMillis = millis();
//unsigned long elapsedTime = 0;

bool automat=LOW;
bool debug=LOW;

#define MAX_MOTOR_SPEED 255    /**< Sets the Maximum PWM Duty Cycle for Line Follower Robot 0=0% 255=100% */

int MOTOR_SPEED_MANU= 50;
int MOTOR_SPEED_AUTO= 100;

double Position = 0;
double error = 0;
double last_error = 0;
double PID_P = 0;
double PID_I = 0;
double PID_D = 0;
double PID_output = 0;

double  Kp = 0.07;
double  Ki = 0.0008;
double  Kd = 0.6;

double Speed_Limit = 100;// I DONT KNOW YET
double Speed = 100;// I DONT KNOW YET

int posi = 3500;
int prev_posi;
int posi_mid=3500;
int motor_speed_a = 0;
int motor_speed_b = 0;

int get_input=0;
int get_input_end=0;

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();

  
  currentMillis = millis();
  /****************************reset_request*******************************/
  if(currentMillis - previousMillis >= 10){
    //elapsedTime = (double)(currentMillis - previousMillis);
    previousMillis = currentMillis;
    
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      WiFi.begin(ssid, password);
    }

    Serial.read(p,2);
    posi = (p[0]<<8)+p[1];
    if(posi >7000 || posi < 0){
      posi = prev_posi;
    }
    prev_posi = posi;
    if(debug==HIGH)Serial.println(posi);//cheat
    
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

      motor_speed_a = MOTOR_SPEED_AUTO + PID_output;
      motor_speed_b = MOTOR_SPEED_AUTO - PID_output;

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
      
      if(debug==HIGH){
        Serial.print("motor_speed_a =");
        Serial.print(motor_speed_a);
        Serial.print("motor_speed_b =");
        Serial.println(motor_speed_b);
      }
      motor1.drive(motor_speed_a);
      motor2.drive(motor_speed_b);
      /*
      if(PID_output>Speed_Limit){
        PID_output = Speed_Limit;
      }else if(PID_output<-Speed_Limit){
        PID_output = -Speed_Limit;
      }

      if (PID_output < 0){
        //Motor_SetSpeed(Speed + PID_output, Speed);
      }
      else
      {
        //Motor_SetSpeed(Speed, Speed - PID_output);
      }
      */
    }

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      WiFi.begin(ssid, password);
      //ESP.restart();
    }
   
    /*
    for(i=0;i<=7;i++){
      if(debug=HIGH){
        Serial.print(p[i]);
        Serial.print(" ");
      }
      if(p[i]>p_granica_final){
        p_bool[i]=HIGH;
      }else{
        p_bool[i]=LOW;
      }
    }
    if(debug=HIGH)Serial.println();

    if(debug=HIGH){
    for(i=0;i<=7;i++){
      Serial.print(p_bool[i]);
      Serial.print(" ");
    }
    Serial.println();
    }
    */
    // SERVER WIFI
    WiFiClient client = server.available();
    if (client) {
      //Serial.print("client");
      request = client.readStringUntil('\r');
      client.flush();

      if (request.indexOf("/WIFI_CONTROL=UP") != -1) {
        if(debug==HIGH)Serial.println("forward");
        motor1.drive(MOTOR_SPEED_MANU);
        motor2.drive(MOTOR_SPEED_MANU);
      }else if (request.indexOf("/WIFI_CONTROL=DOWN") != -1) {
        if(debug==HIGH)Serial.println("back");
        motor1.drive(-MOTOR_SPEED_MANU);
        motor2.drive(-MOTOR_SPEED_MANU);
      }else if (request.indexOf("/WIFI_CONTROL=STRAFE_LEFT") != -1) {
        if(debug==HIGH)Serial.println("strafe left");
        motor1.brake();
        motor2.drive(MOTOR_SPEED_MANU);
      }else if (request.indexOf("/WIFI_CONTROL=STRAFE_RIGHT") != -1) {
        if(debug==HIGH)Serial.println("strafe right");
        motor1.drive(MOTOR_SPEED_MANU);
        motor2.brake();
      }else if (request.indexOf("/WIFI_CONTROL=STOP") != -1) {
        if(debug==HIGH)Serial.println("break");
        motor1.brake();
        motor2.brake();
      }else if (request.indexOf("/WIFI_CONTROL=LEFT") != -1) {
        if(debug==HIGH)Serial.println("left");
        motor1.drive(-MOTOR_SPEED_MANU);
        motor2.drive(MOTOR_SPEED_MANU);
      }else if (request.indexOf("/WIFI_CONTROL=RIGHT") != -1) {
        if(debug==HIGH)Serial.println("right");
        motor1.drive(MOTOR_SPEED_MANU);
        motor2.drive(-MOTOR_SPEED_MANU);
      }else if (request.indexOf("/WIFI_CONTROL=BACK_LEFT") != -1) {
        if(debug==HIGH)Serial.println("back left");
        motor1.brake();
        motor2.drive(-MOTOR_SPEED_MANU);
      }else if (request.indexOf("/WIFI_CONTROL=BACK_RIGHT") != -1) {
        if(debug==HIGH)Serial.println("back right");
        motor1.drive(-MOTOR_SPEED_MANU);
        motor2.brake();
      }else if (request.indexOf("/WIFI_AUTOMAT=ON") != -1) {
        automat=HIGH;
      }else if (request.indexOf("/WIFI_AUTOMAT=OFF") != -1) {
        automat=LOW;
      }else if (request.indexOf("/WIFI_DEBUG=ON") != -1) {
        debug=HIGH;
      } else if (request.indexOf("/WIFI_DEBUG=OFF") != -1) {
        debug=LOW;
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
        client.println(request.substring(get_input,get_input_end));
        client.println("</body></html>");
      }
    }
  }
}
