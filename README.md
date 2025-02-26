# Monitoramento de Consumo de Energia personalizável
Esse projeto tem o objetivo de simular na placa Bitdoglab as funcionalidades centrais de um possível sistema embarcado.

## Funcionalidades
Esse código permite associar determinado valor a um nome, e a placa reagirá assim que o valor for encontrado novamente e exibirá no display o nome associado. Além disso, também apresenta um contador que armazena o valor convertido do joystick a cada segundo a vai amontoando-os.

## Componente utilizados
RP2040 (Placa BitDogLab)

Display SSD1306 OLED (I2C)

Joystick analógico

Botão A (GPIO 5)

Botão B (GPIO 6)

LED RGB (Pinos 11 e 13)

## Como configurar no VS Code
### 1 Instale o SDK do Raspberry Pi Pico

### 2 Clone o repositório

### 3 Configure o VSCode

### 4 Compile o projeto

### 5 Carregue o código no Pico
Conecte o RP2040 segurando o botão BOOTSEL e monte a unidade USB.
Copie o arquivo .uf2 gerado para a unidade.

## Como usar
Joystick -> Alterna os valores exibidos na tela conforme ele se move no eixo vertical.
Botão A -> Associa um nome ao valor que estiver sendo exibido na tela no momento em que for pressionado
Botão B -> Alterna o que está sendo exibido na tela para um contador que responde ao Joystick.
