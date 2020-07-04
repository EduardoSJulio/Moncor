// *************************************************************
//
// Este código faz a leitura do sensor BMP180 e do ADC do ESP8266. 
// O ADC deve estar conectado à saída de um dividor de tensão 
// para que a tensão da bateria possa ser mensurada. A resistência 
// dos resistores foram de 100k e 330k e a montagem desse experimento
// pode ser encontrada no meu relatório da IC.
// Os valores de temperatura e pressão enviados ao ThingsBoard 
// a cada 5 segundos. No entanto, a tensão da bateria é enviada
// a cada 10 min. Bastar alterar as informações definidas no início 
// para utilizá-lo.
//
// **************************************************************

#include <Wire.h>
#include <ThingsBoard.h>
#include <ESP8266WiFi.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_Sensor.h>

#define STATION_SSID        "TELECOM_LAMMI"                    // Nome da rede Wi-Fi
#define STATION_PASSWORD    "schottky"                         // Senha Wi-Fi
// Configurações do ThingsBoard
#define THINGSBOARD_SERVER  "lavoisier.eletrica.ufpr.br"       // Endereço do servidor
#define TOKEN               "rM1yFSkN86Yoa7MNN0qz"             // Token do dispositivo

int lastSendForVin      = 0;               // Último envio de Vin servidor inicia em 0 segundos
int lastSendForSensor   = 0;               // Último envio do Sensor ao servidor inicia em 0 segundos

float temperature, pressure, vout, vin;

// Criando o objeto espClient
WiFiClient espClient;

// Inicialize a instância ThingsBoard
ThingsBoard tb(espClient);

// Inicialize o BMP180 ou BMP085
Adafruit_BMP085 bmp;

// Declarando funções
void sendTeletry(void);
void setupWiFi(void);

void setup() 
{
  // Inicialize a comunicação serial
  Serial.begin(9600);
  
  // Inicialize a comunicação I2C na GPIO 4 e 5
  Wire.begin(4,5);
  
  // Inicialize a comunicação com o BMP
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP180!");
  while (1){}
  }

  // Conecte-se a rede Wi-Fi
  setupWiFi();
}

void loop() 
{

  // Teste a conexão Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    //Conecte-se a rede Wi-Fi
    setupWiFi();
  }

  // Envie para TB a leitura do ADC e sensor
  sendTeletry();
  
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

void sendTeletry(){

  // SE não estiver conectado ao Thingsboard
  if (!tb.connected())
  {
    
    // Faça um serial print das informações sobre o Thingsboard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);

    // Tente se conectar ao Thingsboard
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN))
    {
      Serial.println("Failed to connect");
      return;
    }
  }

  // A cada 5 min
  if ( millis() - lastSendForSensor > 5000)
  { 
    // Faça a leitura dos sensores
    temperature = bmp.readTemperature();
    pressure = bmp.readPressure();

    // Envia os valores lidos pelo sensor ao Thingsboard usando uma função da biblioteca Thingboard.h
    tb.sendTelemetryFloat("Temperature", temperature);
    tb.sendTelemetryFloat("Pressure", pressure);

    Serial.println("Sending sensor data...");

    // Atualize o tempo do último envio do Sensor
    lastSendForSensor = millis();
  }
  
  // A cada 10 minutos
  if ( millis() - lastSendForVin > 600000)
  {

    // Vout = (Vin * R2)/(R1+R2)
    // 1V --- 1023 
    vout = (analogRead(A0) * 1.0/1023.0);
    Serial.printf("Vout = %f", vout);
    Serial.println("");

    // Vin = (Vout * (R1+R2))/R2
    vin = vout * 4.3 * 0.98; // 0.98 é um fator de calibração do ADC
    Serial.printf("Vin = %f", vin);
    Serial.println("");

    Serial.println("Sending batery voltage (Vin)...");

    // Envia os valores lidos pelo ADC ao Thingsboard usando uma função da biblioteca Thingboard.h
    tb.sendTelemetryFloat("Vin", vin);

    // Atualize o tempo do último envio do Vin
    lastSendForVin = millis();
  }
  
}