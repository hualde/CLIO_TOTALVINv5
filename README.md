Proyecto CLIO_TOTALVINv5
Este proyecto implementa un sistema de comunicación CAN para un vehículo utilizando el framework de Espressif (ESP-IDF) en un microcontrolador ESP32. El sistema permite leer y escribir tramas CAN, gestionar el VIN del vehículo, realizar calibraciones de ángulo, y borrar códigos de diagnóstico (DTC). Además, ofrece una interfaz web accesible mediante Wi-Fi para interactuar con el sistema.

Requisitos
ESP32 con soporte CAN.
Espressif ESP-IDF (v4.0 o superior).
Eclipse con el plugin CDT para la edición y compilación del proyecto.
Estructura del Proyecto
main/: Contiene el código fuente principal, incluyendo las tareas de comunicación CAN, manejo del VIN y la interfaz web.
can_communication.c: Gestiona el envío y recepción de tramas CAN.
vin_handling.c: Maneja el almacenamiento y validación del VIN del vehículo y de la columna.
dtc_handling.c: Implementa la tarea para borrar códigos de diagnóstico (DTC).
immo_handling.c: Maneja la programación del inmovilizador (IMMO).
wifi_server.c: Configura un servidor web para interactuar con el sistema a través de una red Wi-Fi.
CMakeLists.txt: Archivo de configuración para compilar el proyecto con CMake.
Configuración
GPIOs
TX GPIO (Transmisión CAN): GPIO 18
RX GPIO (Recepción CAN): GPIO 19
Estos pines pueden modificarse en config.h.

Configuración del VIN
El proyecto está diseñado para leer y validar el VIN del vehículo y de la columna. Si los VIN coinciden, el sistema permite continuar con las operaciones. De lo contrario, realiza acciones correctivas como reprogramar el VIN.

Calibración de Ángulo
El sistema incluye un proceso de calibración de ángulo que se puede ejecutar desde la interfaz web, guiando al usuario a través de pasos específicos para la calibración del volante del vehículo.

Interfaz Web
Configuración Wi-Fi
El sistema crea un punto de acceso Wi-Fi con las siguientes credenciales:

SSID: Lizarte Clio
Contraseña: RLizarteClio
Una vez conectado al punto de acceso, puedes acceder a la interfaz web en la dirección http://192.168.4.1 desde cualquier navegador web.

Funcionalidades del Servidor Web
El servidor web proporciona una interfaz sencilla para interactuar con el sistema CAN y realizar diversas operaciones, incluyendo:

Verificación del VIN: Muestra el VIN actual del vehículo y la columna.
Borrado de Códigos de Diagnóstico (DTC): Permite al usuario borrar los códigos de fallos almacenados en la ECU del vehículo.
Calibración de Ángulo: A través de un asistente en la web, guía al usuario paso a paso para calibrar el ángulo de la dirección.
Verificación del Estado: Realiza una verificación del estado actual de la calibración de ángulo y otros parámetros.
Endpoints del Servidor Web
/: Página principal donde se muestra el VIN del vehículo y opciones para realizar calibraciones o borrar fallos.
/clear_dtc: Inicia el proceso de borrado de códigos DTC.
/calibracion_angulo: Muestra las instrucciones y permite iniciar el proceso de calibración de ángulo.
/check_status: Verifica el estado actual del sistema.
/send_calibration_frames: Envía las tramas CAN necesarias para realizar la calibración de ángulo.
/change_language: Permite cambiar el idioma de la interfaz web (español, inglés, francés).
Soporte Multilenguaje
La interfaz web soporta tres idiomas: español (por defecto), inglés y francés. El idioma se puede cambiar desde la misma interfaz seleccionando el idioma en la esquina superior derecha.

Ejemplo de Flujo de Usuario
Acceso al servidor: Una vez conectado a la red Wi-Fi creada por el ESP32, accede a http://192.168.4.1.
Borrar fallos: Desde la página principal, puedes seleccionar la opción de "Borrar Fallos". Esto enviará una trama CAN para eliminar los DTC.
Calibración de ángulo: La opción "Calibración del ángulo" guía al usuario a través de los pasos necesarios para completar la calibración del volante.
Verificación del estado: La interfaz muestra el estado actual de la calibración y otras operaciones.

Acceso a la Interfaz Web: Una vez cargado el firmware, el ESP32 creará un punto de acceso Wi-Fi con el nombre Lizarte Clio y la contraseña RLizarteClio. Puedes conectarte a esta red y acceder a la interfaz web desde un navegador ingresando la dirección http://192.168.4.1.

Funcionalidades
Comunicación CAN: El ESP32 envía y recibe tramas CAN para interactuar con la ECU del vehículo y otros componentes.
Borrado de DTC: Permite borrar códigos de diagnóstico almacenados en la ECU.
Calibración de Ángulo: Proporciona una interfaz para calibrar el ángulo de la dirección del vehículo.
Manejo del VIN: Lee y compara el VIN del vehículo y la columna para verificar su correspondencia.
Servidor Web: Interfaz intuitiva y multilenguaje para realizar las operaciones de diagnóstico y calibración.
