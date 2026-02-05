<img src="https://github.com/user-attachments/assets/15600b18-f73b-4ba3-a959-47f0048a1ab6" alt="image2" width="50%">

# **Avances del proyecto : Luz-Morse - Decodificador de Código Morse por Señales de Luz**

**Autores:**

| Apellido y Nombre | Padrón |
| :--- | :--- |
| Monforte, Camila Sol | 107193 |
| Spratte, Federico | 105694 |
| Morhell, Haidar Ali | 108576 |

**Fecha: 2do cuatrimestre 2025**

El presente documento resume los avances alcanzados hasta la fecha e incluye tanto los requisitos funcionales como las decisiones de hardware, software y arquitectura adoptadas para la implementación.
Para facilitar el seguimiento del progreso, se utiliza un código de colores asociado al estado de cada requisito o módulo del sistema:

🟩 Verde — Implementado
🟨 Amarillo — En progreso
🟥 Rojo — Suspendido / En duda

Este esquema permite visualizar rápidamente qué partes del proyecto están completas, cuáles están en desarrollo y cuáles requieren revisión o decisión futura.

## **Requisitos Funcionales**

| ID	| Requisito	 | Descripción |Status | 
| :--- | :--- |:--- |:--- |
| RF-01 | Decodificación de Pulsos | El sistema debe ser capaz de medir la duración de los pulsos de luz (estados "ON") y las pausas (estados "OFF"). |🟩|
| RF-02 | Clasificación Temporal | Debe clasificar los pulsos como "punto" o "raya" y las pausas como inter-símbolo, inter-letra o inter-palabra según los estándares del código Morse. |🟩 |
| RF-03 | Traducción a Caracteres | Debe traducir las secuencias de puntos y rayas válidas a su correspondiente caracter alfanumérico. |🟩|
| RF-04 | Comunicación Externa | Los caracteres decodificados deben ser transmitidos vía Bluetooth a un dispositivo externo. |🟨|
| RF-05 | Modo de Calibración | Debe existir un modo (SET_UP) que permita al usuario calibrar el umbral de detección de luz para adaptarse a diferentes condiciones ambientales. |🟩 |
| RF-06 | Persistencia de Configuración | El valor del umbral de calibración debe guardarse en memoria no volátil (E2PROM/Flash) para que persista entre reinicios. |🟩|
| RF-07 | Feedback al Usuario | El sistema debe proveer feedback visual (LEDs) y auditivo (Buzzer) sobre el estado de la decodificación. |🟩|

## **Requisitos de Hardware**

| ID| Componente |Uso en el Proyecto |Status |
| :--- | :--- |:--- |:--- |
|RH-01| Dip Switchs  |Para cambiar entre el modo NORMAL (operación) y un modo DEBUG que envíe mediciones de tiempo en crudo por Bluetooth. |🟥|
|RH-02| Buttons  | Para iniciar el modo de calibración (SET_UP) y confirmar los pasos de la misma. |🟩|
|RH-03| Leds  | Un LED para indicar el estado de la señal (luz detectada) y otro para indicar un evento (ej. letra decodificada). |🟩|
| RH-04| Buzzer  |Para emitir una notificación sonora al decodificar una letra o al producirse un error. |🟩|
| RH-05| Módulo HM-10  |Para la comunicación inalámbrica con una aplicación de terminal serie. |🟨|
| RH-06| Memoria E2PROM I²C (o Flash interna)  |Para almacenar el umbral de luz definido durante la calibración de forma persistente. |🟩|
|RH-07|Resistores |Actúan como divisores de tensión para acondicionar señales de datos y también protegen a algunos componentes frente a sobretensiones|🟩|
|RH-08|Tiras de Pines (Headers) |Permiten la conexión entre la placa de desarollo stm32 con la placa experimental|🟩|
|RH-09| Sensor Analógico  | Un LDR (Light-Dependent Resistor) será el sensor principal para medir la intensidad de la luz. |🟩|
|RH-10|Placa de desarrolo STM32 | Modulo principal del proyecto|🟩|
|RH-11|Placa experimental perforada |Actúa como shield de la placa de desarrollo, se integran los sensores y algunos de los actuadores.|🟥|
|RH-12|Alambre para Prototipado |Alambre de tipo Wire-Wrap (AWG 30) o alambre telefónico. Ideal para hacer las conexiones por debajo de la placa experimental.|🟩|
|RH-13||Pantalla OLED I²C | Visualización local de símbolo/letra/estado. |🟥|


## **Requisitos de Software y Arquitectura**

|ID | Requisito |Implementación en el Proyecto |Status|
| :--- | :--- |:--- |:--- |
|RS-01| Bare Metal | El firmware se desarrollará sin el uso de un sistema operativo. |🟩|
|RS-02| Event-Triggered | La lógica principal se basará en eventos (flags temporizados y cambios de estado). |🟩|
|RS-03| Estructura Modular | El código se organizará en módulos (adc_light, morse_decode, ui/buzzer, bt_cmd, storage, app_control). |🟩|
|RS-04| Super-Loop < 1 ms | El bucle principal estará diseñado para ser no bloqueante, con tareas periódicas de sondeo y gestión de flags. |🟩|
| RS-05| Tick de 1 ms |Se configurará el Systick para generar una interrupción cada 1 ms, que servirá como base de tiempo para toda la lógica de temporización. |🟩|
|RS-06| Diagramas de Estado | El decodificador se modelará con una máquina de estados finitos (FSM) que gestionará los estados: IDLE, DETECTING_PULSE, DETECTING_PAUSE. |🟩|
|RS-07| Menú Interactivo | El modo SET_UP para la calibración funcionará como un menú simple guiado por LEDs y confirmado con un botón. |🟩|
|RS-08| Modos de Operación | NORMAL / SET_UP (CAL) / FALLA. |🟥|
|RS-09| Periféricos | I²C (EEPROM y, si aplica, OLED) y UART (HM-10); ADC para el sensor de luz. |🟨|


## **Conclusiones y Próximos Pasos**

- Integrar completamente el módulo HM-10 con la decodificación estable.  
- Finalizar el modo FALLA y completar los tests de robustez.  
- Montar y soldar la placa experimental perforada.  
- Evaluar si se mantiene o descarta el uso de la pantalla OLED.

_Última actualización: 06/02/2026_  

---

---

