#include <Nextion.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "SparkFunHTU21D.h"

String version_prosh ="ver_0.33b";                                                                                //-------------Версия прошивки
//----------------------------------------------------------Сетевые настройки WIFI----------------------------------------------------------------------------------------
byte dhcp = 0;
byte ip[4] = {192,168,1,2};
byte gateway[4] = {192,168,1,1};
byte subnet[4] = {255,255,255,0};
//---------------------------------------------------------переменные для MQTT--------------------------------------------------------------------------------------------
const char* CLIENT_ID = "";
IPAddress ipmqtt(192,168,1,1);
byte mqtt_check = 0;
//---------------------------------------------------------переменные для GET---------------------------------------------------------------------------------------------
byte count_GET =0;
//-----------------------------------------------------------переменные для Nextion---------------------------------------------------------------------------------------
String incStr = "";                                                                                                 //----------Это будет приемник информации от Nextion
bool serialReadFlag = false;                                                                                        //-----Флаг проверки сообщения пришедшего от Nextion
String data_param="";
String data_value="";
bool switshflag =false;
bool stringReadFlag=false;
//-----------------------------------------------------------переменные для PIR-------------------------------------------------------------------------------------------
int count_PIR = 0;
//--------------------------------------------------------------настройки WEB Server--------------------------------------------------------------------------------------
String webPage = "";
String str = "";
boolean conf = false;
String iphost = "";
int count_WIFI = 0;

const char * param_str ="";                                                                                         //----------------------Получаем параметр из Nextion
const char * value_param_str ="";                                                                                   //----------------------Получаем значение из Nextion
//-----------------------------------------------------------Настройка стилей CSS-----------------------------------------------------------------------------------------
String style_css ="<style>\
        body {height:auto;background-color: #FAF9F9;font-family: Arial, Helvetica, Sans-Serif;Color: #000088;}\
        a, #send {text-align:center;margin-bottom: 10px;margin-top: 10px;font-family: arial,sans-serif;color: rgb(68,68,68);text-decoration: none;user-select: none;padding: .2em 1.2em;outline: none;border: 1px solid rgba(0,0,0,.1);background: rgb(245,245,245) linear-gradient(#f4f4f4, #f1f1f1);transition: all .218s ease 0s;}\
        a:hover, #send {color: rgb(24,24,24);border: 1px solid rgb(198,198,198);background: #f7f7f7 linear-gradient(#f7f7f7, #f1f1f1);box-shadow: 0 1px 2px rgba(0,0,0,.1);}\
        a:active {color: rgb(51,51,51);border: 1px solid rgb(204,204,204);background: rgb(238,238,238) linear-gradient(rgb(238,238,238), rgb(224,224,224));box-shadow: 0 1px 2px rgba(0,0,0,.1) inset;}\
</style>";
//-----------------------------------------------------------Скрипт javascript--------------------------------------------------------------------------------------------
String script = "<script type=\"text/javascript\">\
        window.addEventListener('load', function () {\
            var ip = document.getElementById('ip');\
            var gateway = document.getElementById('gateway');\
            var subnet = document.getElementById('subnet');\
            var ip_mqtt = document.getElementById('ip_mqtt');\
            function checkChecked() {\
                if (document.getElementById('check')) {\
                    if (document.getElementById('check').checked) {\
                        if (ip || gateway || subnet) {\
                            ip.disabled = 'disabled';\
                            gateway.disabled = 'disabled';\
                            subnet.disabled = 'disabled';};}\
                    else {if (ip || gateway || subnet) {\
                            ip.disabled = '';\
                            gateway.disabled = '';\
                            subnet.disabled = '';}}}}\
            checkChecked();\
            if (document.getElementById('check')) { document.getElementById('check').addEventListener('click', function () { checkChecked(); }) }\
            function setParam(element, p_element, e) {\
                var val = /\\b(([01]?\\d?\\d|2[0-4]\\d|25[0-5])\\.){3}([01]?\\d?\\d|2[0-4]\\d|25[0-5])\\b/;\
                if (element.value.search(val) == -1) {p_element.innerHTML = 'Неверно введены данные';\
                    e.preventDefault();}\
                else { p_element.innerHTML = ''; }}\
            if (document.getElementById('send')) {\
                document.getElementById('send').addEventListener('click', function (e) {\
                    if (ip) { setParam(ip, document.getElementById('ip_p'), e); }\
                    if (gateway) { setParam(gateway, document.getElementById('gateway_p'), e); }\
                    if (subnet) { setParam(subnet, document.getElementById('subnet_p'), e); }\
                    if (ip_mqtt) { setParam(ip_mqtt, document.getElementById('ip_mqtt_p'), e); }});}});\
</script>";

HTU21D htu;
WiFiClient espClient;
PubSubClient client(espClient);
MDNSResponder mdns;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
HTTPClient http;
//------------------------------------------------------------------ОСНОВНАЯ ФУНКЦИЯ--------------------------------------------------------------------------------------
void loop() {
  server.handleClient();
//-------------------------------------------------Перевод модуля в режим конфигурации путем замыкания GPIO0 на массу-----------------------------------------------------
  if((digitalRead(14) == LOW) && !conf){ //d5
    EEPROM.write(0,255);
    EEPROM.commit();
    EEPROM.end();
    conf = true;
    Serial.println("Please reboot module for coniguration");
    ESP.restart();
  }
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------ 

if(iphost != "ESPap"){
  client.loop();

  if(Serial.available()) {
    delay(50);
      uint8_t inn = Serial.read();
      if(serialReadFlag) {
        if(inn == 59){
          if(incStr.length() > 0) {AnalyseString(incStr);}
          serialReadFlag = false;}
        else {incStr += (char)inn;}}
        else {if(inn == 35){serialReadFlag = true;
        incStr = "";}}}
        
if(data_param !=""){
 
          if(switshflag){
            if(mqtt_check && client.connected())sendDataToMqtt(data_param,data_value,stringReadFlag,CLIENT_ID);                                     //Проверяем включен ли MQTT и отправляем на сервер MQTT
            else {if(0<count_GET&&count_GET<21){sendDataToGET(data_param,data_value);}}}                                             //--------------------------------Отправляем запрос GET
          
          else if(data_param == "temp"){
            String val = String(htu.readTemperature()-0.5)+"C";
            stringReadFlag=true;

            if(mqtt_check)sendDataToMqtt(data_param,val,stringReadFlag,CLIENT_ID);                                         //Проверяем включен ли MQTT и отправдяем на сервер MQTT
            sendDataToNextion(data_param,val);                                                                                                  //-----------------Отправляем данные на монитор Nextion
          }
          else if(data_param == "hum"){
            String val = String (htu.readHumidity())+"%";
            stringReadFlag=true;

            if(mqtt_check)sendDataToMqtt(data_param,val,stringReadFlag,CLIENT_ID);                                         //Проверяем включен ли MQTT и отправдяем на сервер MQTT
            sendDataToNextion(data_param,val);                                                                                                  //-----------------Отправляем данные на монитор Nextion
          }
          else{if(mqtt_check)sendDataToMqtt(data_param,data_value,stringReadFlag,CLIENT_ID); }                                //Проверяем включен ли MQTT и отправдяем на сервер MQTT
}

  sendPIRData(4);
}}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
