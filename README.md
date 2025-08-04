#WiFi GPS Checker
Code made for a university subject project, it consists of a GPS Module and a ESP32 microcontroller with a integrated WiFi antenna. It checks if there are WiFi connections nearby.
If it detects one, it stores it, along with the GPS location it detected it (if available) and sends it via MQTT to a raspberry pi with node-red installed, to store it in a local database and show them in a grafana map.
Made in 2024
