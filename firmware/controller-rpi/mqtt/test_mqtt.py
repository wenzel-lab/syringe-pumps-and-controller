import paho.mqtt.client as mqtt
import threading
import time
import random

# Configuración del broker MQTT
BROKER = "35.223.234.244"  # Cambia esto a la dirección de tu broker
PORT = 1883
TOPIC_SUBSCRIBE = "wl/sp/tx"#"esp32/test/up"
TOPIC_PUBLISH   = "wl/sp/rx"#"esp32/test/down"
USERNAME = "iowlabs"  # Cambia si tu broker requiere autenticación
PASSWORD = "!iow_woi!"
PUBLISH_INTERVAL = 15  # Intervalo en segundos para publicar datos



# Callback al conectarse al broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectado al broker MQTT.")
        # Suscribirse al tópico
        client.subscribe(TOPIC_SUBSCRIBE, qos=1)
        print(f"Suscrito al tema: {TOPIC_SUBSCRIBE}")
    else:
        print(f"Error de conexión. Código: {rc}")

# Callback al recibir un mensaje
def on_message(client, userdata, msg):
    print(f"Mensaje recibido en el tema '{msg.topic}': {msg.payload.decode()}")

# Publicador en segundo plano
def publish_data(client):
    for i in range(1, 11):  # Enviar 100 datos
        #random_value = random.randint(0, 100)
        #message = f"Dato {i}: {random_value}"
        message = '{\"id\":\"SPC-01\",\"cmd\":\"mov\",\"arg\":{\"ch\":2,\"ml\":500 }}'
        client.publish(TOPIC_PUBLISH, payload=message, qos=1)
        print(f"Publicado dato {i} :  {message} en {TOPIC_PUBLISH}")
		#trying with random intervals
        #time.sleep(random.randint(0,120))
        time.sleep(30)

# Configuración del cliente MQTT
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

if USERNAME and PASSWORD:
    client.username_pw_set(USERNAME, PASSWORD)

# Conectar al broker
client.connect(BROKER, PORT, keepalive=60)

# Iniciar un hilo para la publicación
publisher_thread = threading.Thread(target=publish_data, args=(client,))
publisher_thread.start()

# Iniciar el bucle para escuchar mensajes
client.loop_forever()
