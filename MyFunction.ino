//-------------------------------------------------------------------Анализ строки от Nextion-----------------------------------------------------------------------------

void AnalyseString(String incStr) {
  String a_param="";
  String a_value="";

 for (int i = 0; i <= incStr.length(); i++)
 {
  if(!stringReadFlag){
    if(incStr[i]!= 61){
    a_param += incStr[i];
    if(a_param=="switch")switshflag=true;
    }
    else{stringReadFlag=true;}
   }
   else{
    a_value+= incStr[i];
    }
 }

 
 data_param = a_param;
 data_value = a_value; }
//-------------------------------------------------------------Приходящие данные с сервера MQTT---------------------------------------------------------------------------
void getData(char* topic, byte* payload, unsigned int length)
{ 
  payload[length] = '\0';
  String strTopic = String(topic);
  String strPayload = String((char*)payload);
  String data = "";
  bool flag = false;
  for(int i=1; i<strTopic.length(); i++){if(strTopic[i] == 47){flag =true;}
  else{if(flag && strTopic[i] != 47){data += strTopic[i];}}}

if(data.indexOf("switch") != -1){
  Serial.print(data + ".val");
  Serial.print("=");
  Serial.print(""+ strPayload +"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);}
else{
  Serial.print(data);
  Serial.print("=");
  Serial.print("\""+strPayload+"\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);}}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------Функция отправки по MQTT------------------------------------------------------------------------------- 
void sendDataToMqtt(String data,String param,bool flag,const char* id){
  String str = "/"+String(id)+"/"+ data;
  const char * buff = str.c_str ();
  const char * val = param.c_str ();

     if(!flag){
          if (client.connected()) client.subscribe(buff);
          else{if(client.connect(CLIENT_ID))client.subscribe(buff);}}
     else{
          if (client.connected()) {//client.subscribe(buff);
                                   client.publish(buff,val);}
          else{if(client.connect(CLIENT_ID)){//client.subscribe(buff);
                                             client.publish(buff,val);}}}}
//-----------------------------------------------------------------Функция отправки в Nextion----------------------------------------------------------------------------- 
void sendDataToNextion(String data,String val){     
                Serial.print(data + ".txt");
                Serial.print("=");
                Serial.print("\""+ val +"\"");
                Serial.write(0xff);
                Serial.write(0xff);
                Serial.write(0xff);}

void sendDataToNextionVal(String data,String val){     
                Serial.print(data + ".val");
                Serial.print("=");
                Serial.print(""+ val +"");
                Serial.write(0xff);
                Serial.write(0xff);
                Serial.write(0xff);}
//-----------------------------------------------------------------Функция обработки PIR----------------------------------------------------------------------------------
bool sendPIRData(int a){
      int value = digitalRead(a);
    if(value && !count_PIR){
                Serial.print("dim");
                Serial.print("=");
                Serial.print("100");
                Serial.write(0xff);
                Serial.write(0xff);
                Serial.write(0xff);
                
                Serial.print("dim_timer.en");
                Serial.print("=");
                Serial.print("1");
                Serial.write(0xff);
                Serial.write(0xff);
                Serial.write(0xff);
                count_PIR = 1;
    }
    else if(!value){count_PIR = 0;}}
//-----------------------------------------------------------------Функция отправки запросв GET---------------------------------------------------------------------------                 
bool sendDataToGET(String data,String val){
bool result = false;
String ip = "";
String str_pos = "";
int pos = 0;
char pwd[4]={};

for(int i=6;i<data.length();i++){str_pos +=data[i];}
pos = (str_pos.toInt()-1)*20;
ip +=(String)EEPROM.read(101+pos)+"."+(String)EEPROM.read(102+pos)+"."+(String)EEPROM.read(103+pos)+"."+(String)EEPROM.read(104+pos);

pwd[0]=EEPROM.read((int)105+pos);
pwd[1]=EEPROM.read((int)106+pos);
pwd[2]=EEPROM.read((int)107+pos);

  
for(int i=0;i<13;i++){
  if(EEPROM.read((int)108+pos+i) != 0){
          if(val!=""){
              http.begin("http://"+ip+"/"+(String)pwd+"/?cmd="+(String)EEPROM.read(108+pos+i)+":"+val+"");
              int httpCode = http.GET();
              if (httpCode > 0) {
              String payload = http.getString(); 
              if(payload !="Done")sendDataToNextion("log",(String)data+":Naa");
              }
              //else sendDataToNextion("log",(String)data+":Naa");
              http.end();}
           else{
              http.begin("http://"+ip+"/"+(String)pwd+"/?pt="+(String)EEPROM.read(108+pos+i)+"&cmd=get");
              int httpCode = http.GET();
              if (httpCode > 0) {
              String payload = http.getString(); 
              if(payload=="ON")sendDataToNextionVal((String)data,"1");
              else if (payload=="OFF")sendDataToNextionVal((String)data,"0");
              else sendDataToNextionVal((String)data,payload);
              }
             // else sendDataToNextion("log",(String)data+":Naa");
              http.end();}}}
              return result;
              }
//--------------------------------------------------------Настройка WEB---------------------------------------------------------------------------------------------------
//--------------------------------------------------------Root------------------------------------------------------------------------------------------------------------
void webRoot() {
    webPage="";
    webPage += "<!DOCTYPE html>";
    webPage += "<html>\
                              <head>\
                                    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
                                    <title>ESP8266 Settings</title>\
                                        "+style_css+"\
                               </head>";
    webPage += "<body>\
    <div>\
                  <a href=\"setting\">Setting Ethernet</a>\
                  <a href=\"settingmqtt\">Setting MQTT</a>\
                  <a href=\"getsetting\">Setting GET</a>\
                  <a href=\"update\">Firmware Update</a>\
     </div>";               
     webPage +="<p>Temperature: "+ String(htu.readTemperature()-0.5) +"</p>";                                             //-----------------------Температура
     webPage +="<p>Humidity: "+ String(htu.readHumidity()) +"</p>";                                                       //-------------------------Влажность
     webPage +="<div class='bottom'>"+version_prosh+"</div>";
     webPage += "</body>\
                 </html>";
    server.send(200, "text/html; charset=utf-8", webPage);}
//-------------------------------------------------------------------Настройки сети --------------------------------------------------------------------------------------
void ethernetSetting() {
  EEPROM.begin(512);
  String dhcp = "checked";
  if(EEPROM.read(60) != 1){
    dhcp = "";}
    webPage="";
    webPage += "<!DOCTYPE html>";
    webPage += "<html>\
                              <head>\
                                    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
                                    <title>ESP8266 Settings Ethernet</title>\
                                    "+style_css+script+"\
                               </head>\
               <body>\
               <div>\
                       <div><h2>Setting Ethernet</h2><div id=\"grid\"></div></div>\
                      <form method=\"POST\" action=\"saveether\" enctype='multipart/form-data'>\
                           <label for='check'>DHCP server: </label><input type=\"checkbox\" name=\"check\" id='check' " + dhcp + "><br><br>\
                           <label for='ip'>IP addres: </label><input class='input_text' name=\"ip\" id='ip' value= " + String(EEPROM.read(61)) + "." + String(EEPROM.read(62)) +"."+String(EEPROM.read(63))+"."+String(EEPROM.read(64)) + "><p id=\"ip_p\"></p><br><br>\
                           <label for='gateway'>Gateway: </label><input class='input_text' name=\"gateway\" id='gateway' value="+ String(EEPROM.read(65)) + "." + String(EEPROM.read(66)) +"."+String(EEPROM.read(67))+"."+String(EEPROM.read(68)) +"><p id=\"gateway_p\"></p><br><br>\
                           <label for='subnet'>Subnet: </label><input class='input_text' name=\"subnet\" id='subnet' value="+ String(EEPROM.read(69)) + "." + String(EEPROM.read(70)) +"."+String(EEPROM.read(71))+"."+String(EEPROM.read(72)) +"><p id=\"subnet_p\"></p><br><br>\
                           <input class='input_submit' type=SUBMIT value='Save' id=\"send\">\
                      </form>\
                      <a href='/'>Back</a>\
                      </div>\
               </body>\
                 </html>";             
    server.send(200, "text/html; charset=utf-8", webPage);}
//--------------------------------------------------------Настройки MQTT сервера -----------------------------------------------------------------------------------------
void settingMqtt() {
  char* matt_val = readStringEEPROM(77,87);
  EEPROM.begin(512);
  String mqtt = "checked";
  if(EEPROM.read(99) != 1){mqtt = "";}

    webPage="";
    webPage += "<!DOCTYPE html>";
    webPage += "<html>\
                              <head>\
                                    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
                                    <title>ESP8266 Settings MQTT</title>\
                                         "+style_css+script+"\
                               </head>";
                webPage += "<body>\
                <div>\
                      <div><h2>Setting MQTT</h2><div id=\"grid\"></div> </div>\
                      <form method=\"POST\" action=\"savemqtt\" enctype=\"multipart/form-data\">\
                      <label for='check'>Client MQTT </label><input type=\"checkbox\" name=\"check\" id='check' " + mqtt + "><br><br>\
                      <label for='ip_mqtt'>IP addres server MQTT: </label><input class='input_text' name=\"ip_mqtt\" id='ip_mqtt' value= "+ String(EEPROM.read(73)) + "." + String(EEPROM.read(74)) +"."+String(EEPROM.read(75))+"."+String(EEPROM.read(76)) +"><p id=\"ip_mqtt_p\"></p><br><br>\
                      <label for='gateway_mqtt'>Port MQTT: </label><input class='input_text' name=\"gateway_mqtt\" disabled=\"disabled\" id='gateway_mqtt' value="+ String(matt_val)+"><br><br>\
                      <label for='id_mqtt'>Client ID MQTT: </label><input class='input_text' name=\"id_mqtt\" id='id_mqtt' value="+ String(CLIENT_ID)+"><br><br>\
                      <input class='input_submit' type=SUBMIT value='Save' id=\"send\">\
                </form>\
                <a href='/'>Back</a>\
                </div>\
                </body>\
                </html>";
    server.send(200, "text/html; charset=utf-8", webPage);}
//--------------------------------------------------------Настройки GET запросов -----------------------------------------------------------------------------------------
void getSetting() {
  String scriptGet ="";
  String str_port ="";
  int count = (int)count_GET;
  int countStart = 0;
  
  scriptGet += "<script type=\"text/javascript\">\
                   window.addEventListener('load', function () {\
                   var count = "+(String)count_GET+";";
                    
                    if(0<count&&count<21){
                        for (int i = 1; i < count+1; i++) {
                           char pwd[4]={};
                              pwd[0]=EEPROM.read((int)105+countStart);
                              pwd[1]=EEPROM.read((int)106+countStart);
                              pwd[2]=EEPROM.read((int)107+countStart);
                          
                        for(int z=0;z<13;z++){
                          if(EEPROM.read((int)108+countStart+z)==0||EEPROM.read((int)108+countStart+z)==255){continue;}
                          str_port += (String)EEPROM.read((int)108+countStart+z);
                          str_port +=";";}
                          
  scriptGet +=     "var input = document.getElementById('send');\
                    var input_count = document.getElementById('count');\
                    input_count.value ="+ (String)count+";";
  scriptGet +=     "var switchinner = \"<h4>Switch"+(String)i+"</h4><label for='get_ip"+(String)i+"'>IP</label><input maxlength='15' class = 'get_ip input_text input_ip' name='get_ip"+(String)i+"'  id='get_ip"+(String)i+"' value= "+ String(EEPROM.read(101+countStart)) + "." + String(EEPROM.read(102+countStart)) +"."+String(EEPROM.read(103+countStart))+"."+String(EEPROM.read(104+countStart)) +"><br><br><label for='get_pwd"+(String)i+"'>Pass(pwd)</label><input maxlength='3' class = 'get_pwd input_text input_pwd' name='get_pwd"+(String)i+"' id='get_pwd"+(String)i+"' value="+(String)pwd+"><br><br><label for='get_p"+(String)i+"'>Port(p)</label><input class = 'get_p input_text' name='get_p"+(String)i+"' id='get_p"+(String)i+"' value="+str_port+"><br><br><div id='grid'></div>\";\
                    var div_add = document.createElement('DIV');\
                    div_add.setAttribute('id','switch"+(String)i+"');\
                    div_add.innerHTML = switchinner;\
                    document.getElementById('boxineer').parentNode.insertBefore(div_add, input);";
                    str_port="";        
                    countStart+=20;}}                   
 scriptGet +=          "if (document.getElementById('add_get')) {\
                document.getElementById('add_get').addEventListener('click', function () {\
                    count += 1;\
                    if(count<=20){\
                    var input = document.getElementById(\"send\");\
                    var input_count = document.getElementById('count');\
                    input_count.value = count;\
                    var switchinner = \"<h4>Switch\" + count + \"</h4>\
                    <label for='get_ip\" + count + \"'>IP</label><input maxlength='15' class = 'get_ip input_text input_ip' name='get_ip\" + count + \"'  id='get_ip\" + count + \"'><br><br><label for='get_pwd\" + count + \"'>Pass(pwd)</label><input maxlength='3' class = 'get_pwd input_text input_pwd' name='get_pwd\" + count + \"' id='get_pwd\" + count + \"'><br><br><label for='get_p\" + count + \"'>Port(p)</label><input class = 'get_p input_text' name='get_p\" + count + \"' id='get_p\" + count + \"'><br><br><div id='grid'></div>\";\
                    var div_add = document.createElement('DIV');\
                    div_add.setAttribute(\"id\", \"switch\" + count);\
                    div_add.innerHTML = switchinner;\
                    document.getElementById('boxineer').parentNode.insertBefore(div_add, input);}});}});\
                    </script>";
                    
    webPage="";
    webPage += "<!DOCTYPE html>";
    webPage += "<html>\
                              <head>\
                                    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
                                    <title>GET Settings</title>\
                                         "+style_css+script+scriptGet+"\
                               </head>";
                webPage += "<body>\
     <div><div><h2>Setting GET for megad</h2><div id=\"grid\"></div></div><div><div>\
     <form method=\"POST\" action=\"saveget\" enctype='multipart/form-data'><div id=\"boxineer\"></div>\
    <input class='input_submit' type=SUBMIT value='Save' id=\"send\"><input name=\"count\" id=\"count\" hidden/></form></div><a id='add_get'>Add Switch</a><a href='/'>Back</a>\
    </div></div></body>\
                </html>";
    server.send(200, "text/html; charset=utf-8", webPage);}
//----------------------------------------------------------------Сохранение настроек GET---------------------------------------------------------------------------------
void saveGet(){


  
  webPage="";
  webPage += "<html>\
 <head>\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
   <title>ESP8266 Saved param GET</title>\
   "+style_css+"\
 </head>";
  webPage += "<body>\
   <a href='/'>Back</a>\
   <p>Setting 'Get' saved.</p>";
   
   
      int count = server.arg("count").toInt();
      int count_write = 0;
      int count_port = 0;
      bool flag = false;
      String str = "";
      for(int i=1;i<count+1;i++){
      if(server.arg("get_ip"+(String)i) == "" || server.arg("get_pwd"+(String)i) == "" || server.arg("get_p"+(String)i) == ""){break;}
  
        String ip = returnStringFromEEPROM(101+count_write,104+count_write,".");
        String pwd = returnStringFromEEPROM(105+count_write,3,"");
        String port = returnStringFromEEPROM(108+count_write,13,";");

          flag = writeIpInEEPROM(("get_ip"+(String)i),((int)101+count_write));                        //----Записываем IP GET
          
          if(pwd != server.arg("get_pwd"+(String)i)||server.arg("get_pwd"+(String)i)==""){            //---Записываем PWD GET
              EEPROM.begin(512);
              EEPROM.write((int)105+count_write,(byte)server.arg("get_pwd"+(String)i)[0]);
              EEPROM.write((int)106+count_write,(byte)server.arg("get_pwd"+(String)i)[1]);
              EEPROM.write((int)107+count_write,(byte)server.arg("get_pwd"+(String)i)[2]);
              EEPROM.commit();
              flag=true;}
          
       if(port != server.arg("get_p"+(String)i)||server.arg("get_p"+(String)i)==""){                  //-Записываем PORTS GET
        EEPROM.begin(512);
        for(int j= 0;j<server.arg("get_p"+(String)i).length();j++){
        if(server.arg("get_p"+(String)i)[j]!=';'){str += server.arg("get_p"+(String)i)[j];}
        else{
            EEPROM.write((int)108+count_write+count_port,str.toInt());
            count_port++;
            str="";}
          }
          for(;count_port<20;count_port++){
            if(EEPROM.read(108+count_write+count_port)!=0){
            EEPROM.write((int)108+count_write+count_port,0);}}
            EEPROM.commit();}
         count_port=0;   
         count_write +=20;}

    if((String)count_GET!= server.arg("count")){
         EEPROM.begin(512);
         EEPROM.write(100,(byte)count);
         EEPROM.commit();}
     webPage += "</body></html>";
  server.send ( 200, "text/html", webPage );   
  if(flag)ESP.restart();}
//----------------------------------------------------------------Сохранение настроек MQTT--------------------------------------------------------------------------------
void saveMqtt(){
byte leng = EEPROM.read(99);
bool flag = false;
String gateway_mqtt_data = readStringEEPROM(77,10);
String id_mqtt_data = readStringEEPROM(88,10);
  webPage="";
  webPage += "<html>\
 <head>\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
   <title>ESP8266 Settings mqtt</title>\
   "+style_css+"\
 </head>";
  webPage += "<body>\
   <p>Setting 'MQTT' saved.</p>\
   <a href='/'>Back</a>";
      //-----------------------------------------------Проверяем настройки mqtt-----------------------------------------------------------------------------
    if(server.hasArg("check")){
        if(leng != 1){
         EEPROM.begin(512);
         EEPROM.write(99,1);
         EEPROM.commit();
         EEPROM.end();
         flag = true;}}
    else{
       if(leng != 0){
        EEPROM.begin(512);
        EEPROM.write(99,0);
        EEPROM.commit();
        EEPROM.end();
        flag = true;}}

    flag = writeIpInEEPROM("ip_mqtt",73);

    if(server.arg("gateway_mqtt") != gateway_mqtt_data){
    writeStringEEPROM(77,server.arg("gateway_mqtt"),87);
    flag = true;}
    
    if(server.arg("id_mqtt") != id_mqtt_data){
    writeStringEEPROM(88,server.arg("id_mqtt"),98);
    flag = true;}
    
     webPage += "</body></html>";
  server.send ( 200, "text/html", webPage );
  if(flag)ESP.restart();}                                                                                         //При изменении любого параметра restart
//------------------------------------------------------------------Сохранение настроек Ethernet--------------------------------------------------------------------------
void saveEther(){
  byte leng = EEPROM.read(60);                                                                                        //------------Читаем включен ли DHCP
  bool flag = false;

  webPage = "";
  webPage += "<html>\
 <head>\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
   <title>ESP8266 Settings ethernet</title>\
    "+style_css+"\
 </head>";
  webPage += "<body>\
   <p>Setting 'Ethernet' saved.</p>\
   <a href='/'>Back</a>";
    //-----------------------------------------------Проверяем настройки DHCP-----------------------------------------------------------------------------
    if(server.hasArg("check")){
        if(leng != 1){
         EEPROM.begin(512);
         EEPROM.write(60,1);
         EEPROM.commit();
         EEPROM.end();
         flag = true;}}
    else{
       if(leng != 2){
        EEPROM.begin(512);
        EEPROM.write(60,2);
        EEPROM.commit();
        EEPROM.end();
        flag = true;}}
     //----------------------------------------------Проверяем настройки ip-------------------------------------------------------------------------------
     flag = writeIpInEEPROM("ip",61);
     //----------------------------------------------Проверяем настройки gateway--------------------------------------------------------------------------
     flag = writeIpInEEPROM("gateway",65);
     //----------------------------------------------Проверяем настройки subnet---------------------------------------------------------------------------
     flag = writeIpInEEPROM("subnet",69);

  webPage += "</body></html>";
  server.send ( 200, "text/html", webPage );
  if(flag)ESP.restart();}                                                                                          //При изменении любого параметра restart
//----------------------------------------------------------------Настройки WIFI для режима SoftAP------------------------------------------------------------------------
void handleRoot() {
  webPage ="";
  webPage += "<html>\
 <head>\
  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
   <title>ESP8266 Settings</title>\
   "+style_css+"\
 </head>";
  webPage += "<body>\
  <div>\
      <div><h2>Setting SoftAP</h2><div></div></div>\
   <form method=\"POST\" action=\"ok\">\
     <label for='ssid'>WIFI ssid: </label><input name=\"ssid\" id='ssid'><br><br>\
     <label for='pswd'>WIFI pass: </label><input name=\"pswd\" id='pswd'></br></br>\
     <label for='check'>DHCP server: </label><input type=\"checkbox\" name=\"check\" id='check'><br><br>\
     <label for='ip'>IP addres: </label><input name=\"ip\" id='ip' value= \"0.0.0.0\"><br><br>\
     <label for='gateway'>Gateway: </label><input name=\"gateway\" id='gateway' value=\"0.0.0.0\"><br><br>\
     <label for='subnet'>Subnet: </label><input name=\"subnet\" id='subnet' value=\"0.0.0.0\"><br><br>\
     <input type=SUBMIT value=\"Save settings\">\
   </form>\
   <a href='/clearflash'>Clear Flash GET</a>\
   </div>\
 </body>\
</html>";
server.send ( 200, "text/html", webPage );}
//-----------------------------------------------------------Очистка EEPROM под GET---------------------------------------------------------------------------------------
void clearFlash() {
  webPage ="";
  webPage += "<html>\
 <head>\
  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
   <title>ESP8266 Settings</title>\
   "+style_css+"\
 </head>";
  webPage += "<body>";
  EEPROM.begin(512);    
  EEPROM.write(100,0);
  EEPROM.commit();
  EEPROM.end();
  webPage += "Flash was cleared!</br>\
  <a href='/'>Back</a>\
 </body>\
</html>";
server.send ( 200, "text/html", webPage );
ESP.restart();}
//-----------------------------------------------------------Сохранение настроек WIFI для режима SoftAP------------------------------------------------------------------- 
void handleOk(){
  byte leng = EEPROM.read(60);                                                                                             //------------Читаем включен ли DHCP
  webPage ="";
  webPage += "<html>\
 <head>\
  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
   <title>ESP8266 Settings</title>\
   "+style_css+"\
 </head>";
  webPage += "<body>";

  if(server.arg("ssid") != ""){
   writeStringEEPROM(0,server.arg("ssid"),29);                                                                             //-------------Запись имя точки WIFI
   writeStringEEPROM(30,server.arg("pswd"),59);                                                                            //---------------------Запись пароля
   writeStringEEPROM(77,"1883",87);
   writeStringEEPROM(88,"none",98);
   if(server.hasArg("check")){
         EEPROM.begin(512);
         EEPROM.write(60,1);
         EEPROM.commit();
         EEPROM.end();}
    else{if(leng != 2){
        EEPROM.begin(512);
        EEPROM.write(60,2);
        EEPROM.commit();
        EEPROM.end();}}
     //----------------------------------------------Проверяем настройки ip-------------------------------------------------------------------------------
     writeIpInEEPROM("ip",61);
     //----------------------------------------------Проверяем настройки gateway--------------------------------------------------------------------------
     writeIpInEEPROM("gateway",65);
     //----------------------------------------------Проверяем настройки subnet---------------------------------------------------------------------------
     writeIpInEEPROM("subnet",69);
    if(!EEPROM.read(99)){
        EEPROM.begin(512);
        EEPROM.write(99,0);
        EEPROM.commit();
        EEPROM.end();}

    webPage +="Configuration saved in FLASH</br>\
   Changes applied after reboot</p></br></br>\
   <a href=\"/\">Return</a> to settings page</br>";}
  else {
    webPage += "No WIFI Net</br>\
   <a href=\"/\">Return</a> to settings page</br>";}
  webPage += "</body></html>";
  server.send ( 200, "text/html", webPage );
  ESP.restart();}
//---------------------------------------------------------------Функции Записи\Чтения EEPROM ----------------------------------------------------------------------------
void writeStringEEPROM (int Addr, String Str, int Size) {
    if(Str != ""){
    byte leng=Str.length();
    EEPROM.begin (Size);
    EEPROM.write(Addr , leng);
    unsigned char* buf = new unsigned char[Size];
    Str.getBytes(buf, leng + 1);
    Addr++;
    for(byte i = 0; i < leng; i++) {EEPROM.write(Addr+i, buf[i]); delay(10);}
    EEPROM.end();}}
    
char *readStringEEPROM (int Addr,int Size) {
    byte leng = EEPROM.read(Addr);
    char* buf = new char[Size];
    Addr++;
    for(byte i = 0; i < leng; i++) buf[i] = char(EEPROM.read(i+Addr));
    buf[leng] = '\x0';
    return buf;}
//---------------------------------------------------------------Функции Записи ip в EEPROM ------------------------------------------------------------------------------
bool writeIpInEEPROM (String arg_Name, int StartAddrEeprom){
     if(server.hasArg(arg_Name)){
      String data = String(EEPROM.read(StartAddrEeprom)) + "." + String(EEPROM.read(StartAddrEeprom + 1)) +"."+String(EEPROM.read(StartAddrEeprom + 2))+"."+String(EEPROM.read(StartAddrEeprom + 3));

      if(server.arg(arg_Name) != data){
      data = "";
      int count=0;
      String val = "";
      data = server.arg(arg_Name);
          for(int i =0;i<=data.length(); i++){
            if(data[i] != '.'){val += data[i];}
             else{
         EEPROM.begin(512);
         EEPROM.write(StartAddrEeprom+count,byte(val.toInt()));
         EEPROM.commit();
         EEPROM.end();
                count++;
                val ="";}}
         EEPROM.begin(512);
         EEPROM.write(StartAddrEeprom +3,byte(val.toInt()));
         EEPROM.commit();
         EEPROM.end();
         return true;}
         else{return false;}}}
//---------------------------------------------------------------Функции чтения данных для GET ---------------------------------------------------------------------------
String returnStringFromEEPROM(int Start, int End, String Divider){
String endRes ="";
for(;Start<End+1;Start++){
endRes+=(String)EEPROM.read(Start);
if(Start<End)endRes+=Divider;
}
return endRes;}
