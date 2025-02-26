### Estação de Captura de Áudio e Envio de Sinal

Este código implementa um **menu interativo** no **Raspberry Pi Pico**, utilizando um **display OLED SSD1306**, uma **matriz de LEDs WS2812**, um **microfone (ADC)**, botões e um **buzzer para SOS**.  

---

### **📌 Funcionalidades principais:**
1. **Menu de opções no display OLED:**  
   - O usuário pode escolher entre **Visualizador de Áudio** e **Modo SOS**.
   - A navegação é feita pelos botões **A** (modo áudio) e **B** (modo SOS).

2. **Visualizador de Áudio:**  
   - Captura o som via **microfone (GPIO 28, ADC2)**.  
   - Aplica um **filtro de ruído** para evitar detecção falsa.  
   - Mostra os níveis de som na **matriz de LEDs WS2812**.  
   - Sai do modo ao pressionar **Botão B**.

3. **Modo SOS:**  
   - Envia um **sinal Morse "SOS"** no **LED vermelho (GPIO 13)** e no **buzzer (GPIO 21)**.  
   - O usuário pode interromper o SOS pressionando **Botão A**.

---

### **🔍 Código**
1. **Definição de Hardware**  
   - **SSD1306 (I2C1, GPIO 14 e 15)**  
   - **Matriz WS2812 (PIO, GPIO 7)**  
   - **Microfone (ADC2, GPIO 28)**  
   - **Botões A e B (GPIOs 5 e 6, INPUT com pull-up)**  
   - **LED de SOS (GPIO 13, OUTPUT)**  
   - **Buzzer (GPIO 21, OUTPUT)**  

2. **Inicialização do Hardware**
   - **Configuração do I2C para o SSD1306.**
   - **Configuração dos botões como entrada com pull-up.**
   - **Configuração do LED SOS e do buzzer como saída.**
   - **Inicialização do ADC para leitura do microfone.**
   - **Configuração do PIO para controlar a matriz de LEDs.**

3. **Loop Principal (`while (true)`)**
   - **Exibe o menu** no SSD1306.  
   - **Lê os botões:**  
     - Se **Botão A** → inicia **Visualizador de Áudio**.  
     - Se **Botão B** → inicia **Modo SOS**.  

4. **Execução dos Modos**
   - **Visualizador de Áudio:**  
     - Lê o som do microfone (ADC).  
     - Filtra ruído com um **filtro exponencial (EMA)**.  
     - Exibe a intensidade do som na matriz de LEDs.  
     - Sai do modo ao pressionar **Botão B**.

   - **Modo SOS:**  
     - Envia o sinal Morse "SOS" com LED e buzzer.  
     - O usuário pode interromper pressionando **Botão A**.

---

### **🔧 Técnicas Importantes Utilizadas**
✅ **Uso de PIO** para controle eficiente da matriz de LEDs WS2812.  
✅ **Filtro Exponencial (EMA)** para remover ruído do microfone.  
✅ **Uso de interrupções (botões)** para alternância rápida entre modos.  
✅ **Uso de `ssd1306.h` e `font.h`** para exibição no OLED via I2C.  
✅ **Utilização de ADC (GPIO 28)** para captar o som ambiente.  
✅ **Envio do código Morse "SOS"** com controle de tempo e interrupção pelo usuário.  

---

### **📌 Resumo Final**
Este código cria um **sistema interativo** no Raspberry Pi Pico com **menu no OLED**, **visualização de áudio via matriz WS2812** e um **modo SOS com LED e buzzer**. Ele é bem estruturado, modularizado e otimizado para embarcados. 🚀
