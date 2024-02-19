#include <ArduinoJson.h>
#include <EspMQTTClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define DHTTYPE DHT22
uint8_t DHTPin = 18;
DHT dht(DHTPin, DHTTYPE);
#define rele 5
#define led_azul 17
#define led_verde 4
#define led_vermelho 16
#define led_roxo 2
#define botao 12
#define buzzer 14
#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_LINES 2
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

char temp[100];
char umid[100];
float temperatura;
float umidade;

bool sistema_ativo() {
  return true;
}

//Conexão da plataforma TagoIO com protocolo MQTT.
EspMQTTClient client(
  "Wokwi-GUEST",
  "",
  "mqtt.tago.io",
  "Default",
  "d678e0cc-c16f-44ba-94ba-6731cfdb9682",
  "espKuhn",
  1883
);

//Envio e funcionamento de dados para os botões da plataforma TagoIO.
void onConnectionEstablished() {
  Serial.println("Conectado");

  client.subscribe("info/led", [](const String &payload) {
    Serial.println(payload);
    if (payload == "L") {
      Serial.println("LIGAR LED");
      digitalWrite(led_roxo, HIGH);
    } else if (payload == "D") {
      Serial.println("DESLIGAR LED");
      digitalWrite(led_roxo, LOW);
    } else {
      Serial.println("ERRO");
    }
  });

  client.subscribe("info/rele", [](const String &payload) {
    Serial.println(payload);
    if (payload == "L") {
      Serial.println("LIGAR RELE");
      digitalWrite(rele, HIGH);
      delay(1500);
      digitalWrite(rele, LOW); // Desliga após 1,5 segundos
    } else if (payload == "D") {
      Serial.println("DESLIGAR RELE");
      digitalWrite(rele, LOW);
    } else {
      Serial.println("ERRO");
    }
  });

  client.publish("info/led", "payload");
  client.publish("info/rele", "acionamento");
}

void sistema_ligado() {
  if (sistema_ativo() == true) {
    digitalWrite(led_azul, HIGH);
  } else {
    digitalWrite(led_azul, LOW);
  }
}

void controle_temperatura() {
  if (temperatura < 27) {
    digitalWrite(led_vermelho, HIGH);
  } else {
    digitalWrite(led_vermelho, LOW);
  }
}

void movimento_servo() {
  static bool estado_botao = false;
  static Servo servo_botao;
  static int angulo_botao = 0;

  if (digitalRead(botao) == HIGH) {
    estado_botao = !estado_botao;
    if (estado_botao) {
      digitalWrite(led_verde, HIGH);
      servo_botao.attach(19);
      for (angulo_botao = 0; angulo_botao < 180; angulo_botao++) {
        servo_botao.write(angulo_botao);
        delay(5);
      }
    } else {
      digitalWrite(led_verde, LOW);
      servo_botao.write(0);
    }
  }
}

void enviar_dados_iot() {
  StaticJsonDocument<300> dadosT;
  StaticJsonDocument<300> dadosU;

  dadosT["variable"] = "dado_temp";
  dadosT["value"] = temperatura;

  dadosU["variable"] = "dado_umid";
  dadosU["value"] = umidade;

  serializeJson(dadosT, temp);
  serializeJson(dadosU, umid);

  client.publish("info/temp", temp);
  client.publish("info/umid", umid);
}

void leitura_sensor() {
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();
  Serial.print(temperatura);
  Serial.println("C");
  Serial.print(umidade);
  Serial.println("%");
}

void display_lcd() {
  lcd.setCursor(0, 0);
  lcd.print("Temperatura: ");
  lcd.print(temperatura);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Umidade: ");
  lcd.print(umidade);
  lcd.print("%");
}

void setup() {
  Serial.begin(115200);
  pinMode(led_verde, OUTPUT);
  pinMode(led_azul, OUTPUT);
  pinMode(led_roxo, OUTPUT);
  pinMode(led_vermelho, OUTPUT);
  pinMode(rele, OUTPUT);
  pinMode(botao, INPUT);
  lcd.init();
  lcd.backlight();
}

void loop() {
  sistema_ligado(); // Acionamento do LED para representar que está funcionando.
  controle_temperatura(); // Controle de temperatura, caso caia a temperatura irá acender o LED vermelho.
  movimento_servo(); // Função de acionamento do Servo motor com push button.
  display_lcd(); // Função de funcionamento do display.
  leitura_sensor(); // Função de leitura de temperatura e umidade do sensor DHT.
  enviar_dados_iot(); // Função de envio de dados para a plataforma TagoIO.
  delay(2000);

  client.loop();
}
