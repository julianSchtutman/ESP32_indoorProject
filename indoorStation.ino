//#include <ESP8266WiFi.h>          
#include <DNSServer.h>
//#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <datoSensorLib.h>

/*PROPIO DEL SENSOR*/
#include <SensorDHTLib.h>
/******************/



//PARAMETROS CON MAYOR FRECUENCIA DE CONFIGURACION
#define SLEEP_PERIOD 5e6 //En us. Tiempo que el esp se va a dormir luego de hacer la lectura.
#define WIFI_NAME "SENSOR_NET" //nombre de la red wifi que pone el esp cuando se pone en modo configuración
#define WIFI_PASSWORD "password" //password de la red que abre el esp
#define INFLUX_IP "188.166.6.107";
#define INFLUX_PORT 8086


#define LED 2 //2 es led interno
#define ON LOW
#define OFF HIGH

//Para manejar el parpadeo del LED
Ticker ticker;

//for HTTP to Influx
WiFiClient client;

//Cambia el estado de encendido del LED
void tick(){
 int state = digitalRead(LED);  
 digitalWrite(LED, !state);     
}


//Esta funcion se llama cuando se entra al modo de configuración WIFI.
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

//Esta funcion se llama cada vez que se quiere mandar a dormir al ESP
void goToSleep(){
  delay(1000); //proteccion para que termine algo que este terminando
  Serial.println("Me voy a dormir " + String(SLEEP_PERIOD) + " microsegundos");
  ESP.deepSleep(SLEEP_PERIOD);
  
}

void sendToInflux(DatoSensor dato){
  String sentencia; //la sentencia influx se va a formar como "medicion,sensor=nombre_sensor value=valor" ej. temperatura,sensor=SENSOR1 value=20.32
  sentencia = dato.getMedicion() + ",sensor=" + dato.getNombreSensor() + " value=" + dato.getValor();
 
  Serial.print("Enviando a Influx: ");
  Serial.println(sentencia);
  client.print("POST /write?db=mediciones HTTP/1.1\r\n");
  client.print("User-Agent: esp8266/0.1\r\n");
  client.print("Host: localhost:8086\r\n");
  client.print("Accept: */*\r\n");
  client.print("Content-Length: " + String(sentencia.length()) + "\r\n");
  client.print("Content-Type: application/x-www-form-urlencoded\r\n");
  client.print("\r\n");
  client.print(sentencia + "\r\n");
  client.flush();
 
}

void setup() {
  const int httpPort = INFLUX_PORT;
  const char* host = INFLUX_IP
  int n;

  //Si se esta usando el medidor de consumo dejar la velocidad en 9600
  Serial.begin(115200);
  //set led pin as output
  pinMode(LED, OUTPUT);     //Inicializa el puerto del LED como salida
  digitalWrite(LED, OFF);

   // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.5, tick);
  
  WiFiManager wifiManager;
  //Habilitar la linea de abajo cuando se este debugiando y se quiera "borrar" la configuracion wifi guardada en EPROM
  //wifiManager.resetSettings();

  //timeout para que si no se puede conectar , a los n segundos salga por error y se vaya a dormir.
  wifiManager.setConfigPortalTimeout(600); //segundos por favor
  
  //Configura la funcion que se llama cuando falla en conectarse a la ultima wifi
  wifiManager.setAPCallback(configModeCallback);

  //La siguiente funcion intenta conectase con la ultima configuración wifi. Si no puede se pasa a modo configuración. Si se cumple el tiempo de timeout sale por falso.
  if (!wifiManager.autoConnect(WIFI_NAME,WIFI_PASSWORD)) {
    Serial.println("failed to connect and hit timeout");
    goToSleep();
  }

  //--------------------------------------       A PARTIR DE ACA ESTAMOS CONECTADOS A WIFI         ----------------------------------
  digitalWrite(LED, OFF);
  ticker.detach(); 
  Serial.println("connected to wifi :)");

 
  //-------------ME CONECTO A LA BASE DE DATOS
   if (client.connect(host, httpPort)){
      Serial.println("connected to Influx");

    } else {
        Serial.println("Connection to influx failed, rebooting..."); 
        goToSleep();
    }
  

  
  
  //LECTURA DE SENSORES - 
  DatoSensor dato;
  SensorDHT sensorDHT("Sensor_Temperatura");
  if(sensorDHT.readDataConsumo()){
  
        dato = sensorDHT.getTemperature();
        sendToInflux(dato);

        dato = sensorDHT.getHumidity();
        sendToInflux(dato);

        dato = sensorDHT.getHeatIndex();
        sendToInflux(dato);
        
  }
  else{
      Serial.println("Error leyendo datos del sensor");
    
  }
  delay(500);

  goToSleep();
 

 
}

void loop() {
  //Aca no se hace nada porque usamos el esp en modo DeepSleep

}
