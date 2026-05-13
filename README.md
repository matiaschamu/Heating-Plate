# Alien Tech

Controlador electrónico para anafe / placa de calentamiento basado en **ESP32**, con control de temperatura por PID, control remoto vía WiFi, OTA y display de 7 segmentos.

---

## Tabla de contenidos

1. [Descripción general](#descripción-general)
2. [Especificaciones técnicas](#especificaciones-técnicas)
3. [Operación local](#operación-local)
   - [Botones y encoder](#botones-y-encoder)
   - [Modos de operación](#modos-de-operación)
   - [Significado de los LEDs](#significado-de-los-leds)
   - [Significado del display](#significado-del-display)
4. [Operación remota (WiFi + Web)](#operación-remota-wifi--web)
   - [Primera configuración](#primera-configuración)
   - [Uso normal](#uso-normal)
   - [Borrar la red guardada](#borrar-la-red-guardada)
   - [Endpoints HTTP (API)](#endpoints-http-api)
5. [Seguridad y comportamiento automático](#seguridad-y-comportamiento-automático)
6. [Arquitectura del firmware](#arquitectura-del-firmware)
7. [Compilación y carga](#compilación-y-carga)
8. [Solución de problemas](#solución-de-problemas)

---

## Descripción general

El equipo es una placa calefactora que puede operar en dos modos principales:

- **Modo Potencia** — entregás directamente potencia constante a la resistencia (100 W – 2000 W).
- **Modo Temperatura** — el equipo regula la potencia automáticamente para alcanzar una temperatura objetivo (0 °C – 650 °C) usando un **controlador PID**.

Cualquiera de los dos modos se puede combinar con un **timer de apagado automático** (5 segundos a 99:59 minutos).

El control se hace localmente desde dos botones y un encoder rotativo con un display de 4 dígitos 7-segmentos, o remotamente desde un navegador conectado a la misma red WiFi.

---

## Especificaciones técnicas

| Característica | Valor |
|---|---|
| Microcontrolador | ESP32 |
| Sensor de temperatura | Termocupla tipo K + MAX6675 |
| Rango de temperatura | 0 – 650 °C, paso 5 °C |
| Rango de potencia | 100 – 2000 W, paso 100 W |
| Control de la resistencia | Phase-angle con detección de cruce por cero (50 Hz) |
| Timer | 5 s – 99:59 minutos |
| Conectividad | WiFi 2.4 GHz (STA + AP) |
| OTA | ArduinoOTA por mDNS (`AlienTech.local`) |
| Display | 4 dígitos 7-segmentos multiplexados (~154 Hz) |
| Entradas físicas | 2 botones + encoder rotativo |
| Salidas físicas | TRIAC, ventilador, buzzer, 3 LEDs de estado |

---

## Operación local

### Botones y encoder

El equipo tiene **dos botones** y **un encoder rotativo**:

- **Botón POWER** — prende y apaga el equipo (entra y sale de OFF).
- **Botón FUNCIÓN** — cambia entre modos y entre vistas del display.
- **Ambos botones simultáneos** — activa o desactiva la salida (empieza a calentar / detiene el calentamiento).
- **Encoder rotativo** — ajusta el setpoint según el modo y la vista actual.

### Modos de operación

El equipo tiene 5 modos. Al prender pasa de `OFF` a `POWER`. Desde ahí, con el botón **FUNCIÓN** (cuando la salida está apagada) se cicla:

```
OFF  ⇌  POWER  →  POWER+TIMER  →  TEMP  →  TEMP+TIMER  →  POWER  →  …
```

Cada modo:

#### `OFF`
Equipo apagado. El display muestra `----` o, si la placa todavía está caliente (≥ 35 °C), una animación de enfriamiento. Con **FUNCIÓN** podés alternar entre la animación y la lectura real de temperatura durante el enfriamiento.

#### `POWER`
Modo de **potencia constante**. El encoder ajusta la potencia entregada a la resistencia entre 100 W y 2000 W (paso de 100 W). Con la salida activa, **FUNCIÓN** alterna entre mostrar el setpoint de potencia y la temperatura actual de la placa.

#### `POWER+TIMER`
Igual que `POWER` pero con cuenta atrás. El encoder edita el timer (subiendo de a 1 minuto, bajando de a 5 segundos). Con **FUNCIÓN** alternás entre la vista del timer (MM:SS) y la del setpoint de potencia. Cuando el timer llega a cero, la salida se apaga sola y suena un beep largo.

#### `TEMP`
Modo de **regulación por temperatura** con PID. El encoder ajusta el setpoint entre 0 °C y 650 °C (paso de 5 °C). El PID modula la potencia de la resistencia para mantener la placa cerca del setpoint.

#### `TEMP+TIMER`
Igual que `TEMP` pero con cuenta atrás de apagado automático.

### Significado de los LEDs

El equipo tiene **3 LEDs** que indican modo y estado:

| LED 1 (PWR) | LED 2 (TEMP) | LED 3 (TIMER) | Significado |
|:-:|:-:|:-:|---|
| ● | – | – | Modo `POWER` (fijo si la salida está apagada, destellando a 1 Hz si está activa) |
| – | ● | – | Modo `TEMP` |
| ● | – | ● | Modo `POWER+TIMER` |
| – | ● | ● | Modo `TEMP+TIMER` |
| – | – | – | Modo `OFF` |

Durante el arranque, los LEDs indican también el estado del WiFi (ver [Operación remota](#operación-remota-wifi--web)).

### Significado del display

| Lo que ves | Qué significa |
|---|---|
| `----` | Equipo en `OFF` y placa fría |
| Animación de tres segmentos rotando | Equipo en `OFF` con placa todavía caliente (≥ 35 °C) |
| Número 0–9999 | Setpoint o temperatura actual, según el modo y la vista |
| `MM:SS` con dos puntos | Timer (cuenta atrás si la salida está activa, valor configurado si no) |
| Número que cambia 4 veces al arrancar | Cada uno de los 4 octetos de la IP asignada por el router |

---

## Operación remota (WiFi + Web)

### Primera configuración

Cuando el equipo arranca por primera vez (o si las credenciales WiFi guardadas no funcionan), levanta una red WiFi propia llamada **`Alien Tech`** (sin contraseña). Los **3 LEDs quedan fijos** indicando "esperando configuración".

Pasos:

1. Desde el teléfono / notebook, conectate a la red WiFi **`Alien Tech`**.
2. Debería aparecer automáticamente un popup (captive portal) con un formulario. Si no aparece, abrí el browser y andá a cualquier URL — el equipo redirige todo a su página de configuración.
3. Cargá el **SSID** y la **contraseña** de tu red WiFi habitual y tocá **Aceptar**.
4. El equipo guarda las credenciales en memoria flash, se reinicia, y se conecta a tu red.

### Uso normal

Después de conectarse a tu WiFi, el equipo muestra su **IP** en el display, octeto por octeto (por ejemplo `192` → `168` → `0` → `73`). Anotala o usá `AlienTech.local` desde el browser (vía mDNS).

Abriendo `http://AlienTech.local/` (o `http://<IP>/`) accedés al **panel de control web**:

- **Temperatura actual** — se actualiza cada segundo
- **Modo** — selector con los 5 modos disponibles
- **Setpoint de temperatura** — input numérico (0–650 °C)
- **Setpoint de potencia** — input numérico (100–2000 W)
- **Timer** — input `MM:SS`
- **Botón Encender/Apagar** — equivalente a presionar los dos botones físicos
- **Botón Reiniciar equipo** — fuerza un reset del ESP32 (con confirmación)

Cualquier cambio que hagas en la web se aplica inmediatamente. Mientras estás editando un campo, el polling no te lo pisa por 2.5 segundos.

### Borrar la red guardada

Si cambiaste de red, o querés reconfigurar el WiFi:

> **Mantené presionados los dos botones (Power + Función) mientras conectás el equipo a la corriente.**

Los 3 LEDs quedan fijos como confirmación. Soltá los botones — el equipo borra las credenciales y levanta el AP `Alien Tech` para configurar una red nueva.

### Endpoints HTTP (API)

Útil para integraciones (Home Assistant, scripts, otros sistemas). Todos los `POST` reciben el valor en `application/x-www-form-urlencoded` con la clave `v`.

| Método | Ruta | Cuerpo | Descripción |
|---|---|---|---|
| `GET` | `/` | — | UI HTML de control |
| `GET` | `/api/state` | — | Estado actual en JSON |
| `POST` | `/api/mode` | `v=0..4` | Cambia el modo (`0=OFF, 1=POWER, 2=POWER+TIMER, 3=TEMP, 4=TEMP+TIMER`) |
| `POST` | `/api/output` | `v=0\|1` | Apaga (`0`) o enciende (`1`) la salida |
| `POST` | `/api/temp` | `v=0..650` | Setpoint de temperatura en °C |
| `POST` | `/api/power` | `v=100..2000` | Setpoint de potencia en W |
| `POST` | `/api/timer` | `v=5..5940` | Timer en segundos |
| `POST` | `/api/reset` | — | Reset del ESP32 |

Respuesta de `/api/state`:

```json
{
  "mode": 3,
  "outputOn": true,
  "powerWatts": 1000,
  "tempSetpoint": 180,
  "curTemp": 178.5,
  "timerSetSecs": 600,
  "timerSecs": 423,
  "resistorDuty": 47
}
```

---

## Seguridad y comportamiento automático

- **Ventilador** — se prende automáticamente con la salida y se mantiene activo hasta que la placa baja de **42 °C** (histéresis 42–47 °C). Esto enfría la placa después de apagar.
- **Detección de termocupla rota** — si el MAX6675 detecta termocupla abierta (cable cortado), el equipo retiene la última lectura válida en lugar de informar 0 °C, evitando que el PID dispare a 100 % por una falsa "temperatura baja".
- **PID con anti-windup** — la componente integral no acumula cuando la salida está saturada (0 % o 100 %), evitando sobreimpulsos al cambiar de setpoint.
- **Reset del PID al activar** — cada vez que se enciende la salida en modo TEMP, la integral y la derivada se reinician, garantizando un arranque limpio sin "memoria" de sesiones anteriores.
- **Apagado automático por timer** — en modos `*+TIMER`, al llegar la cuenta atrás a cero la salida se apaga sola y suena un beep largo.

---

## Arquitectura del firmware

El código está organizado en módulos C++ con responsabilidades bien definidas:

```
src/
├── main.cpp           Orquestación: máquina de estados, modos, display, encoder, teclado
├── controller.h       API pública del estado (consumida por la web)
│
├── adc.cpp/.h         Lectura de tensión de red (ADC interno)
├── temperatura.cpp/.h Lectura MAX6675 + control PID
├── Resistencia.cpp/.h Control TRIAC con cruce por cero
├── FanBuzzer.cpp/.h   Ventilador y buzzer (comparten pin)
├── display.cpp/.h     Driver 7-segmentos multiplexado por timer
├── encoder.cpp/.h     Encoder rotativo con debounce
├── keyboard.cpp/.h    Botones (incluye detección de "ambos pulsados al boot")
│
├── wifimgr.cpp/.h     Conexión WiFi, credenciales en NVS, modo AP, mostrar IP
├── web.cpp/.h         WebServer: captive portal (AP) + UI de control (STA)
└── ota.cpp/.h         ArduinoOTA (carga de firmware por aire)
```

### Flujo de arranque (resumen)

1. Inicializa display y teclado
2. ¿Ambos botones pulsados? → borrar credenciales WiFi
3. Intentar conectar al WiFi guardado (timeout 15 s)
   - **Éxito** → mostrar IP en display → iniciar OTA → iniciar UI de control en `:80`
   - **Fallo** → levantar AP `Alien Tech` → mostrar IP del AP → iniciar captive portal
4. Inicializar el resto del hardware (encoder, fan, resistencia, ADC, sensor)
5. Loop principal

### PID — ganancias iniciales

Definidas en [src/temperatura.cpp](src/temperatura.cpp):

```cpp
#define PID_KP_DEFAULT   3.0f   // % por °C de error
#define PID_KI_DEFAULT   0.08f  // % por °C·s acumulados
#define PID_KD_DEFAULT   8.0f   // % por °C/s de variación
```

Estas son valores razonables para arrancar. Se pueden modificar directamente o ajustar en tiempo de ejecución llamando `temperaturaPidSetTunings(kp, ki, kd)`. Si querés afinarlas con tu placa real, **Ziegler-Nichols** o **Cohen-Coon** son métodos clásicos para estimar valores empíricamente.

---

## Compilación y carga

El proyecto usa **PlatformIO**. Hay dos entornos definidos en `platformio.ini`:

### Compilar y cargar por USB
```
pio run -e esp32dev -t upload
```

### Cargar por WiFi (OTA)
```
pio run -e esp32dev_ota -t upload
```

Requisitos para OTA:
- El equipo tiene que estar **conectado a tu WiFi** (no en modo AP).
- Tu PC tiene que poder resolver `AlienTech.local` por **mDNS** (Bonjour en Windows / Avahi en Linux / nativo en Mac).
- El puerto UDP 5353 no puede estar bloqueado por el firewall.

> **Nota:** la primera vez después de cambiar de red, el IP puede cambiar pero `AlienTech.local` sigue funcionando. Si por algún motivo mDNS no resuelve, podés cambiar `upload_port` en `platformio.ini` por la IP que muestre el display al arrancar.

---

## Solución de problemas

### El equipo arrancó pero no aparece en la red
Mirá los LEDs:
- **LED 1 destellando lento** → todavía está intentando conectar al WiFi guardado.
- **Los 3 LEDs fijos** → no logró conectar y levantó el AP `Alien Tech`. Conectate ahí para reconfigurar.

### Olvidé en qué red está
Hacé el procedimiento de [borrar la red guardada](#borrar-la-red-guardada): mantené los dos botones presionados al alimentarlo.

### `AlienTech.local` no resuelve
- En Windows, asegurate de tener instalado el **Bonjour Print Services** (incluido con iTunes) o que el servicio de descubrimiento mDNS esté activo.
- Si no querés depender de mDNS, anotá la IP que el equipo muestra en el display al arrancar.

### El PID oscila o anda lento
Los valores por defecto son conservadores. Ajustá las ganancias en [src/temperatura.cpp](src/temperatura.cpp) según el comportamiento térmico de tu placa específica:
- **Oscilación** → bajá `Kp` o subí `Kd`.
- **Convergencia lenta a setpoint** → subí `Ki`.
- **Sobreimpulso** → subí `Kd` o bajá `Kp`.

### La temperatura marca 0 y no sube
Probablemente la termocupla está desconectada o rota. El MAX6675 detecta esto y el firmware retiene el último valor válido. Verificá el cableado de la termocupla a los pines del MAX6675.

### El ventilador no se apaga después de apagar el equipo
Es el comportamiento esperado: el ventilador sigue funcionando hasta que la placa baja de 42 °C (medido por la termocupla). Esto protege la electrónica del calor residual.
