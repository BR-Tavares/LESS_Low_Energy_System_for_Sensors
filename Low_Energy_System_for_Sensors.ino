// Este código denominado LESS-Low_Energy_System_for_Sensors utiliza o ESP32 em configuração de baixa energia (deep sleep)
// para leiuras sequenciais de 8 sensores de umidade do solo, incluindo registro dos dados na memória e transmissão
// por radiofrequencia.

#include <Wire.h> // Comunicação com módulo RTC externo
#include <RTClib.h> // Funções de formatação do tempo
#include <BluetoothSerial.h> // Comunicação bluetooth
#include <LittleFS.h> // Gerenciamento de arquivos
#include <esp_bt.h> // Necessária para ajuste de potencia do bluetooth
#include <ESP32Time.h> // Simplifica as funções do RTC interno

#define uS_TO_S_FACTOR 1000000ULL
RTC_DATA_ATTR int TEMPO_PARA_DORMIR = 20; // Tempo para dormir em segundos
#define PINO_DE_ACORDAR GPIO_NUM_27 // Pino para acordar o ESP32
#define TEMPO_PARA_ESPERAR_SERIAL 120000 // Tempo de espera para comandos via serial (ms)
#define LED_INTERNO 22 // LED embutido (PNP LoRa)
#define PINO_REFERENCIA 19 // Pino de referência
#define SDA 12 // Pino SDA
#define SCL 14 // Pino SCL

int SAIDAS[8] = {13, 15, 2, 0, 4, 5, 18, 23}; // Pinos de saída
int ENTRADAS[8] = {26, 25, 33, 32, 35, 34, 39, 36}; // Pinos de entrada
const char DESCRICAO[] = "LESS 1.0, gpios 27,26,25,33,32,35,34,39 e Vcc no gpio 22 para módulo de transmissão";
const char IDENTIFICACAO[] = "LESS 1.0";

RTC_DATA_ATTR int contador = 0;
RTC_DS3231 rtcExterno;
ESP32Time rtcInterno;
bool usarRtcExterno = false;
BluetoothSerial SerialBT;

void Arquivos() {
    if (!LittleFS.begin()) {
        Serial.println("Erro ao montar o LittleFS. Tentando formatar...");
        if (LittleFS.format()) {
            Serial.println("LittleFS formatado com sucesso!");
        } else {
            Serial.println("Falha ao formatar LittleFS!");
            return;
        }
    }
}

void conf_Pinos() {
    for (int i = 0; i < sizeof(SAIDAS)/sizeof(SAIDAS[0]); i++) {
        pinMode(SAIDAS[i], OUTPUT);
        digitalWrite(SAIDAS[i], LOW);
    }
    for (int i = 0; i < sizeof(ENTRADAS)/sizeof(ENTRADAS[0]); i++) {
        pinMode(ENTRADAS[i], INPUT);
    }
    pinMode(PINO_DE_ACORDAR, INPUT_PULLDOWN);
    pinMode(LED_INTERNO, OUTPUT); 
    digitalWrite(LED_INTERNO, 0); delay(500); digitalWrite(LED_INTERNO, 1);
}

void RTC() {
    Wire.begin(SDA, SCL);
    Serial.println(DESCRICAO);
    if (rtcExterno.begin()) {
        usarRtcExterno = true;
        if (rtcExterno.lostPower()) {
            rtcExterno.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        // Atualiza o RTC interno com o tempo do RTC externo
        rtcInterno.setTime(rtcExterno.now().unixtime());
    } else {
        Serial.println("Não foi possível encontrar o RTC externo. Usando RTC interno.");
    }
}

void leituraDeSensores() {
    contador++;
    Serial.println("Leitura: " + String(contador));

    // Obtém o tempo atual do RTC interno
    unsigned long unix_time = rtcInterno.getEpoch();
    String leituras = String(unix_time);
    // Percorre os interruptores dos sensores, ligando, medindo, atribuindo valor a N[] e deligando cada sensor.
    int N[8];
    for (int i = 0; i < 8; i++) {
        digitalWrite(SAIDAS[i], HIGH);
        delay(100);

        int sum = 0;
        for (int j = 0; j < 5; j++) {
            sum += analogRead(ENTRADAS[i]);
            delay(10);
        }
        // Calcula a média de 5 leituras
        N[i] = sum / 5; 
        digitalWrite(SAIDAS[i], LOW);
        yield();
        leituras += "," + String(N[i]);
    }
    int REF = analogRead(PINO_REFERENCIA); //Bateria de referência para identificação de ruídos.
    leituras += "," + String(REF);
    leituras += ",";
    leituras += IDENTIFICACAO;

    File file = LittleFS.open("/dados.csv", "a");
    if (file) {
        file.println(leituras);
        file.close();
    } else {
        Serial.println("Erro ao abrir arquivo para escrita");
    }
    digitalWrite(LED_INTERNO, LOW); // Ativa módulo LoRa
    time_t t = rtcInterno.getEpoch(); time_t tlocal=t-10800;
    String hora_local = ctime(&tlocal);
    Serial.println(hora_local);
    Serial2.begin(115200, SERIAL_8N1, 16, 17); // Inicia Serial2 para comunicação LoRa conectado aos pinos 16 e 17.
    Serial.println("Dados: " + leituras);
    Serial2.println(leituras);
    Serial.flush();
    Serial2.flush();
    if (Serial2.available()) {
    String response = Serial2.readStringUntil('\n');
    if (eNumero(response)) {
        long novoTempo = response.toInt();
        if (novoTempo > 100000) {
            Serial.println("Comando recebido. Ajustando tempo epoch para " + response);
            rtcExterno.adjust(DateTime(novoTempo));
            rtcInterno.setTime(novoTempo); 
  }
    }
      }
    digitalWrite(LED_INTERNO, HIGH);
}

void baixarDados() {
    File file = LittleFS.open("/dados.csv", "r");
    if (!file) {
        Serial.println("Erro ao abrir arquivo para leitura");
        SerialBT.println("Erro ao abrir arquivo para leitura");
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        Serial.println(line);
        SerialBT.println(line);
    }
    file.close();
}

void deletarArquivo() {
    if (LittleFS.remove("/dados.csv")) {
        Serial.println("Arquivo deletado com sucesso");
    } else {
        Serial.println("Erro ao deletar arquivo");
    }
}

bool eNumero(const String &str) {
    for (char c : str) {
        if (!isDigit(c)) {
            return false;
        }
    }
    return true;
}

void tratarComando(String comando) {
    comando.trim();

    if (comando == "download") {
        Serial.println("Comando recebido. Enviando dados...");
        baixarDados();
    } else if (comando == "del") {
        Serial.println("Comando recebido. Deletando arquivo...");
        deletarArquivo();
    } else if (comando == "exit") {
        Serial.println("Saindo do modo de comando.");
        return;
    } else if (eNumero(comando)) {
        long novoTempo = comando.toInt();
        if (novoTempo > 10000) {
            Serial.println("Comando recebido. Ajustando tempo epoch para " + String(novoTempo));
            rtcExterno.adjust(DateTime(novoTempo));
            rtcInterno.setTime(novoTempo);
        } else if (novoTempo > 0) {
            Serial.println("Comando recebido. Ajustando tempo de sono para " + String(novoTempo) + " segundos.");
            TEMPO_PARA_DORMIR = novoTempo;
        }
    } else {
        Serial.println("Comando desconhecido.");
    }
}

void comandos() {
    SerialBT.begin("ESP32_BT");
    esp_bredr_tx_power_set(ESP_PWR_LVL_N12, ESP_PWR_LVL_N12);
    digitalWrite(LED_INTERNO, LOW);
    unsigned long startMillis = millis();
    Serial.println("Comandos: download, del, exit. Entradas: Tempo desligado e unixtime");
    SerialBT.println("Comandos: download, del, exit. Entradas: Tempo desligado e unixtime");
    while (millis() - startMillis < TEMPO_PARA_ESPERAR_SERIAL) {
        if (Serial.available() > 0) {
            String comando = Serial.readStringUntil('\n');
            tratarComando(comando);
        }
        if (SerialBT.available() > 0) {
            String comando = SerialBT.readStringUntil('\n');
            tratarComando(comando);
        }
    }
    digitalWrite(LED_INTERNO, HIGH);
}

void setup() {
    Serial.begin(115200);
    Arquivos();
    conf_Pinos();
    RTC();

    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    switch (wakeup_cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Acordado por pino externo (EXT0).");
            comandos();
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Acordado por timer.");
            leituraDeSensores();
            break;
        default:
            Serial.println("Acordado por outra razão.");
            break;
    }

    esp_sleep_enable_ext0_wakeup(PINO_DE_ACORDAR, 1);
    esp_sleep_enable_timer_wakeup(TEMPO_PARA_DORMIR * uS_TO_S_FACTOR);

    Serial.println("Entrando em sono profundo");
    delay(1000);
    esp_deep_sleep_start();
}

void loop() {
    // Loop vazio, pois tudo é tratado no setup
}
