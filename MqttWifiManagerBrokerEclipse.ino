#if defined(ESP8266)
#include <ESP8266WiFi.h> //ESP8266 Core WiFi Library         
#else
#include <WiFi.h> //ESP32 Core WiFi Library    
#endif
 
#if defined(ESP8266)
#include <ESP8266WebServer.h> //Local WebServer used to serve the configuration portal
#else
#include <WebServer.h> //Local WebServer used to serve the configuration portal ( https://github.com/zhouhan0126/WebServer-esp32 )
#endif

#include "DHT.h"

#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal ( https://github.com/zhouhan0126/DNSServer---esp32 )
#include <WiFiManager.h> //WiFi Configuration Magic ( https://github.com/zhouhan0126/WIFIMANAGER-ESP32 ) >> https://github.com/tzapu/WiFiManager (ORIGINAL)

#include <PubSubClient.h>

int pinLed = 18;
int pinTerm = 19;
DHT dht(pinTerm, DHT11);

const int PIN_AP = 2;
WiFiClient wifiClient;

//MQTT Server
const char* BROKER_MQTT = "mqtt.eclipse.org"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883;                       // Porta do Broker MQTT
#define ID_MQTT "***EspTeste"       //Informe um ID unico e seu. Caso sejam usados IDs repetidos a ultima conexão irá sobrepor a anterior. 
#define TOPIC_SUBSCRIBE "***Led"    //Informe um Tópico único. Caso sejam usados tópicos em duplicidade, o último irá eliminar o anterior.
#define TOPIC_PUBLISH_TEMP "***Temp"
#define TOPIC_PUBLISH_HUMID "***Humid"
PubSubClient MQTT(wifiClient);                //Instancia o Cliente MQTT passando o objeto espClient

//Declaração das Funções
void publish();
void mantemConexoes();  //Garante que as conexoes com MQTT Broker se mantenham ativas
void conectaMQTT();     //Faz conexão com Broker MQTT
void recebePacote(char* topic, byte* payload, unsigned int length);
//void enviaPacote();

void setup() {
  Serial.begin(115200);
  pinMode(pinLed, OUTPUT);
  pinMode(PIN_AP, INPUT);
  Serial.println("DHTxx test!");
  dht.begin();

  //declaração do objeto wifiManager
  WiFiManager wifiManager;
 
  //utilizando esse comando, as configurações são apagadas da memória
  //caso tiver salvo alguma rede para conectar automaticamente, ela é apagada.
  //wifiManager.resetSettings();
 
  //callback para quando entra em modo de configuração AP
  wifiManager.setAPCallback(configModeCallback);
  
  //callback para quando se conecta em uma rede, ou seja, quando passa a trabalhar em modo estação
  wifiManager.setSaveConfigCallback(saveConfigCallback); 
 
  //cria uma rede de nome ESP_AP com senha 12345678
  wifiManager.autoConnect("ESP_AP", "12345678");

  MQTT.setServer(BROKER_MQTT, BROKER_PORT);  
  MQTT.setCallback(recebePacote); 
}

void loop() {
   if (!MQTT.connected()) {
      conectaMQTT(); 
   }
   MQTT.loop();
   
   if ( digitalRead(PIN_AP) == HIGH ) {
      WiFiManager wifiManager;
      if(!wifiManager.startConfigPortal("ESP_AP", "12345678")) {
        Serial.println("Falha ao conectar");
        delay(2000);
        ESP.restart();
      }
   }
   
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  char temp[20];
  char humid[20];
  sprintf(temp, "%2.0f", temperature);
  sprintf(humid, "%2.0f", humidity);
  MQTT.publish(TOPIC_PUBLISH_TEMP, temp);
  MQTT.publish(TOPIC_PUBLISH_HUMID, humid);
}

//callback que indica que o ESP entrou no modo AP
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entrou no modo de configuração");
  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
  Serial.println(myWiFiManager->getConfigPortalSSID()); //imprime o SSID criado da rede
}

//callback que indica que salvamos uma nova rede para se conectar (modo estação)
void saveConfigCallback () {
  Serial.println("Configuração salva");
}

void conectaMQTT() { 
    while (!MQTT.connected()) {
        Serial.print("Conectando ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado ao Broker com sucesso!");
            MQTT.subscribe(TOPIC_SUBSCRIBE);
        } else {
            Serial.println("Nao foi possivel se conectar ao broker.");
            Serial.println("Nova tentatica de conexao em 10s");
            delay(10000);
        }
    }
}

void recebePacote(char* topic, byte* payload, unsigned int length) {
    String msg;
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) {
       char c = (char)payload[i];
       msg += c;
    }
    if (msg == "0") {
       digitalWrite(pinLed, LOW);
    }
    if (msg == "1") {
       digitalWrite(pinLed, HIGH);
    }
}
