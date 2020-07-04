// **************************************************************
//
// Este código faz a leitura do sensor BMP180 e envia os 
// valores de pressão e temperatura ao ThingsBoard e EMONcms.
// O envio dos valores é feito a cada 5 segundos. Bastar alterar 
// as informações definidas no início para utilizá-lo.
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

// Configurações do EMONcms
const char* host      = "200.17.220.141";                      // Server ou host do EMONcms
const char* apikey    = "8d3cdd9e751fb13cc20d970b26fa96f7";    // Apikey EMONcms
int port              = 80;                                    // Porta
int nodedata          = 18;                                    // Nó ou grupo
int lastSend = 0;                                              // Último envio ao servidor inicia em 0 segundos

// Inicialize o cliente ThingsBoard
WiFiClient tbClient;

// Inicialize a instância ThingsBoard 
ThingsBoard tb(tbClient);

// Inicialize o cliente EmonCMS
WiFiClient emonClient;

// Inicialize o BMP180 ou BMP085
Adafruit_BMP085 bmp;

// Declando funções
void setupWiFi(void);
void sendToThingsBoard(float temperature, float pressure);
void sendToEMONCMS(float temperature, float pressure);

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

  // Conecte-se a rede Wi-Fi
  setupWiFi();
}

void loop() {
  
  // Teste a conexão Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    //Conecte-se a rede Wi-Fi
    setupWiFi();
  }

  // A cada 5 segundos 
  if (millis() - lastSend > 5000) {

    // Faça a leitura dos sensores
    float temperature = bmp.readTemperature();
    float pressure = bmp.readPressure();

    // Envie para TB e EMON a leitura do sensor
    sendToThingsBoard(temperature, pressure);
    sendToEMONCMS(temperature, pressure);

    // Atualize o tempo do último envio
    lastSend = millis();
  }

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

// Função responsável por enviar leitura feita pelo BMP180 para EMONcms
void sendToEMONCMS(float temperature, float pressure){

  Serial.println("Sending data to EMONCMS...");

  // Tente se conectar ao EMONcms
  if (!emonClient.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }

  // Envie requisição para o servidor postar dados do BMP180
  emonClient.print("GET /emoncms/input/post.json?node="+String(nodedata)+"&json={"+"Temperature:"+String(temperature)+","+"Pressure:"+String(pressure)+"}&apikey=" + apikey + " HTTP/1.1\r\n" +
  "Host: " + host + "\r\n" +
  "Connection: close\r\n\r\n");

  // Leia todas as linhas da resposta do servidor e imprima-as no serial monitor
  while(emonClient.available()){
    String line = emonClient.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");

}
