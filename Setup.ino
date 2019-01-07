//--------------------------------------------------------------------SETUP-----------------------------------------------------------------------------------------------
void setup(void) {

  delay(1000);
  Serial.begin(115200);
  pinMode(14, INPUT_PULLUP);  //d5 reset
  pinMode(4, INPUT_PULLUP);   // PIR sensor
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------  
// --------------------------------------------------------------Режим STATION--------------------------------------------------------------------------------------------
  //------------------------------------------------------------------------------------------------------------Проверка на сброс в режим AP(GPIO14 на GND)
  if((digitalRead(14) == LOW) && !conf){ //d5
    EEPROM.begin(2);
    EEPROM.write(0,255);
    EEPROM.commit();
    EEPROM.end();
    conf = true;
    Serial.println("Please reboot module for coniguration");
    ESP.restart();
   }
  //----------------------------------------------------------------------------------------------------------------------------------------Если нет сброса
  else{
    EEPROM.begin(512);
    byte leng = EEPROM.read(0);
      if(leng != 255){
       const char* ssid = readStringEEPROM(0,29);                                                             //------------------Чтение имя WIFI из EEPROM
       const char* pass = readStringEEPROM(30,29);                                                            //---------------Чтение пароля WIFI из EEPROM
       byte dhcp = EEPROM.read(60);                                                                           //------------Чтение настройки DHCP из EEPROM

        //-----------------------------------------------------------------------------------------------------------------------Чтение ip адреса из EEPROM
       ip[0] = EEPROM.read(61);
       ip[1] = EEPROM.read(62);
       ip[2] = EEPROM.read(63);
       ip[3] = EEPROM.read(64);
        //-------------------------------------------------------------------------------------------------------------------------Чтение GATEWAY из EEPROM
       gateway[0] = EEPROM.read(65);
       gateway[1] = EEPROM.read(66);
       gateway[2] = EEPROM.read(67);
       gateway[3] = EEPROM.read(68);
        //-----------------------------------------------------------------------------------------------------------------------------Чтение чекбокса mqtt
       mqtt_check = EEPROM.read(99);
        //---------------------------------------------------------------------------------------------------------------------------Чтение маски из EEPROM
       subnet[0] = EEPROM.read(69);
       subnet[1] = EEPROM.read(70);
       subnet[2] = EEPROM.read(71);
       subnet[3] = EEPROM.read(72);
        //------------------------------------------------------------------------------------------------------------------Чтение ip адреса MQTT из EEPROM
       ipmqtt[0]=EEPROM.read(73);
       ipmqtt[1]=EEPROM.read(74);
       ipmqtt[2]=EEPROM.read(75);
       ipmqtt[3]=EEPROM.read(76);
        //-------------------------------------------------------------------------------------------------------------------------Чтение ID MQTT из EEPROM
       CLIENT_ID = readStringEEPROM(88,10);
        //------------------------------------------------------------------------------------------------------------------------------Чтение счетчика GET
       count_GET = EEPROM.read(100);

  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("Connecting to ");
  Serial.println(ssid);
  Serial.println(pass);
  WiFi.begin(ssid, pass);

  if(dhcp==2){
    WiFi.config(ip, gateway, subnet);
  }

  while (WiFi.status() != WL_CONNECTED) {
    if(digitalRead(14) != LOW){  //d5
   delay(1000);
   count_WIFI++;
   if(count_WIFI>=60){ESP.restart();}
   else{Serial.print(".");}
    }
    else{
    EEPROM.begin(2);
    EEPROM.write(0,255);
    EEPROM.commit();
    EEPROM.end();
    conf = true;
    Serial.println("Reboot");
    ESP.restart();
      }
  }

  Serial.println("");
  
  Serial.println(count_GET);
  Serial.println(dhcp);
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");                                                                                 //----"Запущен MDNSresponder"
  }

// --------------------------------------------------------------Настройка WEB--------------------------------------------------------------------------------------------
  server.on("/", webRoot);                                                                                                    //--------Корневой катало WEB
  server.on("/setting", ethernetSetting);                                                                                     //---------Настройки сети WEB
  server.on("/settingmqtt", settingMqtt);                                                                                     //-----Настройки клиента MQTT
  server.on("/getsetting", getSetting);                                                                                       //-----Настройка запросов GET
  server.on("/savemqtt", saveMqtt);                                                                                           //---Сохранение настроек MQTT
  server.on("/saveether", saveEther);                                                                                         //---Сохранение настроек сети
  server.on("/saveget", saveGet);                                                                                             //----Сохранение настроек GET
// ---------------------------------- Настройки возврата MQTT-------------------------------------------------------------------------------------------------------------
if (mqtt_check){  
  client.setServer(ipmqtt,String(readStringEEPROM(77,87)).toInt());
  Serial.println("Connecting to MQTT server");
  client.connect(CLIENT_ID);
  Serial.println("connect mqtt...");
  client.connected();
  delay(10);
  Serial.println("IP сервера MQTT: " + String(EEPROM.read(73))+"."+String(EEPROM.read(74))+"."+String(EEPROM.read(75))+"."+String(EEPROM.read(76)));
  Serial.println("Port сервера MQTT: " + String(readStringEEPROM(77,87)));
  client.setCallback(getData);
}
//--------------------------------------HTTP server подключение-----------------------------------------------------------------------------------------------------------
  httpUpdater.setup(&server);
  server.begin();
  Serial.println("HTTP server started");
  MDNS.addService("http", "tcp", 80);
//-------------------------------------------------Проверка датчика температуры-------------------------------------------------------------------------------------------
    Wire.begin(0,2);
    htu.begin();
  }
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------Режим SoftAP--------------------------------------------------------------------------------------------------
else 
      {
        const char *ssid_ap = "ESPap";
        iphost = "ESPap";
        WiFi.mode(WIFI_AP);
       
        Serial.print("Configuring access point...");
        
        WiFi.softAP(ssid_ap);
 
        delay(2000);
        Serial.println("done");
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
        server.on("/", handleRoot);
        server.on("/ok", handleOk);
        server.on("/clearflash", clearFlash);                                                                                       //-----Очистка EEPROM под GET
        server.begin();
        Serial.println("HTTP server started");  
     }
  }
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
