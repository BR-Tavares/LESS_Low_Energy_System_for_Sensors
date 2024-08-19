[Descrição Low Energy System for Sensors.pdf](https://github.com/user-attachments/files/16661920/Descricao.Low.Energy.System.for.Sensors.pdf)

DESCRIÇÃO DE SOFTWARE LESS - Low Energy System for Sensors
1.   Objetivo
O  Low Energy System for Sensors (LESS) é um software desenvolvido para ser instalado em módulos eletrônicos para aquisição de dados de sensores de umidade do solo. Foi desenvolvido com foco na eficiência energética para ser implementado em dispositivos alimentados por bateria. O programa prevê a transmissão dos dados por radiofrequência tornando-se independente do acesso à internet. 
2. Aplicação
O programa LESS proporciona uma gestão eficiente da energia, sendo ideal para uso em dispositivos eletrônicos instalados em ambientes remotos sem uso de energia solar e sem acesso à internet. Esta característica permite a ocultação do dispositivo, podendo ser posicionado dentro de florestas ou abaixo do solo. O software foi desenvolvido e testado em uma plataforma de desenvolvimento Wemos Lolin 32 Lite com um módulo RTC DS3231 (opcional) nos pinos SDA 12 e SCL 14 e um módulo LoRa Ebyte E32 (opcional) ligado aos pinos Rx e Tx. O LESS configura o microcontrolador para ativar e obter dados de 8 sensores, armazenar estes dados na memória interna e transmitir utilizando bluetooth e radiofrequência. Os pinos de saída {13, 15, 2, 0, 4, 5, 18, 23} são ligados ao Vcc de oito sensores de umidade do solo. Os pinos de entrada {26, 25, 33, 32, 35, 34, 39, 36} são ligados na saída analógica dos sensores.  Desta forma, a cada 30 minutos, o dispositivo irá transmitir por rádio frequência uma série de caracteres contendo o horário da leitura, a umidade do solo dos oito sensores, a tensão de referência e a descrição do dispositivo. 
3. Funcionalidades
Referência de tempo: O programa verifica a comunicação com um relógio externo (RTC), se não houver comunicação ele mantém o uso do relógio interno. O tempo pode ser ajustado via comunicação serial, seja pela configuração local ou pela resposta do receptor de radiofrequência.

Leitura de Sensores: O comando configPin() configura oito pinos como saída para ativar os módulos sensores individualmente e 8 pinos de entrada para realizar a leitura. A string leituras recebe a média de 5 leituras, juntamente o tempo epoch, a tensão de referência e uma informação sobre a aplicação do programa. A ativação dos sensores é sequencial, de forma que durante a leitura de um sensor os demais estejam desligados. Esta funcionalidade reduz a demanda de corrente da bateria e evita a interferência entre os sensores de umidade do solo.

Armazenamento de Dados: A função arquivo() armazena os dados em formato CSV na memória interna do ESP32 utilizando a função file.println() da biblioteca LittleFS. 

Transmissão: O programa prevê a transmissão dentro da função leituraDeSensores() utilizando o comando Serial2.println() que transmite a String leituras nos pinos Rx e Tx para um módulo externo opcional. 

Configuração e Controle: Se a função esp_sleep_get_wakeup_cause() identificar o acionamento por botão, o sistema ativa um temporizados para receber comandos pela serial para configuração remota do tempo de pausa e tempo epoch, assim como para baixar, deletar e sair do modo de comando. A função baixarDados(), a qual imprime os dados usando as funções Serial.println() e SerialBT.println().

Gestão de Energia: A função esp_deep_sleep_start() desabilita algumas funções do microcontrolador para economia de energia. O sistema é reiniciado ao final do temporizador ou se houver mudança de estado lógico do pino de despertar.

Interface de Usuário: Interface via aplicativo de comunicação serial para configuração e download de dados.

Bibliotecas: As funções de tempo, armazenamento e bluetooth utilizam bibliotecas de código provenientes do SDK (Software Development Kit) oficial da Espressif para dispositivos ESP32. As bibliotecas usadas são: Wire.h (Comunicação com módulo RTC externo), RTClib.h (Funções de formatação do tempo), BluetoothSerial.h (Comunicação bluetooth), LittleFS.h (Gerenciamento de arquivos), esp_bt.h (Ajuste de potência do bluetooth), ESP32Time.h (Simplifica as funções do RTC interno).

4. Fluxogramas: 
Função global (setup). 
O programa LESS inicializa a comunicação serial, o sistema de arquivos e a configuração dos pinos de entrada e saída. Em seguida identifica a causa da inicialização, se a causa for o botão, ativa a função de comandos, se for o alarme, ativa a função de leitura dos sensores. Tanto a função de comando quanto a função de leitura retornam para o sono profundo ao final da sua execução. O sistema reinicializa totalmente ao despertar do sono profundo, não sendo necessária a função loop.

Função de leitura dos sensores. 
A subrotina Leitura de Sensores percorre um array de inteiros correspondente aos pinos de entrada e saída do ESP32. A tensão nos pinos de saída é elevada a HIGH para ativar cada sensor individualmente. A leitura analógica do sensor de umidade do solo é feita no pino de entrada com cinco repetições. Uma leitura analógica é feita no pino 19, ligado a uma bateria extra de referência. Ao final é criado um termo do tipo string chamado “leituras”, contendo o tempo epoch, as leituras dos sensores, a tensão de referência e a descrição do dispositivo. Esta string é impressa na serial 1 e serial 2 e adicionada ao arquivo dados.CSV. 

Função de comando. 
No início da função de comando é estabelecido um temporizador de 60 segundos para que o usuário possa entrar com os comandos no aplicativo de comunicação serial. Se o comando recebido for um número ele será utilizado para ajuste do tempo, se for maior que 10.000 será considerado como entrada de tempo epoch, se for menor será considerado como intervalo entre leituras. Os demais comandos são: Download, del e exit.


