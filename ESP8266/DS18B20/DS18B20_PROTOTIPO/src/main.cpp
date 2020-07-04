// *********************************************************************
// 
// Esse código foi usado no protótipo que tem como objetivo utilizar 
// o sensor DS18B20 para monitorar remota e continuamente a temperatura 
// corporal de pessoas. Foi desenvolvido para conectar o ESP8266 ao 
// WiFi e enviar o valor da temperatura para o servidor 
// (neste caso, o ThingsBoard). Por meio da interação com o hardware 
// o usuário pode definir o ESP como ponto de acesso para alterar 
// configurações de rede. O funcionamento detalhado desse dispositivo 
// e código (fluxograma) pode ser encontrado no relatório armazenado 
// no GDrive.
// 
// **********************************************************************

#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ArduinoHttpClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ThingsBoard.h>
#include <EEPROM.h>

int PIN_APSETUP = 12;         // Pino para configurar Wi-Fi
String st;                    // Variável usada para armazenar o HTML da página do web server
String content;               // Variável usada para armazenar o HTML da página do web server
String esid;                  // Variável usada para armazenar o ssid lido da EEPROM
String epass;                 // Variável usada para armazenar o password lido da EEPROM
String etopic;                // Variável usada para armazenar o token lido da eeprom
String contentType = "application/json";       // Tipo de conteúdo, nesse caso é um json
char serverAddress[] = "200.17.220.141";       // Servidor do ThingsBoard
const int port = 8080;                         // Porta onde o ThingsBoard está instalado
const int PINO_ONEWIRE = 5;                    // Define pino do sensor

DeviceAddress endereco_temp;         // Cria um endereco temporario da leitura do sensor
OneWire oneWire(PINO_ONEWIRE);       // Cria um objeto OneWire
DallasTemperature DS18B20(&oneWire); // Informa a referencia da biblioteca dallas temperature 
                                     // para Biblioteca onewire

// Criando o objeto wifi
WiFiClient wifi;

// Adicionando atributos ao objeto client por meio do método HttpClient
HttpClient client = HttpClient(wifi, serverAddress, port);

// Declarando as funções
void testAP(void);
void setupAP(void);
void createWebServer(void);
String readSSID(void);
String readPASS(void);
String readTOPIC(void);
void setupWiFi(const char* ssid, const char* pass);
void sendToThingsBoard(float temperatura, String tensao);

// Cria um objeto "servidor" na porta 80 (http).
ESP8266WebServer server(80);

void setup() { 
  // Inicializa comunicação serial com 9600 de baud rate
  Serial.begin(9600);

  // Inicializando EEPROM
  EEPROM.begin(512); 
  delay(10);

   // Verifica se o usuário quer reconfigurar o Wi-Fi
  testAP();

  Serial.println();
  Serial.println("Startup");
  
  // Lendo dados gravados na EEPROM
  String ssid = readSSID();
  String pass = readPASS();

  // Desbilita o AP 
  WiFi.enableAP(0);
  
  // Conecte-se ao Wi-Fi cadastrado
  setupWiFi(ssid.c_str(), pass.c_str());

  // Inicializa sensor DS18B20
  DS18B20.begin();

}

void loop() {
  
  // Vout = (Vin * R2)/(R1+R2)
  // 1V --- 1023 
  float Vout = (analogRead(A0) * 1.0/1023.0);
  Serial.printf("Vout = %f", Vout);
  Serial.println();

  // Vin = (Vout * (R1+R2))/R2
  float Vin = Vout * 4.3 * 0.962; // FATOR DE CALIBRACAO
  Serial.printf("Vin = %f", Vin);
  Serial.println();

  String status_bateria;
  
  // Atribui o status da bateria de acordo com a tensão lida
  if(Vin >= 3.5){
    status_bateria = "ALTO";
  }
  else if (Vin >= 3.3 && Vin < 3.5)
  {
    status_bateria = "MÉDIO";
  }
  else{
    status_bateria = "BAIXO";
  }

  // Envia requisição aos dispositivos no barramento
  DS18B20.requestTemperatures();
  // Coleta o valor de temperatura no index 0
  float valueTempDS18B20 = DS18B20.getTempCByIndex(0);
  // Offset para calibrar o sensor
  valueTempDS18B20 += 1.00;
  
  // Transforma o valor lido em um vetor de caracteres com a informação
  char bufferTempDS18B20[5];
  sprintf(bufferTempDS18B20,"%.3f",valueTempDS18B20);
 
  // Mostra no monitor serial o valor da leitura
  Serial.print(F("Temperature_DS18B20: "));
  Serial.print(valueTempDS18B20);
  Serial.println(" *C");

  sendToThingsBoard(valueTempDS18B20, status_bateria);
  
  // Aguarda 1 segundo
  delay(1000);

  // Dorme por 5 Minutos (Deep-Sleep em Micro segundos).
  ESP.deepSleep(1 * 60000000);

}

void testAP(){
  // Pino com botão para inicializar configurações no Wi-Fi
  pinMode(PIN_APSETUP, INPUT);
  
  // Se o botão for apertado, inicia Ponto de Acesso para Configuração
  if (!digitalRead(PIN_APSETUP))
  {
    Serial.println("Disconnecting previously connected WiFi");
    WiFi.disconnect();
    
    Serial.println("Turning the HotSpot On");
    
    // Cria um ponto de acesso para o usuário configurar o dispositivo
    setupAP();

    Serial.println();
    Serial.println("Waiting for new data.");
    
    // Aguarda a atualização das informações
    while ((WiFi.status() != WL_CONNECTED))
    {
      Serial.print(".");
      delay(100);
      server.handleClient();
    }
  }
}

// Função responsável por criar um ponto de acesso ao usuário
void setupAP(void){
  // Define o nome do ponto de acesso criado
  WiFi.softAP("MonCoT");
  // Configura o dispositivo como station (ponto de acesso)
  WiFi.mode(WIFI_STA);
  // Desconecta o dispositivo de 
  WiFi.disconnect();
  delay(100);
  
  // Scannea todas a redes disponíveis para o ESP se conectar
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");

  if (n == 0)
    Serial.println("No networks found");
  else
  {
    Serial.print(n);
    Serial.println("Networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID e RSSI para cada rede encontrada
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }

    Serial.println("");

    st = "<ol>";
    for (int i = 0; i < n; ++i)
    {
      // Adiciona na variável o SSID e RSSI para cada rede encontrada
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
 
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
      st += "</li>";
    }

    st += "</ol>";
    delay(100);

  }
  
  // Cria Web Server
  createWebServer();

  // Iniciando ESP como Web Server 
  server.begin();
  Serial.println("Server started");
}

// Função responsável por criar Web Server utilizado para configurar o dispositivo
void createWebServer()
{
 {
    server.on("/", []() {

      // Conteúdo do formulário da página HTML 
      content = "<!DOCTYPE html> <html> <head> <title>Configurações</title>";
      content += "<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\">";
      content += "<style>body {background-color: #cccccc; font-family: Sans-Serif; Color: #000088;}</style> </head>";
      content += "<body> <h1>Configurações da Rede</h1> <form method='get' action='setting'>";
      content += "<p>Redes encontradas:</p>";
      content += st;
      content += "<label><b>Nome da rede:</b></label> <input type='text' name='ssid'/>";
      content += "<p> <label><b>Senha da rede:</b></label> <input type='text' name='pass'/> </p>";
      content += "<p> <label><b>Chave de acesso:</b></label> <input type='text' name='topic'/> </p>"; 
      content += "<p> <input type=submit name=botao value=Enviar /> </p>";
      content += "<p><strong>2020 - MonCoT - UFPR</strong></p> </form> </body> </html>";

      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      // Recuperando informações digitadas pelo usuário
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      String qtopic = server.arg("topic");
      
      // Status do HTML;
      int statusCode;

      if (qsid.length() > 0 && qpass.length() > 0 && qtopic.length() > 0) {
        
        Serial.println("clearing eeprom");
        // Limpa EEPROM
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }

        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
        Serial.println(qtopic);
        Serial.println("");

        Serial.println("writing eeprom ssid:");

        // Grava na EEPROM o SSID da rede
        for (uint i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }

        // Grava na EEPROM a Senha da rede
        Serial.println("writing eeprom pass:");
        for (uint i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }  
     
        // Grava na EEPROM o TOPIC do Device
        Serial.println("writing eeprom topic:");
        for (uint i = 0; i < qtopic.length(); ++i)
        {
          EEPROM.write(64 + i, qtopic[i]);
          Serial.print("Wrote: ");
          Serial.println(qtopic[i]);
        }

        // Comita as gravações
        EEPROM.commit();

        // Conteúdo da página HTML com a informações gravadas
        content = "<!DOCTYPE html> <html lang = \"pt-br\"> <head> <title>Config</title>";
        content += "<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\">";
        content += "<style>body {background-color: #cccccc; font-family: Sans-Serif; Color: #000088;}</style> </head>";
        content += "<body> <h1>Informações cadastradas:</h1>";
        content += "<p> <b>Rede:</b> ";
        content += server.arg("ssid");
        content += "<p> <b>Senha:</b> ";
        content += server.arg("pass");
        content += "<p> <b>Chave de acesso:</b> ";
        content += server.arg("topic");
        content += "</body> </html>";
        // Enviando HTML para o servidor
        server.send(200, "text/html", content);
        statusCode = 200;
        
        // Delay para atualizar HTML
        delay(3000);

        // Reseta ESP
        ESP.reset();
      }
      else 
      {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

    });
  } 
}

// Função responsável pela leitura do SSID da rede gravado na EEPROM
String readSSID(void){

  Serial.println("Reading EEPROM ssid");
  String esid;

  // Lendo EEPROM
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }

  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println();

  return esid;
}

// Função responsável pela leitura do PASSWORD da rede gravado na EEPROM
String readPASS(void){
  Serial.println("Reading EEPROM pass");

  String epass;
  // Lendo EEPROM
  for (int i = 32; i < 64; ++i)
  {
    epass += char(EEPROM.read(i));
  }

  Serial.print("PASS: ");
  Serial.println(epass);
  Serial.println();

  return epass;
}

// Função responsável pela leitura do TOKEN gravado na EEPROM
String readTOPIC(void){
  Serial.println("Reading EEPROM TOKEN");

  String etopic;

  // Lendo EEPROM
  for (int i = 64; i < 96; ++i)
  {
    etopic += char(EEPROM.read(i));
  }

  Serial.print("TOPIC: ");
  Serial.println(etopic);

  return etopic;
}

// Função responsável pela conexão Wi-Fi
void setupWiFi(const char* ssid, const char* pass){

  // Conecte-se a rede Wi-Fi
  WiFi.begin(ssid, pass);
  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Aguarde conexão a ser estabelecida
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(100);

    /*int lastTry = 0;

    if (millis() - lastTry < 10000) { 
      lastTry = millis();
    }
    else{
      Serial.println("Network not found");
      ESP.deepSleep(1 * 60000000);
    }*/
  }

  Serial.println("");
  Serial.println("WiFi connected");

}

// Função responsável por enviar leitura feita pelo DS18B20 para ThingsBoard
void sendToThingsBoard(float temperatura, String tensao){
  
  // Lendo TOKEN gravado na EEPROM
  String TOKEN = readTOPIC();

  Serial.println("making POST request");

  // Criando um JSON 
  String postData = "{";
  postData += "\"temperatura\":"; 
  postData += temperatura; 
  postData += ",";
  postData += "\"tensao\":"; 
  postData += tensao;
  postData += "}";

  // Criando a String com o caminho para envio
  String path = String("/api/v1/") + TOKEN.c_str() + "/telemetry";

  // Envia o JSON para o ThingsBoard usando o método POST via HTTP
  client.post(path, contentType, postData);

  // Leitura do status do código e corpo de resposta
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

}