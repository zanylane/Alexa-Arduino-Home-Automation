/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws_iot_mqtt.h>
#include <aws_iot_version.h>
#include "aws_iot_config.h"
#include <string.h>

aws_iot_mqtt_client myClient;
char JSON_buf[100];
int cnt = 0;
int rc = 1;
bool success_connect = false;
/*  In order to support a nice siren sound, we need to keep track of whether it
 *  is 'running,' even if right now it's in the quiet lull between two loud times.
 *  So there need to be a couple of state variables to track that.
 */
bool runSiren = false;  //desired state
bool sirenRunningNow = false; //if it actually has power
unsigned long nextTimeSiren = 0;
unsigned long nextTimeShadow = 0;

// Siren
uint8_t sirenPin = 4; // doesn't really matter which one, but for future upgrades,
                      // I wan't the PWM available to drive the motor through a MOSFET.


bool print_log(const char* src, int code) {
  bool ret;
  if(code == 0) {
    #ifdef AWS_IOT_DEBUG
    Serial.print(F("[LOG] command: "));
    Serial.print(src);
    Serial.println(F(" completed."));
    #endif
    ret = true;
  }
  else {
    #ifdef AWS_IOT_DEBUG
    Serial.print(F("[ERR] command: "));
    Serial.print(src);
    Serial.print(F(" code: "));
    Serial.println(code);
    #endif
    ret = false;
  }
  Serial.flush();
  return ret;
}

void handle_siren()
{
  if(runSiren == false){
    //Serial.println("turn off");
    digitalWrite(sirenPin, HIGH);
    nextTimeSiren = 0;
    sirenRunningNow = false;
  }
  else {
    if(sirenRunningNow == true){
      sirenRunningNow = false;
      // This next line is just for in case. The mqtt client is blocking and takes long enough
      // that the minimum time between calls is far more than this.
      nextTimeSiren = millis() + 200;
      //Serial.println("turn off");
      digitalWrite(sirenPin, HIGH);
    } else {
      sirenRunningNow = true;
      nextTimeSiren = millis() + 1000;
      //Serial.println("turn on");
      digitalWrite(sirenPin, LOW);
    }
    
    
  }

}

void msg_callback_delta(char* src, unsigned int len, Message_status_t flag) {
  if(flag == STATUS_NORMAL) {
    // Get delta for light key
    print_log("getDeltaKeyValue", myClient.getDeltaValueByKey(src, "power", JSON_buf, 100));
    //Serial.println(JSON_buf);
    if (strcmp(JSON_buf, "ON") == 0){
      runSiren = true;
    }
    else{
      runSiren = false; 
    }
    String payload = "{\"state\":{\"reported\":";
    payload += "{\"power\":\"";
    payload += JSON_buf;
    payload += "\"}}}";
    payload.toCharArray(JSON_buf, 100);
    //Serial.println(JSON_buf);
    print_log("update thing shadow", myClient.shadow_update(AWS_IOT_MY_THING_NAME, JSON_buf, strlen(JSON_buf), NULL, 5));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(sirenPin, HIGH);
  pinMode(sirenPin, OUTPUT);

  
  //Disable this line if connected to wall plug!
  //while(!Serial);

  char curr_version[80];
  snprintf_P(curr_version, 80, PSTR("AWS IoT SDK Version(dev) %d.%d.%d-%s\n"), VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
  Serial.println(curr_version);

  if(print_log("setup...", myClient.setup(AWS_IOT_CLIENT_ID))) {
    if(print_log("config", myClient.config(AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, AWS_IOT_ROOT_CA_PATH, AWS_IOT_PRIVATE_KEY_PATH, AWS_IOT_CERTIFICATE_PATH))) {
      if(print_log("connect", myClient.connect())) {
        success_connect = true;
        
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        
        print_log("shadow init", myClient.shadow_init(AWS_IOT_MY_THING_NAME));
        print_log("register thing shadow delta function", myClient.shadow_register_delta_func(AWS_IOT_MY_THING_NAME, msg_callback_delta));
      }
    }
  }
}

void loop() {
  unsigned long timeNow = millis();
//  Serial.println(timeNow);
  // Amazon limits us to only checking once per second, and to be as responsive as possible when given a command
  // by a user, don't want to go any past that. So check them every second.
  if(timeNow >= nextTimeShadow){
    Serial.println("Checking Shadow");
    if(success_connect) {
      if(myClient.yield()) {
        Serial.println(F("Yield failed."));
      }
      //delay(1000); // check for incoming delta per 1000 ms
    }
    nextTimeShadow = timeNow + 1000; // Bump it up another second
  }
  if(timeNow >= nextTimeSiren){
    if(runSiren == true){ 
      handle_siren();
    }
  }
  if((sirenRunningNow == true) && (!runSiren)){
      handle_siren();
  }
}
