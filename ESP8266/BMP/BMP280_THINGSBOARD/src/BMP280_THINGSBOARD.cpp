// *************************************************************
//
// Este código faz a leitura do sensor BMP280 e envia os 
// valores de pressão e temperatura ao ThingsBoard.
// O envio dos valores é feito a cada 5 segundos. Bastar alterar 
// as informações definidas no início para utilizá-lo.
//
// **************************************************************

#include <Wire.h>
#include <ThingsBoard.h>
#include <ESP8266WiFi.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

#define STATION_SSID        "TELECOM_LAMMI"                    // Nome da rede Wi-Fi
#define STATION_PASSWORD    "schottky"                         // Senha Wi-Fi
// Configurações do ThingsBoard
#define THINGSBOARD_SERVER  "lavoisier.eletrica.ufpr.br"       // Endereço do servidor
#define TOKEN               "rM1yFSkN86Yoa7MNN0qz"             // Token do dispositivo

// Criando o objeto espClient
WiFiClient espClient;

// Inicialize a instância ThingsBoard
ThingsBoard tb(espClient);

// Inicialize o BMP180 ou BMP085
Adafruit_BMP280 bmp;

// Declarando as funções
void setupWiFi(void);
void sendToThingsBoard(float temperature, float pressure);

void setup() {

  // Inicialize a comunicação serial
  Serial.begin(9600);
  
  // Inicialize a comunicação I2C na GPIO 4 e 5
  Wire.begin(4,5);

  // Inicialize o sensor
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP180!");
  while (1){}
  }

  // Configurações padrões do datasheet
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     // Operating Mode. 
                  Adafruit_BMP280::SAMPLING_X2,     // Temp. oversampling 
                  Adafruit_BMP280::SAMPLING_X16,    // Pressure oversampling 
                  Adafruit_BMP280::FILTER_X16,      // Filtering. 
                  Adafruit_BMP280::STANDBY_MS_500); // Standby time. 

  // Conecte-se a rede Wi-Fi
  setupWiFi();
}

void loop() {

  // Teste a conexão Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    //Conecte-se a rede Wi-Fi
    setupWiFi();
  }

  // Faça a leitura dos sensores
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure();

  // Envie para TB a leitura do sensor
  sendToThingsBoard(temperature, pressure);

  // Aguarde 5 segundos
  delay(5000);

  tb.loop();

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

// Função responsável por enviar leitura feita pelo sensor para ThingsBoard
void sendToThingsBoard(float temperature, float pressure){

  //SE não estiver conectado ao Thingsboard
  if (!tb.connected()) {
    
    // Faça um serial print das informações sobre o Thingsboard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);

    // Tente se conectar ao Thingsboard
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }

    Serial.println("Sending data to TB...");
    
    // Envia os valores lidos pelo sensor ao Thingsboard usando uma função da biblioteca Thingboard.h
    // Notei instabilidade nessa função, se não funcionar é necessário construir a função. 
    // Tem um código no repositório para contornar isso.
    tb.sendTelemetryFloat("Temperature", temperature);
    tb.sendTelemetryFloat("Pressure", pressure);
}