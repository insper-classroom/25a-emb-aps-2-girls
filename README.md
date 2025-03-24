# Controle Customizado para jogo de corrida de carros

## Jogo
O controle será desenvolvido para um jogo de corrida, provavelmente o *Need for Speed*. Ele será projetado para proporcionar uma experiência imersiva ao jogador.

## Ideia do Controle
A proposta do controle é ser um dispositivo de mesa com um volante baseado em acelerômetro, um botão liga/desliga e pedais para aceleração e frenagem que ficariam no chão, para dar a impressão de que o jogador está em um carro.

## Inputs e Outputs
### **Entradas (Inputs)**
- **Acelerômetro:** Para capturar os movimentos do volante.
- **Potenciômetros:**
  - **Pedal de aceleração**
  - **Pedal de freio**
- **Botão liga/desliga:** Para ativar ou desativar o controle.

### **Saídas (Outputs)**
- **Buzzer para feedback sonoro/vibratório.**

## Protocolo Utilizado
- **UART (Universal Asynchronous Receiver-Transmitter)** para comunicação entre o controle e o computador.
- **GPIO Interrupts** para capturar eventos dos botões e potenciômetros.

## Diagrama de Blocos Explicativo do Firmware
### **Estrutura Geral**
---

![Estrutura](diagrama.jpg)

---

#### **Principais Componentes do RTOS**
- **Tasks:**
  - Task de leitura das entradas (acelerômetro, potenciômetros, botão liga/desliga)
  - Task de envio de comandos via UART
  - Task de controle do buzzer para feedback sonoro

- **Filas:**
  - Fila de eventos de entrada
  - Fila de comandos para o jogo
  - Fila de feedback sonoro para o buzzer

- **Semáforos:**
  - Verificação do estado de conexão

- **Interrupts:**
  - Callbacks para os botões e potenciômetros

## Imagens do Controle
### **Proposta Inicial**
---

![Proposta](controle.png)

---
