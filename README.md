<img src="https://github.com/user-attachments/assets/15600b18-f73b-4ba3-a959-47f0048a1ab6" alt="image2" width="50%">

# **Luz-Morse: Decodificador de Código Morse por Señales de Luz**

**Autores:**

| Apellido y Nombre | Padrón |
| :--- | :--- |
| Monforte, Camila Sol | 107193 |
| Spratte, Federico | 105694 |
| Morhell, Haidar Ali | 108576 |

**Fecha: 2do cuatrimestre 2025**


### **1 Objetivo y alcance del proyecto**
El objetivo de este trabajo es diseñar, implementar y validar un prototipo funcional (Producto Mínimo Viable) de un sistema embebido capaz de decodificar en tiempo real el código Morse transmitido a través de señales ópticas. 
Se espera que el sistema pueda interpretar secuencias de pulsos lumínicos (cortos y largos), traducirlos a caracteres alfanuméricos y enviarlos a una aplicación móvil para su visualización.
El proyecto abarcará desde el diseño del hardware y el circuito de acondicionamiento de la señal hasta el desarrollo del firmware en bare metal, siguiendo una arquitectura modular y orientada a eventos.

### **2 Análisis de alternativas técnicas**

Se evaluaron tres enfoques para un proyecto centrado en el procesamiento de señales, considerando su complejidad técnica, el hardware requerido y su alineación con los objetivos de aprendizaje del curso.

1. **Decodificador de Morse por Señales de Luz (Óptico).**  
   Utiliza un sensor de luz (LDR) para detectar los pulsos. El principal desafío técnico es el procesamiento de la señal analógica para discriminar los pulsos de la luz ambiental y la implementación de una máquina de estados precisa para la decodificación temporal. 

2. **Decodificador de Morse por Señales de Audio (Acústico).**  
   Utiliza un micrófono para capturar tonos de audio. Esta alternativa introduce una complejidad significativamente mayor en el procesamiento de la señal (requeriría filtrado de frecuencia, posiblemente un algoritmo de FFT), lo cual puede exceder el alcance de un sistema bare metal sin librerías de DSP dedicadas.

3. **Generador y Transmisor de Morse por Luz.**  
   Este proyecto se enfocaría en generar y transmitir una señal Morse precisa usando un LED. Aunque es interesante para practicar el control preciso de la temporización de salida, es menos complejo en cuanto al procesamiento de señales de entrada y la interacción con sensores analógicos.

#### **3 Selección de proyecto**

Se opta por el **Decodificador Óptico**. Esta alternativa ofrece el mejor equilibrio entre un desafío técnico relevante y la viabilidad de implementación. Permite abordar problemas clave: lectura de sensores analógicos, procesamiento de señales en tiempo real, gestión de temporizadores y máquinas de estado, todo con un hardware simple y accesible.

Los desafíos principales de este proyecto son:
1. **Implementación de una máquina de estados robusta:** Debe ser capaz de medir y clasificar correctamente las duraciones de los pulsos (puntos, rayas) y las pausas (entre símbolos, letras y palabras).
2. **Calibración y Adaptabilidad:** El sistema debe poder calibrarse para distinguir entre la señal de luz y la luz ambiental, adaptándose a diferentes condiciones de iluminación.
3. **Gestión precisa del tiempo:** El núcleo del sistema se basa en la medición precisa de milisegundos para interpretar el código correctamente.

#### **3.1 Arquitectura y Diseño del Sistema**

El sistema se estructura en los siguientes módulos principales, como se muestra en el diagrama de bloques.

<p align="center"> <img src="Diagrama de Bloques.png" width="400" > </p> 
<p align="center"><em>Figura 1: Diagrama en bloques del sistema</em></p>

1. **Módulo de Sensado**: Compuesto por un LDR en un divisor de tensión. Convierte la intensidad lumínica en una señal de tensión analógica.
2. **Unidad de Procesamiento (MCU)**: El microcontrolador, que ejecuta el firmware principal. Se encarga de leer el ADC, ejecutar la lógica de decodificación, gestionar los periféricos y la comunicación.
3. **Periféricos de Salida**: Incluye LEDs para feedback visual y un buzzer para notificaciones auditivas. Permiten al usuario conocer el estado del sistema.
4. **Módulo de Comunicación**: El módulo Bluetooth (HM-10) actúa como un puente serie inalámbrico para enviar los datos decodificados a una aplicación de terminal en un smartphone o PC.
5. **Módulo de Interacción y Configuración**: Botones y Dip Switches para la interacción del usuario, como iniciar una calibración o cambiar de modo de operación. La memoria **E2PROM I²C (o alternativamente Flash interna)** se usará para almacenar parámetros de forma persistente.

#### **4. Requisitos del sistema**

##### **4.1 Requisitos Funcionales**

| ID	| Requisito	 | Descripción |
| :--- | :--- |:--- |
| RF-01 | Decodificación de Pulsos | El sistema debe ser capaz de medir la duración de los pulsos de luz (estados "ON") y las pausas (estados "OFF"). |
| RF-02 | Clasificación Temporal | Debe clasificar los pulsos como "punto" o "raya" y las pausas como inter-símbolo, inter-letra o inter-palabra según los estándares del código Morse. |
| RF-03 | Traducción a Caracteres | Debe traducir las secuencias de puntos y rayas válidas a su correspondiente caracter alfanumérico. |
| RF-04 | Comunicación Externa | Los caracteres decodificados deben ser transmitidos vía Bluetooth a un dispositivo externo. |
| RF-05 | Modo de Calibración | Debe existir un modo (SET_UP) que permita al usuario calibrar el umbral de detección de luz para adaptarse a diferentes condiciones ambientales. |
| RF-06 | Persistencia de Configuración | El valor del umbral de calibración debe guardarse en memoria no volátil (E2PROM/Flash) para que persista entre reinicios. |
| RF-07 | Feedback al Usuario | El sistema debe proveer feedback visual (LEDs) y auditivo (Buzzer) sobre el estado de la decodificación. |

##### **4.2 Requisitos de Hardware**

| ID| Componente |Uso en el Proyecto |
| :--- | :--- |:--- |
|RH-01| Dip Switchs  |Para cambiar entre el modo NORMAL (operación) y un modo DEBUG que envíe mediciones de tiempo en crudo por Bluetooth. |
|RH-02| Buttons  | Para iniciar el modo de calibración (SET_UP) y confirmar los pasos de la misma. |
|RH-03| Leds  | Un LED para indicar el estado de la señal (luz detectada) y otro para indicar un evento (ej. letra decodificada). |
| RH-04| Buzzer  |Para emitir una notificación sonora al decodificar una letra o al producirse un error. |
| RH-05| Módulo HM-10  |Para la comunicación inalámbrica con una aplicación de terminal serie. |
| RH-06| Memoria E2PROM I²C (o Flash interna)  |Para almacenar el umbral de luz definido durante la calibración de forma persistente. |
|RH-07|Resistores |Actúan como divisores de tensión para acondicionar señales de datos y también protegen a algunos componentes frente a sobretensiones|
|RH-08|Tiras de Pines (Headers) |Permiten la conexión entre la placa de desarollo stm32 con la placa experimental|
|RH-09| Sensor Analógico  | Un LDR (Light-Dependent Resistor) será el sensor principal para medir la intensidad de la luz. |
|RH-10|Placa de desarrolo STM32 | Modulo principal del proyecto|
|RH-11|Placa experimental perforada |Actúa como shield de la placa de desarrollo, se integran los sensores y algunos de los actuadores.|
|RH-12|Alambre para Prototipado |Alambre de tipo Wire-Wrap (AWG 30) o alambre telefónico. Ideal para hacer las conexiones por debajo de la placa experimental.|

|Pantalla OLED I²C | Visualización local de símbolo/letra/estado. |
| Nota de alimentación | Regulador 5→3,3 V con capacitores según el datasheet (por ejemplo LM1117-3,3: 10 µF entrada + 10 µF salida + 100 nF; 7805: 0,33 µF entrada + 0,1 µF salida + bulk 10 µF). |
||

##### **4.3 Requisitos de Software y Arquitectura**

|ID | Requisito |Implementación en el Proyecto |
| :--- | :--- |:--- |
|RS-01| Bare Metal | El firmware se desarrollará sin el uso de un sistema operativo. |
|RS-02| Event-Triggered | La lógica principal se basará en eventos (flags temporizados y cambios de estado). |
|RS-03| Estructura Modular | El código se organizará en módulos (adc_light, morse_decode, ui/buzzer, bt_cmd, storage, app_control). |
|RS-04| Super-Loop < 1 ms | El bucle principal estará diseñado para ser no bloqueante, con tareas periódicas de sondeo y gestión de flags. |
| RS-05| Tick de 1 ms |Se configurará el Systick para generar una interrupción cada 1 ms, que servirá como base de tiempo para toda la lógica de temporización. |
|RS-06| Diagramas de Estado | El decodificador se modelará con una máquina de estados finitos (FSM) que gestionará los estados: IDLE, DETECTING_PULSE, DETECTING_PAUSE. |
|RS-07| Menú Interactivo | El modo SET_UP para la calibración funcionará como un menú simple guiado por LEDs y confirmado con un botón. |
|RS-08| Modos de Operación | NORMAL / SET_UP (CAL) / FALLA. |
|RS-09| Periféricos | I²C (EEPROM y, si aplica, OLED) y UART (HM-10); ADC para el sensor de luz. |

##### **4.4 Reglas de temporización y criterios de clasificación**

**Unidad de tiempo (Tᵤ).**  
En este proyecto **Tᵤ** es la **duración de un punto** del código Morse. La relación con la velocidad es:

$$
T_u\,[\text{ms}] = \frac{1200}{\text{WPM}}
$$

Donde *WPM* representa ''Palabras por minuto''. El valor constante 1200 surge de utilizar la palabra estándar **''Paris''** como referencia de calibración. Según la norma internacional, la palabra ''Paris'' equivale exactamente 50 unidades de tiempo ($50 \cdot T_u$). Por lo tanto, al dividir los 60.000ms de un minuto por las 50 unidades de la palabra patrón, se obtiene la constante 1200.

Valores por defecto y rango: **18–20 WPM** (≈ 67–60 ms). Configurable **10–25 WPM** (≈ 120–48 ms) vía Bluetooth; el Tᵤ estimado se **guarda en Flash/EEPROM**. También ofrecemos **auto-calibración**: se capturan unos puntos/rayas de prueba, se estima Tᵤ y se actualizan umbrales.

**Clasificación de pulsos (ON):**
- **Punto (·):** `dur_ON < 2·Tᵤ`  
- **Raya (–):** `dur_ON ≥ 2·Tᵤ`

**Clasificación de pausas (OFF):**
- **Separador interno (entre símbolos de una letra):** `dur_OFF < 1.5·Tᵤ`  
- **Fin de letra:** `1.5·Tᵤ ≤ dur_OFF < 4.5·Tᵤ`  
- **Espacio de palabra:** `dur_OFF ≥ 6.5·Tᵤ`

**Por qué esos márgenes.**  
Los valores teóricos (1·Tᵤ, 3·Tᵤ, 7·Tᵤ) se separan con **coeficientes 1.5 / 2 / 4.5 / 6.5** para tener **tolerancia** a jitter, tiempo de reacción humano y respuesta del sensor (LDR/OLED), evitando clasificar de manera ajustada.

**Realismo y usos.**  
Trabajar en **milisegundos** es lo natural para señales ópticas manuales (linterna/flash de celular, LED) y para un LDR, y cuadra con nuestro **tick de 1 ms**.  
- **Útil/realista:** ~**15–25 WPM** (Tᵤ ≈ 80–48 ms) → demostraciones, balizas simples, “SOS” con linterna, comunicación corta indoor/outdoor.  
- **Demasiado lento:** **<10 WPM** (Tᵤ > 120 ms) → mensajes largos se vuelven tediosos (igual decodifica).  
- **Demasiado rápido (manual):** **>25–30 WPM** (Tᵤ < 48–40 ms) exige mucha precisión humana y al LDR; con fuentes automáticas podría subirse ajustando filtros/umbrales.


#### **5. Casos de Uso Principales**

**Caso de Uso 1: Decodificar un mensaje**

* **Disparador**: El sistema detecta una transición de "oscuridad" a "luz" por encima del umbral calibrado.
* **Flujo Principal**: <br> 1. El sistema inicia un temporizador para medir la duración del pulso de luz. <br> 2. Al detectar el fin del pulso, clasifica su duración como "punto" o "raya". <br> 3. Inicia otro temporizador para medir la duración de la pausa. <br> 4. Al detectar el fin de la pausa, la clasifica y, si corresponde, finaliza una letra o palabra. <br> 5. La letra decodificada se envía por Bluetooth. El LED de estado parpadea y el buzzer suena. <br> 6. El sistema vuelve al estado de espera (IDLE).
* **Flujos Alternativos**: <br> a. Si una secuencia de pulsos no forma un caracter válido, se descarta y se notifica un error. <br> b. Si una pausa excede el tiempo de timeout, el sistema se resetea para esperar una nueva letra.
  
**Caso de Uso 2: Calibrar el sensor en un nuevo entorno**

* **Disparador**: El usuario presiona el botón "Calibrar" para entrar en modo SET_UP.
* **Flujo Principal**: <br> 1. El sistema indica (ej. con un LED parpadeando) que espera la medición de "oscuridad". <br> 2. El usuario presiona el botón, y el sistema promedia varias lecturas del ADC para obtener el nivel base. <br> 3. El sistema indica que espera la medición de "luz". <br> 4. El usuario ilumina el sensor y presiona el botón. El sistema promedia las lecturas para obtener el nivel de señal. <br> 5. Se calcula un umbral intermedio, se guarda en la E2PROM/Flash y se notifica la finalización exitosa.

#### **6. Plan de Medición y Análisis**

Para cumplir con los requisitos de análisis del curso, se llevarán a cabo las siguientes mediciones en el prototipo final:

* **Medición de Consumo**: Se medirá la corriente consumida por el circuito en diferentes modos de operación (IDLE, decodificando) utilizando un multímetro en serie con la alimentación.
* **Análisis de Tiempos de Ejecución (WCET)**: Se utilizará un pin de GPIO y un osciloscopio para medir el tiempo de ejecución de las rutinas críticas, como la interrupción del Systick y las funciones dentro del bucle principal, para asegurar que el sistema cumple con las restricciones de tiempo real.
* **Cálculo del Factor de Uso de la CPU (U)**: Con los valores de WCET ($C_i$) y los períodos de activación ($T_i$) de las tareas principales, se calculará el factor de uso para verificar que el sistema no está sobrecargado.

### 7. Cronograma (Gantt)

- 📄 Ver Gantt en PDF: [Diagrama_de_Gantt_TDSE.pdf](Diagrama_de_Gantt_TDSE.pdf)
- ⬇️ Descargar fuente (Excel): [Diagrama_de_Gantt_TDSE.xlsx](Diagrama_de_Gantt_TDSE.xlsx)

---

# Entrega final – Luz-Morse

Informe completo: [README-entrega-final.md](README-entrega-final.md)

Video: https://drive.google.com/file/d/1k0MoTVyF7XIBG3R9jAdmI5QU-V2r-khh/view?usp=sharing
