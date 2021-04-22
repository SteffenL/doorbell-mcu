# Doorbell MCU

## Building

Put `server.cert.pem` into `certs/`.

```sh
# Server is the full URL of the server, e.g. "https://myserver.local".
idf.py -DSERVER_URL=<server> reconfigure
# Run one or more commands: clean, build, flash, monitor
scripts/idf.sh [command...]
```
### WiFi Configuration

1. Power on the device and do a factory reset if needed.
2. Monitor the device log and look for the "proof of possession".
3. Use the ESP32 BLE Prov app to complete provisioning.
