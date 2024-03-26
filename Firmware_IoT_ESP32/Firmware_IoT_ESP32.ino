
/*Development of functional code executed on an ESP32
   Code design and development by:
   Jesús Javier Velázquez Carrillo*/


/* Summary explanation: This is the design code of a firmware created
     for a project focused on IoT. Where data is obtained according to what has been read
     and previously converted with the ESP32; this data has to be sent
     certain time interval, which is configured by the user from
     of a GUI, with also updated dates. When this is achieved you have to
     configure the user a connection to a Wi-Fi network to accomplish this.

     At the end of the process of sending the read data to a dashboard, the processor
     You have to go into sleep mode, whether light or deep.

     The processor has to wake up from deep sleep and take the readings and
     sending information to the dashboard.

     To avoid or solve ESP32 wifi connection problems to be able to send
     The data was developed, a test of constant connection to Wi-Fi, having a limit
     of self-connection attempts, if this is not achieved the ESP32 is factory reset
     Asking the user again to configure dates, wifi and how often it will be sent
     the information to the dashboard */


//Librerias//
#include <WiFi.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_log.h"
#include <rom/rtc.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <RTClib.h>



esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();


String ssid2;
String password2;

/*************************/
int countconection  = 0;
int i = 0;

/////////// WifiManager ////////
////Variable//////
char hour[3] = "10";
char minute[3]= "10";
char second[3]= "10";
/// Variable Fechas ///
char dia[3] = "04";
char mes[3]=  "04";
char anio[5]= "2023";
/// Para Confg. Actualizacion ///
char Valor_minute[3]="1";
char Valor_hra[3]="1";

/// Variables de los punteros ///
int Valor_hour1;
int Valor_hour2;
int Valor_minute1;
int Valor_minute2;
int Valor_second1;

/// Punteros para texbox Config. RTC///
int *Ptr2=&Valor_minute1;
int *Ptr1=&Valor_hour1;
int *Ptr3=&Valor_second1;

// Punteros para los texbox usados en la actualizacion URL///
int *Ptr4=&Valor_minute2;
int *Ptr5=&Valor_hour2;

/// Variables para verficar Wifi ///
int wifiCount = 0;
int NoConnection = 0;
int wifidesconection = 0;
const int AUTOCONNECT_INTERVAL = 60000; // intervalo de autoconexión
 int storedValue;

//int status, *Ptr6=&status;
/// Objetos e instancias creadas ////
WiFiManager wifiman;
Preferences prefs;

Preferences preferences;
Preferences preferences2;

RTC_DS1307 myRTC;
DateTime now;
DateTime lastCheckedTime; //Variable para almacenar la última vez que se comprobó la hora del RTC

/// RECURSO PARA EVITAR DESCONEXION DEL WIFI
int ciclos = 0;
int *Ptr6 = &ciclos;

// Banderas para la ejecucion de tarea
int miBooleano = 0;
int *apuntadorABooleano = &miBooleano;

/**********************************/

int Time1,Time2,Time3,Time4,Time5,Time6;

int *PtrTime1 = &Time1;
int *PtrTime2 = &Time2;
int *PtrTime3 = &Time3;
int *PtrTime4 = &Time4;
int *PtrTime5 = &Time5;
int *PtrTime6 = &Time6;


/////////// Desarrollo Volumen-voltage ////////

float volumenanalogico = 0;
float voltajeanalogico = 0;
int vin = 0;
int vol = 0;
bool status = 0;

// Al inicio del código
unsigned long previousMillis = 0; 
const long interval = 2000;  // 2 segundos

// En lugar del delay(2000)
unsigned long currentMillis = millis();


String cuerpo_respuesta;
int codigo_respuesta;
int estadoN;

bool httpCompleted = false;


////////////// Coonfig. Reading Volume /////////
 void volumen()
 {
  volumenanalogico = analogRead(34);
  vol = (((volumenanalogico/4095)*3.3))*100;
  Serial.print("El valor del voltaje es:");
  Serial.println(vol);

 }
////////////// Coonfig. Reading Voltaje /////////
void voltaje ()
{
   voltajeanalogico = analogRead(35);
   vin = (((voltajeanalogico/4095)*3.3))*100;
   Serial.print("El voltaje es:");
   Serial.println(vin);
}

 void sleepp()
 {
    esp_sleep_enable_timer_wakeup(*Ptr4 * 1000000);
    //esp_light_sleep_start();
    esp_deep_sleep_start();
   // Reducir la frecuencia del reloj del CPU
  //  setCpuFrequencyMhz(10);  // Configura el reloj a 10 MHz
 }
/////////// Config. SendData ///////////////
 int SenData(int n)
{
 unsigned long startTime = millis();  // Registrar el tiempo al inicio
  //if (currentMillis - previousMillis >= interval) {
   //previousMillis = currentMillis;
    // Tu código que quieres ejecutar después del intervalo de 2 segundos va aquí.
    while(n == 0)
    {

      if(WiFi.status() == WL_CONNECTED)
       {
          voltaje();
          volumen();
          HTTPClient http;
          delay(500);
          http.begin("http://serverw.southcentralus.cloudapp.azure.com/randvservice/randv/volume/MDCCV-000001/"+ String(vol)+"/"+ String(vin));

          codigo_respuesta = http.GET();
          Serial.println(codigo_respuesta);
          if (codigo_respuesta > 0)
          {
            Serial.println("codigo HTTP" + String(codigo_respuesta));
            if(codigo_respuesta == 200)
            {
              cuerpo_respuesta = http.getString();
              Serial.println("El servidor respondio");
              Serial.println(cuerpo_respuesta);
             // VerifyWifi();
              http.end();//libero recursos
              httpCompleted = true;  // actualiza la bandera
              n = 1;//Enviamos un 1 si se cumplio
              estadoN = n;
            }
         }
         else
         {
            Serial.print("Error al enviar codigo:");
            Serial.println(codigo_respuesta);
            Serial.println("Volviendo a enviar un REQUEST al serevidor");
            n = 0;
            estadoN = n;
            //http.end();//libero recursos
            VerifyWifi();
            httpCompleted = false;  // actualiza la bandera

         }
      
        }
       VerifyWifi();
     }  
 return n;
}

////////////// Coonfig. WIFIMANAGER /////////
void printMessage()
{
/*********DESARROLLO DE TEXTBOX PARA EL USUARIO ************/
    WiFiManagerParameter custom_text_box("key_text", "Enter desired hour", hour, 3);
    WiFiManagerParameter custom_text_box_2("key_text2", "Enter desired minute", minute, 3);
    WiFiManagerParameter custom_text_box_3("key_text3", "Enter desired second", second, 3);
    WiFiManagerParameter custom_text_box_4("key_text4", "Enter desired  day", dia, 3);
    WiFiManagerParameter custom_text_box_5("key_text5", "Enter desired month", mes, 3);
    WiFiManagerParameter custom_text_box_6("key_text6", "Enter desired year", anio, 5);
    WiFiManagerParameter custom_text_box_7("key_text7", "Cada cuantos minutos se genera la actualizacion", Valor_minute, 3);
  


    wifiman.addParameter(&custom_text_box);
    wifiman.addParameter(&custom_text_box_2);
    wifiman.addParameter(&custom_text_box_3);

    wifiman.addParameter(&custom_text_box_4);
    wifiman.addParameter(&custom_text_box_5);
    wifiman.addParameter(&custom_text_box_6);
    wifiman.addParameter(&custom_text_box_7);
    
    //TryConnect();
     TryConnectWifi();


    /*********** Impresion de valores ingresados*****/
    strncpy(hour, custom_text_box.getValue(), sizeof(hour));
    Serial.print("Hora:");
    Serial.println(custom_text_box.getValue());
    

    strncpy(minute, custom_text_box_2.getValue(), sizeof(minute));
    Serial.print("Minuto:");
    Serial.println(custom_text_box_2.getValue());

    strncpy(second, custom_text_box_3.getValue(), sizeof(second));
    Serial.print("Segundo: ");
    Serial.println(custom_text_box_3.getValue());

    strncpy(dia, custom_text_box_4.getValue(), sizeof(dia));
    Serial.print("Dia:");
    Serial.println(custom_text_box_4.getValue());

    strncpy(mes, custom_text_box_5.getValue(), sizeof(mes));
    Serial.print("Mes:");
    Serial.println(custom_text_box_5.getValue());

    strncpy(anio, custom_text_box_6.getValue(), sizeof(anio));
    Serial.print("Anio:");
    Serial.println(custom_text_box_6.getValue());

    strncpy(Valor_minute, custom_text_box_7.getValue(), sizeof(Valor_minute));
    Serial.print("Tiempo en minutos para actualizacion:");
    Serial.println(custom_text_box_7.getValue());
    


   now = myRTC.now(); // Obtiene la fecha y hora del RTC
    myRTC.adjust(DateTime(atoi(custom_text_box_6.getValue()),
                          atoi(custom_text_box_5.getValue()),
                          atoi(custom_text_box_4.getValue()),
                          atoi(custom_text_box.getValue()),
                          atoi(custom_text_box_2.getValue()),
                          atoi(custom_text_box_3.getValue())));




   Time1  = atoi(custom_text_box_6.getValue());
   Time2 = atoi(custom_text_box_5.getValue());
   Time3 =  atoi(custom_text_box_4.getValue());
   Time4 = atoi(custom_text_box.getValue());
   Time5 = atoi(custom_text_box_2.getValue());
   Time6 = atoi(custom_text_box_3.getValue());

  Valor_minute1 = (atoi(custom_text_box_2.getValue()));
  Valor_second1 = (atoi(custom_text_box_3.getValue()));
  Valor_hour1 = (atoi(custom_text_box.getValue()));
   
  Valor_minute2 = (60-6)*(atoi(custom_text_box_7.getValue()));
 // Valor_hour2 = 3600*(atoi(custom_text_box_8.getValue()));
}

void VerifyWifi()
 {
   if(httpCompleted == false)
     {
      Serial.println("/Deteccion de internet empezada/");

          /********************************************** Instrucciones añadidas*******************************************************/

       /*Inicio para poner a prueba la autoconeccion despues de perder la conexion wifi */

       if(WiFi.status() != WL_CONNECTED)
        {
        
          
          unsigned long currentTime = millis();

          /* Empieza el test para verificar el tiempo sin wifi del ESP32*/
          if (currentTime % AUTOCONNECT_INTERVAL >= 8600 )
            {    
              Serial.print(F("-----> Tiempo transcurrido en millis(): "));
              Serial.println(currentTime);
              Serial.println(F("******intento de Conexion fallido, pasaron mas de 60 segundos:******"));
              Serial.flush();

              /*** ESTA SECCION ES UNICA ESPECIFICAMENTE PARA EL CONTROL DE VARIABLES NVS ****/
              NoConnection++;
              // Guardar el contador de Wi-Fi en la memoria no volátil
              preferences2.begin("NoConnection",false);
              preferences2.putInt("NoConnection", NoConnection);
              preferences2.end();

              //Accedemos al NVS creado, verificamos acceso
              bool nvsInit = preferences2.begin("NoConnection",false);
              if (!nvsInit) 
              {
                  Serial.println("---******Error al inicializar NVS---******");
                  return;
              } 
              else
               {
                  Serial.println("---******NVS inicializado correctamente---******");
               }
              
              // Volvemos acceder al NVS creado para obtener el valor guardado en la variable "NoConnection"
             
             
              

              // Obtener e imprimir el valor de NVS
              storedValue = preferences2.getInt("NoConnection", -1);  // -1 es un valor por defecto en caso de error
              if(storedValue == -1) 
              {
                  Serial.println("Error al recuperar NoConnection");
              } 
              else 
              {
                  Serial.print(F("-----> Número de conexiones a Wi-Fi fallidas : "));
                  Serial.println(storedValue);
              }
              preferences2.end();

              /*** AQUI SE TERMINA EL CONTROL DE VARIABLES NVS ****/

             /* i=0;
              preferences2.begin("i", false);
              preferences2.putUInt("i", i);
              Serial.println(F(i));
            */

            /* Autoconecciones intentadas antes de resetearse el ESP32 cuando pierde la conexion wifi*/

              if(storedValue >=4)
            {
                WiFi.mode(WIFI_STA);

                NoConnection=0;
                preferences2.begin("NoConnection",false);
                preferences2.putInt("NoConnection", NoConnection);
                storedValue = preferences2.getInt("NoConnection", -1);
                /*Serial.println(F(NoConnection));
                Serial.println(F("Numero de intentos fallidos alcanzados al conectarse a la misma red WiFi"));*/
                preferences.end();  
                //wifiman.resetSettings();
               // Serial.print("*****Intento maximos de conexion a red wifi superados*****");
                ESP.restart();
                
              }
              //Al no cumplirse el numero maximo de intentos de autoconexion seguira ejecutandose el programa sin resetearse el ESP32*/
              //Es decir aqui cuando se autoconecte 1,2,...6 veces al perder wifi permitidas
              // el ESP32 intentara autoconetctarse, levantando primero el AP y generando la conexion a la red WIFI con las credenciales 
              //previamente guardadas, estas se perderan solo cuando se resetee el ESP32
             /* else
                {

                  bool connect = wifiman.autoConnect("PROBE_CONNECTION AP", "password"); // intentar autoconectarse
          
                  if (connect)
                  {

                    //Incrementamos
                    wifiCount++;
                    Serial.println("conexion exitosa3");
                    //Serial.println("Entro al modo sueño");
                  // WiFi.begin(wifiman.getWiFiSSID().c_str(), wifiman.getWiFiPass().c_str());
                    setupSpiffs();
                    // Guardar el contador de Wi-Fi en la memoria no volátil
                    preferences.putInt("wifiCount", wifiCount);
                    
                    Serial.print(F("-----> Número de conexiones a Wi-Fi : "));
                    Serial.println(F(wifiCount));
                  }
                  else
                  */// Si en dado caso por alguna razon no se pudo levanatar el AP y autoconectarse , se volevera a intentar*/
                    /*{
                    bool connect = wifiman.autoConnect("PROBE_CONNECTION AP", "password"); // intentar autoconectarse
          
                      if (connect)
                      {
                          //Incrementamos
                          wifiCount++;
                          Serial.println("conexion exitosa4");
                        // Serial.println("Entro al modo sueño");
                          //WiFi.begin(wifiman.getWiFiSSID().c_str(), wifiman.getWiFiPass().c_str());
                          setupSpiffs();
                          // Guardar el contador de Wi-Fi en la memoria no volátil
                          preferences.putInt("wifiCount", wifiCount);
                          
                          Serial.print("-----> Número de conexiones a Wi-Fi : ");
                          Serial.println(F(wifiCount));
                        }

                    }       
                }*/
        
            }
          Serial.println(F("********sale de desconexion**********"));
        
        }
    }
 }
/*void checkDates(){

    DateTime now = myRTC.now(); // Obtiene la hora actual
    Serial.print("Fecha y Hora Actual: ");
    Serial.print(now.year()); // Año
    Serial.print('/');
    Serial.print(now.month()); // Mes
    Serial.print('/');
    Serial.print(now.day()); //
    Serial.print(' ');
    Serial.print(now.hour()); // Obtiene la hora
    Serial.print(":");
    Serial.print(now.minute()); // Obtiene los minutos
    Serial.print(":");
    Serial.print(now.second()); // Obtiene los segundos
    Serial.println();
}*/
void printTime()
{

  while (true) 
  { 
    now = myRTC.now(); // Obtiene la fecha y hora del RTC
    Serial.print(now.year(), DEC); // Año
    Serial.print('/');
    Serial.print(now.month(), DEC); // Mes
    Serial.print('/');
    Serial.print(now.day(), DEC); //
    Serial.print(' ');
    Serial.print(now.hour(), DEC); // Horas
    Serial.print(':');
    Serial.print(now.minute(), DEC); // Minutos
    Serial.print(':');
    Serial.println(now.second(), DEC); // Segundos
    delay(1000);
    VerifyWifi();
    Serial.println("************---------------------------------------****************");
    /*if( (now.second() % *Ptr3)>= 0)   
      { // Si ha pasado un minuto desde la última actualización
        Serial.println(F("Actualización de fecha y hora"));
        //updateVariables(); // Actualiza las variables con los nuevos valores del RTC
      }*/
    if (*Ptr4 != 0 )
      {
         if (now.unixtime() - lastCheckedTime.unixtime() >= *Ptr4 && now.second() >= *Ptr3) 
            // hora actual del RTC en segungos - la ultima hora >=  60*1min && 4min % 4 == 0 && 10 >= 10
           {
              i++;
              Serial.println(i);
              
              Serial.println("--**Actualizacion de logeo en URL...--**");
              lastCheckedTime = now; //Almacena la hora actual como la última hora comprobada
              if ((i==2) || (i>=3))
              {
                  SenData(0);
                  delay(3000);

                  if (httpCompleted == true) 
                  {
                      esp_sleep_enable_timer_wakeup(*Ptr4 * 1000000);
                      esp_light_sleep_start();
                      //esp_deep_sleep_start(); // Notarás que el código nunca llegará más allá de este punto.
                      httpCompleted = false;  // resetea la bandera para la próxima iteración
                      
                  }
                   ciclos = 1;
                    Serial.print("Salio del modo sueño y\t");
                    VerifyWifi();     

              }
           }
        } 
   
  }
}



const char* reset_reason(RESET_REASON reason)
{
  switch (reason)
  {
    case 1  : return "POWERON_RESET";
    case 3  : return "SW_RESET";
    case 4  : return "OWDT_RESET";
    case 5  : return "DEEPSLEEP_RESET";
    case 6  : return "SDIO_RESET";
    case 7  : return "TG0WDT_SYS_RESET";
    case 8  : return "TG1WDT_SYS_RESET";
    case 9  : return "RTCWDT_SYS_RESET";
    case 10 : return "INTRUSION_RESET";
    case 11 : return "TGWDT_CPU_RESET";
    case 12 : return "SW_CPU_RESET";
    case 13 : return "RTCWDT_CPU_RESET";
    case 14 : return "EXT_CPU_RESET";
    case 15 : return "RTCWDT_BROWN_OUT_RESET";
    case 16 : return "RTCWDT_RTC_RESET";
    default : return "NO_MEAN";
  }
}

 
void TryConnectWifi()
{
  Serial.print(F("Failed conncection, try again \n"));
  bool connects = wifiman.autoConnect("PROBE_CONNECTION AP", "password");
  Serial.println("--->Conexion AP exitosa<---");  
}

void saveWiFiCredentials()
{
  ssid2 = wifiman.getWiFiSSID();
  password2 = wifiman.getWiFiPass();

  Serial.println("*************************************************************************");
  Serial.println(F("********       GUARDANDO CREDENCIALES WiFi EN SISTEMA SPIFFS      ********"));
  Serial.println("*************************************************************************");

  // Crear documento JSON
  StaticJsonDocument<512> json;
  json["ssid"] = ssid2;
  json["password"] = password2;



      // Abriendo archivo para escritura
      File configFile = LittleFS.open("/wifi_credentials.json", "w");
      if (!configFile)
      {
        Serial.println("*************************************************************************");
        Serial.println("*****  NO SE PUDO ABRIR EL ARCHIVO DE CREDENCIALES PARA ESCRIBIR    ****");
        Serial.println("*************************************************************************");
        return;

      }
    // Serializar datos JSON para escribir en el archivo
      serializeJsonPretty(json, Serial);
      if (serializeJson(json, configFile) == 0)
      {
        Serial.println("*************************************************************************");
        Serial.println(F("********          Error al escribir en el archivo             ********"));
        Serial.println("*************************************************************************");
      }

  // Cerrar archivo
  configFile.close();
  Serial.println("*************************************************************************");
  Serial.println(F("********             TERMINO PROCESO SPIFFS                    ********"));
  Serial.println("*************************************************************************");


}

bool conectarWifiJson() 
{
  //delay(5000);
  Serial.println("*************************************************************************");
  Serial.println(F("********  CARGANDO CREDENCIALES WiFi DE SISTEMA SPIFFS          ********"));
  Serial.println("*************************************************************************");
 
  if (LittleFS.exists("/wifi_credentials.json")) {
    // El archivo existe, proceder a leerlo
    File configFile = LittleFS.open("/wifi_credentials.json", "r");
    if (!configFile) {
      Serial.println("Error al abrir el archivo de credenciales para lectura");
      return false;
    }

    // Deserializar el JSON desde el archivo
    StaticJsonDocument<512> json;
    DeserializationError error = deserializeJson(json, configFile);
    if (error) {
      Serial.println("Error al leer el JSON desde el archivo de credenciales");
      return false;
    }


    // Asignar las credenciales a las variables
    const char* ssid = json["ssid"];
    const char* password = json["password"];

    if (ssid && password) {
      WiFi.begin(ssid, password);
      unsigned long startAttemptTime = millis();

      while (WiFi.status() != WL_CONNECTED && millis()) { // Espera hasta 10 segundos para la conexión
        delay(100);
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Conectado a: ");
        Serial.println(ssid);
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.println("Pasword :\t");
        Serial.print(password);
        return true;
      }
    }
  }

  Serial.println("No se encontraron credenciales WiFi guardadas o son inválidas.");
  printMessage();
  return false;
}

void setupSpiffs()
{
 if (!LittleFS.begin(true)) 
           { // Inicializar SPIFFS
              Serial.println("Error al montar sistema de archivos");
              return;
           }

          // Intenta conectarse con las credenciales guardadas
          if (!conectarWifiJson()) 
          {
            // Si no pudo conectarse con las credenciales almacenadas, lanza WiFiManager 
            //TryConnectWifi();
            wifiman.autoConnect();
            delay(1000);
            // Después de obtener la conexión, guarda las credenciales
            saveWiFiCredentials();
          }
 
}
void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin(); // Inicia el puerto I2C
  myRTC.begin(); // Inicia la comunicación con el RTC
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Desactiva el brownout detector
  delay(1000);   

  RESET_REASON reason = rtc_get_reset_reason(0);

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();


  //if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
  if(reason == 12)
   {
      //Serial.println("Despertó del sueño profundo debido al temporizador");
       wifiman.resetSettings();
       printMessage();
      // Si la razón del reset es SW_CPU_RESET
          if (!LittleFS.begin()) 
          {
            Serial.println("===Error al montar el sistema de archivos LittleFS===");
            return;
          }

          if (LittleFS.format()) 
          { 
            Serial.println("====LittleFS formateado correctamente===");
          } 
          else 
          {
            Serial.println("===Error formateando LittleFS===");
          }

       LittleFS.end();  // Cerrar LittleFS

       

       Serial.println("\n===El ESP32 se ha reseteado de fabrica debido a :");
       Serial.print("\t*****Intento maximos de conexion a red wifi superados*****\n");
       Serial.println("---Reinicio de fabrica---");
       Serial.print("---Reset Reason:---");
       Serial.println(reset_reason(reason));
   }
   else
   {
      // Si no es por despertar del sueño, entonces es el primer arranque
      //ManejoReset();
      wifiman.resetSettings();
      if (reason == 1) 
      { 
        // Si la razón del reset es POWERON_RESET
          if (!LittleFS.begin()) 
          {
            Serial.println("===Error al montar el sistema de archivos LittleFS===");
            return;
          }

          if (LittleFS.format()) 
          { 
            Serial.println("===LittleFS formateado correctamente===");
          } 
          else 
          {
            Serial.println("===Error formateando LittleFS===");
          }

       LittleFS.end();  // Cerrar LittleFS

       Serial.println("===El ESP32 se ha encendido o se ha reseteado por alimentación.===");
       Serial.println("---Reinicio de fabrica---");
       Serial.print("---Reset Reason:---");
       Serial.println(reset_reason(reason));
       ESP.restart();

      }

   } 
  Serial.print("Reset Reason: ");
  Serial.println(reset_reason(reason));
  setupSpiffs();
}  
void loop() 
{
  // put your main code here, to run repeatedly:
   if (WiFi.status() == WL_CONNECTED)
   {
    Serial.print("\nYa estamos conectados al WIFI\n");
   }

   printTime();
   
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
   {
      Serial.println("Despertó del sueño profundo debido al temporizador");
      //printMessage();
   }
}