#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ESP32_MailClient.h"
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

/* secret.h */
#define SECRET_SSID "realme 3 Pro"    // replace MySSID with your WiFi network name
#define SECRET_PASS "20102000"  // replace MyPassword with your WiFi password
//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701


char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

#define SECRET_CH_ID 1517015      // replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY "F6CRDDPI1J3UW85H"   // replace XYZ with your channel write API Key 


unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

const int trigPin = 5;
const int echoPin = 18;


long duration;
float distanceCm;
//float distanceCm;




// To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
// YOU MUST ENABLE less secure app option https://myaccount.google.com/lesssecureapps?pli=1
#define emailSenderAccount    "sender.iiith@gmail.com"
#define emailSenderPassword   "alerting_12345"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "[ALERT]"

// Default Recipient Email Address
String inputMessage = "projectsiot21@gmail.com";
String enableEmailChecked = "checked";
String inputMessage2 = "true";
// Default Threshold distanceCm Value
String inputMessage3 = "15.0";
String lastdistance;
 
// HTML web page to handle 3 input fields (email_input, enable_email_input, threshold_input)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Email Notification with Dustbin level</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h2>ULTRASONIC Sensor</h2> 
  <h3>%distanceCm% inch</h3>
  <h2>ESP Email Notification</h2>
  <form action="/get">
    Email Address <input type="email" name="email_input" value="%EMAIL_INPUT%" required><br>
    Enable Email Notification <input type="checkbox" name="enable_email_input" value="true" %ENABLE_EMAIL%><br>
    Level Threshold <input type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br>
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

AsyncWebServer server(80);

// Replaces placeholder with DS18B20 values
String processor(const String& var){
  //Serial.println(var);
  if(var == "distanceCm"){
    return lastdistance;
  }
  else if(var == "EMAIL_INPUT"){
    return inputMessage;
  }
  else if(var == "ENABLE_EMAIL"){
    return enableEmailChecked;
  }
  else if(var <= "THRESHOLD"){
    return inputMessage3;
  }
  return String();
}

// Flag variable to keep track if email notification was sent or not
bool emailSent = false;

const char* PARAM_INPUT_1 = "email_input";
const char* PARAM_INPUT_2 = "enable_email_input";
const char* PARAM_INPUT_3 = "threshold_input";

// Interval between sensor readings. Learn more about timers: https://RandomNerdTutorials.com/esp32-pir-motion-sensor-interrupts-timers/
unsigned long previousMillis = 0;     
const long interval = 5000;    





// The Email Sending data object contains config and data to send
SMTPData smtpData;

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  ThingSpeak.begin(client);  
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
  
  // Start the DS18B20 sensor
 // sensors.begin();

  // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Receive an HTTP GET request at <ESP_IP>/get?email_input=<inputMessage>&enable_email_input=<inputMessage2>&threshold_input=<inputMessage3>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET email_input value on <ESP_IP>/get?email_input=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      // GET enable_email_input value on <ESP_IP>/get?enable_email_input=<inputMessage2>
      if (request->hasParam(PARAM_INPUT_2)) {
        inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
        enableEmailChecked = "checked";
      }
      else {
        inputMessage2 = "false";
        enableEmailChecked = "";
      }
      // GET threshold_input value on <ESP_IP>/get?threshold_input=<inputMessage3>
      if (request->hasParam(PARAM_INPUT_3)) {
        inputMessage3 = request->getParam(PARAM_INPUT_3)->value();
      }
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    Serial.println(inputMessage2);
    Serial.println(inputMessage3);
    request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {

 /* WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());*/

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
   digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
   duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
 
  // Convert to inches
  //distanceCm = distanceCm * CM_TO_INCH;
   // Prints the distance in the Serial Monitor
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  //Serial.print("Distance (inch): ");
 // Serial.println(distanceCm);
 
    
    lastdistance = String(distanceCm);
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
  // pieces of information in a channel.  Here, we write to field 1.
  int x = ThingSpeak.writeField(myChannelNumber, 1,distanceCm, myWriteAPIKey);
   if(x == 200){
  Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
  
    
    // Check if distanceCm is above threshold and if it needs to send the Email alert
    /*if(distanceCm > inputMessage3.toFloat() && inputMessage2 == "true" && !emailSent){
      String emailMessage = String("Distance above threshold. Current distance: ") + 
                            String(distanceCm) + String("Inch");
      if(sendEmailNotification(emailMessage)) {
        Serial.println(emailMessage);
        emailSent = true;
      }
      else {
        Serial.println("Email failed to send");
      }    
    }*/
    // Check if distanceCm is below threshold and if it needs to send the Email alert
    /*else*/ if((distanceCm < inputMessage3.toFloat()) && inputMessage2 == "true" && !emailSent) {
      String emailMessage = String("distanceCm below threshold. Current distanceCm: ") + 
                            String(distanceCm) + String("cm");
      if(sendEmailNotification(emailMessage)) {
        Serial.println(emailMessage);
        emailSent = false;
      }
      else {
        Serial.println("Email failed to send");
      }
    }
  }
   delay(20000);
}

bool sendEmailNotification(String emailMessage){
  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);

  // For library version 1.2.0 and later which STARTTLS protocol was supported,the STARTTLS will be 
  // enabled automatically when port 587 was used, or enable it manually using setSTARTTLS function.
  //smtpData.setSTARTTLS(true);

  // Set the sender name and Email
  smtpData.setSender("ESP32", emailSenderAccount);

  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");

  // Set the subject
  smtpData.setSubject(emailSubject);

  // Set the message with HTML format
  smtpData.setMessage(emailMessage, true);

  // Add recipients
  smtpData.addRecipient(inputMessage);

  smtpData.setSendCallback(sendCallback);

  // Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    return false;
  }
  // Clear all data from Email object to free memory
  smtpData.empty();
  return true;
}

// Callback function to get the Email sending status
void sendCallback(SendStatus msg) {
  // Print the current status
  Serial.println(msg.info());

  // Do something when complete
  if (msg.success()) {
    Serial.println("----------------");
  }
}
