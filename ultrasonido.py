from gpiozero import DistanceSensor
from time import sleep
import serial
import os
import smtplib
from email.message import EmailMessage
from datetime import datetime

# Configuración del puerto serie
ser = serial.Serial(port='/dev/ttyACM0', baudrate=115200, timeout=1)

# Configuración del sensor
sensor = DistanceSensor(echo=24, trigger=23)

# Configuración del correo
SMTP_SERVER = "smtp.gmail.com"
SMTP_PORT = 587
EMAIL_SENDER = 'widows61@gmail.com'
EMAIL_PASSWORD = 'vgaguqvdfduuavqp'
EMAIL_RECEIVER = 'sej12599@gmail.com'
CONFIG_FILE = "config.txt"

# Función para enviar un correo
def enviar_correo(asunto, mensaje):
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    msg = EmailMessage()
    msg["Subject"] = asunto
    msg["From"] = EMAIL_SENDER
    msg["To"] = EMAIL_RECEIVER
    msg.set_content(f"{mensaje}\nFecha y hora: {now}")
    
    try:
        with smtplib.SMTP(SMTP_SERVER, SMTP_PORT) as server:
            server.starttls()
            server.login(EMAIL_SENDER, EMAIL_PASSWORD)
            server.send_message(msg)
        print(f"Correo enviado con éxito: {asunto}")
    except Exception as e:
        print(f"Error al enviar correo: {e}")

# Función para leer config.txt
def leer_config():
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r") as f:
            return f.read().strip()
    return ""

try:
    while True:
        distancia = sensor.distance * 100  # Convertir a cm
        print(f"Distancia: {distancia:.2f} cm")  # Mostrar distancia por consola
        
        if distancia <= 10:
            ser.write(b"alerta\n")  # Enviar solo 'ALERTA' por UART
            tiempo_espera = 0
            while tiempo_espera < 10:
                sleep(1)
                tiempo_espera += 1
                if leer_config().lower() == "safe":
                    ser.write(b"safe\n")  # Enviar 'SAFE' si está en config.txt
                    enviar_correo("Alerta detenida por usuario", "La alerta fue detenida manualmente.")
                    break
            else:
                ser.write(b"safe\n")  # Enviar 'SAFE' después de la pausa
                enviar_correo("Alerta activada por 10 segundos", "La alerta ha estado activa por más de 10 segundos sin detenerse.")
        else:
            ser.write(b"safe\n")  # Enviar solo 'SAFE' por UART
        
        sleep(0.5)

except KeyboardInterrupt:
    print("Medición detenida por el usuario")
    ser.close()  # Cerrar el puerto serie

