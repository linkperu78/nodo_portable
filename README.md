# nodo_portable

- Este proyecto esta hecho para usar un ESP32 para acceder a dos diferentes Access Point (AP).
- En base a que AP el equipo logro conectarse, realizara GET() o POST() Request para HTTP/HTTPS
- Todos los datos a guardar/enviar por HTTP/s se almacenaran en una tarjeta SD

## Version
 - ESPIDF = 5.0
 - IDE = Visual Studio Code (No es importante este dato)

## BUG-UNFIXEDS
 - Cuando el buffer para la respuesta del POST() request no tiene la suficiente capacidad para la respuesta, el equipo no logra cerrar correctamnete la SD Card mediante SPI interface, lo que causa el reinicio del ESP32

## Autores: 
### Colaborador 1:
 - Participante: Diego Armando Quispe Cangalaya
 - Correo: diegoar.quispec@gmail.com
