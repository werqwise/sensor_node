from flask import Flask, request, render_template, send_file, jsonify
import os
import paho.mqtt.client as mqtt
from threading import Lock

app = Flask(__name__)

# In-memory storage for devices and firmware versions
devices = {}
latest_firmware = {
    'version': '1.0.0',
    'filename': None
}

# Lock for thread safety
lock = Lock()

# MQTT Broker Configuration
MQTT_BROKER = 'mqtt.iot.werqwall.com'
MQTT_PORT = 1883
MQTT_USERNAME = 'mqtt_client'
MQTT_PASSWORD = 'mqtt_client'

# Initialize MQTT Client
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

# Function to handle incoming MQTT messages
def on_message(client, userdata, msg):
    topic = msg.topic
    if topic.startswith("SensorData/"):
        mac = topic.split("/")[1]
        with lock:
            devices[mac] = True
        print(f"Device detected via MQTT: {mac}")

# MQTT setup: connect, subscribe, and set the on_message callback
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.subscribe("SensorData/#")  # Subscribe to all device topics
mqtt_client.loop_start()

@app.route('/register', methods=['POST'])
def register():
    data = request.get_json()
    mac = data.get('mac')
    if mac:
        with lock:
            devices[mac] = True
        print(f"Device registered: {mac}")
        return 'Device registered', 200
    return 'Invalid data', 400

@app.route('/devices', methods=['GET'])
def get_devices():
    with lock:
        return jsonify(list(devices.keys())), 200

@app.route('/firmware', methods=['GET'])
def firmware():
    mac = request.args.get('mac')
    if not mac:
        return 'MAC address required', 400

    if mac not in devices:
        return 'Device not registered', 404

    if latest_firmware['filename'] is None:
        return '', 204  # No Content

    firmware_path = os.path.join('firmware', latest_firmware['filename'])
    if not os.path.exists(firmware_path):
        return 'Firmware file not found', 404

    return send_file(firmware_path, as_attachment=True)

@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        version = request.form.get('version')
        file = request.files.get('file')
        selected_macs = request.form.getlist('macs')

        if not version or not file or not selected_macs:
            return 'Version, firmware file, and device selection are required', 400

        filename = file.filename
        firmware_dir = 'firmware'
        if not os.path.exists(firmware_dir):
            os.makedirs(firmware_dir)
        firmware_path = os.path.join(firmware_dir, filename)
        file.save(firmware_path)

        latest_firmware['version'] = version
        latest_firmware['filename'] = filename

        # Publish "do_ota" to all selected devices
        for mac in selected_macs:
            topic = f"SensorData/{mac}/do_ota"
            mqtt_client.publish(topic, "do_ota")
            print(f"Published 'do_ota' to topic: {topic}")

        return 'Firmware uploaded and OTA update triggered', 200

    with lock:
        return render_template('index.html', devices=devices.keys())

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)