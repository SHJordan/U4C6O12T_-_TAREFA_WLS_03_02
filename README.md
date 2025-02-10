
# Relatório de Funcionamento – Atividade do dia 03/02

https://github.com/user-attachments/assets/d4a55922-f098-4b6b-b0f1-a0f21776760d



https://github.com/user-attachments/assets/d896c3b0-a7fe-4300-9605-33c40a54c4bf



## Visão Geral

O firmware demonstrado integra várias funcionalidades:
- **LEDs RGB e Matriz WS2812**: Controle de uma matriz 5x5 de LEDs endereçáveis.
- **Display SSD1306**: Exibição de mensagens e informações.
- **Botões com Debounce**: Uso de botões para alternar o estado de LEDs e atualizar o display.
- **Comunicação via UART**: Recepção de caracteres que acionam a exibição de dígitos na matriz de LEDs.

## Estrutura do Código

### 1. Configuração de Pinos e Componentes

- **Pinos Definidos**:  
  - `PIN_WS2812` para a matriz de LEDs (GPIO 7).  
  - LEDs RGB individuais: verde (GPIO 12), vermelho (GPIO 13) e azul (GPIO 11).  
  - Botões A e B (GPIO 6 e 5) com pull-up interno.  
  - I2C para o display SSD1306 (SDA em GPIO 14 e SCL em GPIO 15).

- **Componentes Iniciais**:  
  O código inclui bibliotecas específicas para o ambiente do Raspberry Pi Pico, além de drivers para o display OLED e a matriz WS2812.

### 2. Inicialização e Configuração do Sistema

Na função `main()`, o firmware:
- Inicializa a comunicação padrão e aguarda alguns segundos para estabilização.
- Configura os pinos dos LEDs, botões e I2C.
- Inicializa o display SSD1306, limpando-o e exibindo uma mensagem inicial.
- Inicializa a matriz WS2812 (usando um PIO personalizado) e a limpa para deixar todos os LEDs apagados.

### 3. Controle de Botões e Atualização do Display

- **Debounce e Interrupções**:  
  A função `gpio_callback()` trata as interrupções dos botões A e B. Com um tempo de debounce de 200 ms, ela:
  - Alterna o estado dos LEDs (verde para o botão A e azul para o botão B).
  - Atualiza o display SSD1306 com mensagens indicando qual botão foi pressionado e se o LED foi ligado ou desligado.
  - Envia a mensagem via UART para monitoramento.

### 4. Controle da Matriz de LEDs WS2812

- **Funções Específicas**:
  - `ws2812_init()`: Inicializa a matriz com o uso do PIO, definindo a frequência e os parâmetros do protocolo WS2812.
  - `ws2812_set_pixel()`: Permite definir a cor de um LED específico na matriz, usando as coordenadas x e y.
  - `ws2812_show()`: Envia os dados de cor para cada LED na matriz, atualizando a exibição.
  - `ws2812_clear()`: Limpa a matriz, apagando todos os LEDs.

- **Exibição de Dígitos**:  
  A função `display_digit_on_matrix()` recebe um caractere (de '0' a '9') e desenha um padrão na matriz usando cores diferentes (verde, azul ou vermelho) para representar o dígito. Essa função é chamada quando o sistema recebe um caractere via UART.

### 5. Leitura UART e Loop Principal

No loop principal, o firmware:
- Verifica se há dados recebidos pela UART.  
- Ao detectar um dígito, atualiza o display para mostrar o número recebido e chama `display_digit_on_matrix()` para desenhar o padrão correspondente na matriz de LEDs.
- Mantém um pequeno delay para garantir estabilidade na leitura e na atualização dos dispositivos.

## Funcionamento no Mundo Real

Na prática, esse código permite que a BitDogLab:
- Demonstre a integração entre hardware e software. Por exemplo, ao pressionar os botões, os alunos observam a resposta imediata nos LEDs e no display, reforçando conceitos de interrupção e debounce.
- Mostre a interação com uma interface visual, onde os dígitos recebidos via UART se traduzem em padrões coloridos na matriz de LEDs, facilitando o entendimento da programação de displays e matrizes de LED.
- Seja uma excelente ferramenta didática para experimentar com comunicação serial, controle de LEDs e manipulação de displays OLED, tornando o aprendizado de eletrônica e programação mais interativo e prático.
