// ***************************************************************
// 
// Esse código tem como objetivo utilizar o sensor DS18B20 para
// o monitoramento remoto e contínuo da temperatura corporal.
// Foi desenvolvido para conectar o ESP8266 ao WiFi e enviar o
// valor da temperatura para o servidor (neste caso, o NodeRED)
// via o protocolo MQTT.
// 
// ***************************************************************

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const int PINO_ONEWIRE = 12;         // Define pino do sensor
OneWire oneWire(PINO_ONEWIRE);       // Cria um objeto OneWire
DallasTemperature DS18B20(&oneWire); // Informa a referência da biblioteca dallas temperature para Biblioteca onewire
DeviceAddress endereco_temp;         // Cria um endereco temporário da leitura do sensor

const char* STATION_SSID = "NET_2";                       // Nome da rede Wi-Fi
const char* STATION_PASSWORD = "1001271000";              // Senha Wi-Fi
const char* MQTT_SERVER = "lavoisier.eletrica.ufpr.br";   // Endereço do servidor
 
// Declarando as funções
void setupWiFi(void);
void reconnect(void);

// Criando o objeto espClient
WiFiClient espClient;

// Inicialize a instância client
PubSubClient client(espClient);

void setup() {

  // Inicialize a comunicação serial
  Serial.begin(9600);
  
  // Inicialize o sensor
  DS18B20.begin();
  
  // Conecte-se a rede Wi-Fi
  setupWiFi();
  
  // Sete o servidor MQTT na porta 1880
  client.setServer(MQTT_SERVER, 1880);
}

void loop() {
  
  // Se não houver conexão, reconecte-se
  if (!client.connected()) {
    reconnect();
  }

  // Envia comandos aos dispositivos no barramento para fazer a conversão de temperatura
  DS18B20.requestTemperatures();
  // Obtém temperatura do dispositivo no index 0
  float valueTempDS18B20 = DS18B20.getTempCByIndex(0);
  // Cria um buffer
  char bufferTempDS18B20[30];
  // Adiciona no buffer a temperatura no formato JSON
  sprintf(bufferTempDS18B20,"{\"temperatura_DS18B20\": %.3f}",valueTempDS18B20);
  
  // Publica o JSON servidor mqtt
  client.publish("temperatura", bufferTempDS18B20);
  
  // Envia na comunicação serial a temperatura lida
  Serial.print(F("Temperature_DS18B20 = "));
  Serial.print(valueTempDS18B20);
  Serial.println(" *C");
  Serial.println();
  
  // Aguarda 5 segundos
  delay(5000);

}

// Função responsável pela conexão Wi-Fi
void setupWiFi(){

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(STATION_SSID);

  // Tente se conectar a rede Wi-Fi
  WiFi.begin(STATION_SSID, STATION_PASSWORD);

  // Aguarde conexão a ser estabelecida
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
}

void reconnect(){

  // Loop até que haja conexão
  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");
    /*
     Essa linha pode ser alterada se você tiver problemas com múltiplas conexões com MQTT
     Para mudar o ID do dispositivo, você terá que atribuir um novo nome ao ESP.
     Por exemplo, você pode fazer assim:
       if (client.connect("ESP1_Office")) {
     Então, para outro ESP:
       if (client.connect("ESP2_Garage")) {
    */
    if (client.connect("ESP8266")) {
      // Se tudo ocorrer bem, a conexão será estabelecida
      Serial.println("connected");  

    } 
    else{
      // Se algo der errado, o erro será gerado e apresentado no monitor serial
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Aguarda 5 segundos antes de tentar novamente
      delay(5000);
    }
  }
}