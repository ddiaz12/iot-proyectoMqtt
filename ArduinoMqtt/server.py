from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os
from paho.mqtt import client as mqtt_client

# Estado inicial de los sensores y actuadores
ledState = False
smokeDetected = False
motionDetected = False
buzzerActive = False

# Configuración del servidor MQTT
mqtt_broker = "k8da39e9.ala.us-east-1.emqxsl.com"
mqtt_port = 8883
mqtt_user = "server"
mqtt_password = "password"
mqtt_topic_led = "led"
mqtt_topic_smoke = "smoke"
mqtt_topic_motion = "motion"
mqtt_topic_buzzer = "buzzer"
ca_cert_path = "emqxsl-ca.crt" 

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker with result code "+str(rc))
    client.subscribe(mqtt_topic_led)
    client.subscribe(mqtt_topic_buzzer)

def on_message(client, userdata, msg):
    global ledState, buzzerActive
    payload = msg.payload.decode()
    print(f"Received message from topic {msg.topic}: {payload}")

    if msg.topic == mqtt_topic_led:
        ledState = payload.lower() == "on"
    elif msg.topic == mqtt_topic_buzzer:
        buzzerActive = payload.lower() == "on"

class MyHTTPRequestHandler(BaseHTTPRequestHandler):

    def _set_response(self, content_type="text/plain", status_code=200):
        self.send_response(status_code)
        self.send_header("Content-type", content_type)
        self.end_headers()

    def do_GET(self):
        global ledState, smokeDetected, motionDetected, buzzerActive
        print(self.path)
        if self.path == "/":
            try:
                # Get the absolute path to the HTML file
                self._set_response(content_type="text/html")
                html_file_path = os.path.abspath("index.html")
                with open(html_file_path, "r", encoding="utf-8") as file_to_open:
                    # Write the HTML content to the response
                    self.wfile.write(file_to_open.read().encode())
            except Exception as e:
                print(f"Error: {e}")
                self.wfile.write(f"Error: {e}".encode())

        elif self.path == "/led":
            self._set_response("application/json")
            self.wfile.write(json.dumps({"status": ledState}).encode())
        
        elif self.path == "/smoke":
            self._set_response("application/json")
            self.wfile.write(json.dumps({"smokeDetected": smokeDetected}).encode())
        
        elif self.path == "/motion":
            self._set_response("application/json")
            self.wfile.write(json.dumps({"motionDetected": motionDetected}).encode())
        
        else:
            self._set_response("application/json", 500)
            self.wfile.write(json.dumps({"error": str()}).encode())

    def do_POST(self):
        content_length = int(self.headers["Content-Length"])
        post_data = self.rfile.read(content_length).decode()

        global ledState, smokeDetected, motionDetected, buzzerActive

        try:
            body_json = json.loads(post_data)
        except json.JSONDecodeError:
            self._set_response("application/json", 400)
            self.wfile.write(json.dumps({"message": "Invalid JSON"}).encode())
            return

        if self.path == "/smoke":
            smokeDetected = body_json.get("smokeDetected", False)

        if self.path == "/motion":
            motionDetected = body_json.get("motionDetected", False)

        if self.path == "/buzzer/on":
            buzzerActive = True
            self._set_response("application/json")
            self.wfile.write(json.dumps({"message": "Buzzer turned on"}).encode())
            client.publish(mqtt_topic_buzzer, "on")

        if self.path == "/buzzer/off":
            buzzerActive = False
            self._set_response("application/json")
            self.wfile.write(json.dumps({"message": "Buzzer turned off"}).encode())
            client.publish(mqtt_topic_buzzer, "off")

        self._set_response("application/json")
        self.wfile.write(json.dumps({"message": "Data updated successfully"}).encode())

def run_server(server_class=HTTPServer, handler_class=MyHTTPRequestHandler, port=7800):
    server_address = ("", port)
    httpd = server_class(server_address, handler_class)
    print(f"Starting server on port {port}...")
    httpd.serve_forever()

if __name__ == "__main__":
    client = mqtt_client.Client()
    client.username_pw_set(mqtt_user, password=mqtt_password)
    client.on_connect = on_connect
    client.on_message = on_message
    client.tls_set(ca_certs=ca_cert_path)
    client.connect(mqtt_broker, mqtt_port, keepalive=60)
    client.loop_start()
    run_server()
