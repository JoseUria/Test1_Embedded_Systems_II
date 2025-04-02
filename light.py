from gpiozero import Button
import serial

# Configuración del puerto serie
ser = serial.Serial(port='/dev/ttyACM0', baudrate=115200, timeout=1)

# Configuración de los botones
button_25 = Button(26)  # Sumar contador
button_8 = Button(17)   # Restar contador
button_7 = Button(16)   # ON/OFF

# Contadores
count = 0  # Valor inicial del contador
state = False  # Estado de ON/OFF

# Funciones para manejar las pulsaciones de los botones
def button_25_pressed():
    global count
    if count < 1000:
        count += 100
        print(f"Botón en GPIO 26 presionado. Contador +100: {count}")
        ser.write(f"{count}\n".encode())  # Enviar solo el número por UART

def button_8_pressed():
    global count
    if count > 0:
        count -= 100
        print(f"Botón en GPIO 17 presionado. Contador -100: {count}")
        ser.write(f"{count}\n".encode())  # Enviar solo el número por UART

def button_7_pressed():
    global state
    state = not state  # Alterna entre ON y OFF
    estado_texto = "on" if state else "off"
    print(f"Botón en GPIO 16 presionado. Estado: {estado_texto}")
    ser.write(f"{estado_texto}\n".encode())  # Enviar solo el estado por UART

# Configurar los botones para detectar la pulsación
button_25.when_pressed = button_25_pressed
button_8.when_pressed = button_8_pressed
button_7.when_pressed = button_7_pressed

try:
    while True:
        pass  # El programa sigue ejecutándose esperando pulsaciones

except KeyboardInterrupt:
    print("Programa detenido por el usuario")
    ser.close()  # Cerrar el puerto serie

