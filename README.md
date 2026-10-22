# Monitor solar — Waveshare ESP32-S3-Touch-LCD-7B

Primera versión de diagnóstico seguro. Proyecto PlatformIO para abrir en VS Code.

El firmware y el servidor web aún no están validados en el dispositivo.

## Uso

1. Alimentar la pantalla por USB.
2. Conectar el móvil a **SolarMonitor-7B** usando la contraseña configurada localmente.
3. Abrir **http://192.168.4.1**. API: `/api/status`.
4. Tocar la pantalla alterna entre información Wi-Fi y direcciones I²C.

Las medidas aparecen como `--` (JSON `null`). No hay datos simulados ni control de LEDs, relés o potencia. La prueba solo detecta direcciones del bus raíz; no identifica un INA por su dirección ni selecciona canales de un multiplexor desconocido. El escaneo se hace una vez al arrancar.

## Compilar y cargar

Antes de compilar, crea `src/local_config.h` copiando `src/local_config.example.h` y establece un valor propio para `SOLAR_AP_PASSWORD` (al menos 8 caracteres). El archivo local está excluido de Git.

En PowerShell, desde esta carpeta:

```powershell
$env:PYTHONIOENCODING='utf-8'
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -t upload --upload-port <PUERTO>
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor -p <PUERTO> -b 115200
```

En Windows, PlatformIO ejecuta esptool como módulo de Python para generar las imágenes de firmware. Usa el ejecutable de PlatformIO de su entorno (`.platformio\penv`): un `pio` de otra instalación de Python puede no tener esptool.

En VS Code también se pueden ejecutar las tareas PlatformIO disponibles desde `Terminal > Ejecutar tarea`. Sustituye `<PUERTO>` por el puerto serie de tu placa.

## Hardware y alcance

Configuración específica 7B: RGB 1024×600, flash QIO 16 MB, PSRAM OPI 8 MB. El identificador USB solo confirma Espressif, no el modelo de PCB. La variante 7B se toma del modelo indicado por el usuario y se contrasta con la documentación oficial y los recursos detectados del chip.

I²C: SDA GPIO8, SCL GPIO9; compartido por táctil, expansor y conector externo. Conservar los drivers originales de placa; no usar simultáneamente Wire sobre ese bus. Verificar orden de pines del conector y niveles de las resistencias pull-up antes de conectar un módulo externo. No conectar las baterías de 12 V al conector de batería de la pantalla: ese conector corresponde a una celda de litio de 3,7 V.

Pendiente para medidas reales: modelo de hub (distribuidor o multiplexor), canales y direcciones, presencia de cada INA, valor real de cada shunt, límites del módulo y orientación del cableado de medida. Sin estos datos no se calculan amperios, vatios, estado de carga ni balance solar. No modificar cableado de potencia con esta prueba.

## Procedencia

Drivers de pantalla, GT911, expansor y dibujo tomados del ejemplo oficial `08_TOUCH`, commit `c652c902db607f7ffb376257393cfd7657aa6428`; licencia incluida en `LICENSE.waveshare`.

- https://docs.waveshare.com/ESP32-S3-Touch-LCD-7B
- https://github.com/waveshareteam/ESP32-S3-Touch-LCD-7B

No añadas copias de flash ni configuraciones locales al control de versiones; pueden contener datos privados.
