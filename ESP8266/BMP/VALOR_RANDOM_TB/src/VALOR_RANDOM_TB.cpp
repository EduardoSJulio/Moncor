// *************************************************************
// 
// Esse código tem como objetivo utilizar uma função alternativa 
// para enviar os dados ao ThingsBoard, então para agilizar os
// testes, utilizou-se um gerador de números aleatórios para 
// simular a leitura de um BMP. Além disso, pode ser útil quando
// o objetivo é testar a comunicação entre ESP8266 e não há um 
// sensor disponível. Bastar alterar as informações  definidas 
// no início do código para utilizá-lo.
//
// *************************************************************

#include "PubSubClient.h"
#include "ESP8266WiFi.h"
#include "ArduinoHttpClient.h"

#define STATION_SSID         "TELECOM_LAMMI"                 // Nome da rede Wi-Fi
#define STATION_PASSWORD     "schottky"                      // Senha Wi-Fi
#define THINGSBOARD_SERVER   "lavoisier.eletrica.ufpr.br"    //Endereço do servidor
#define THINGSBOARD_PORT     8080                            //Porta
#define TOKEN                "rM1yFSkN86Yoa7MNN0qz"          //Token do Moncor

String contentType = "application/json";

// Criando o objeto WiFi
WiFiClient wifi;

// Adicionando atributos ao objeto client por meio do método HttpClient
HttpClient client = HttpClient(wifi, THINGSBOARD_SERVER, THINGSBOARD_PORT);

// Declando as funções
void sendToThingsBoard(float temperature, float pressure);
void setupWiFi(void);

void setup() {

  // Inicialize a comunição serial
  Serial.begin(9600);

  //Inicialize a conexão Wi-Fi
  setupWiFi();

}

void loop() {

  // Teste a conexão Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    //Conecte-se a rede Wi-Fi
    setupWiFi();
  }

  //Adicione um valor random de 0 a 180 na variável temperature e pressure
  float temperature = random(0,180); 
  float pressure = random(0,180); 

  // Envie os valores ao Thingsboard
  sendToThingsBoard(temperature, pressure);

  // Aguarde 5 segundos
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

// Função responsável por enviar o valor aleatório para ThingsBoard
void sendToThingsBoard(float temperatura, float pressure){

  Serial.println("making POST request");

  // Criando um JSON 
  String postData = "{";
  postData += "\"temperatura\":"; 
  postData += temperatura; 
  postData += ",";
  postData += "\"tensao\":"; 
  postData += pressure;
  postData += "}";

  // Criando a String com o caminho para envio
  String path = String("/api/v1/") + TOKEN + "/telemetry";
  
  // Envia o JSON para o ThingsBoard usando o método POST via HTTP
  client.post(path, contentType, postData);

  // Leitura do status do código e resposta
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

}
