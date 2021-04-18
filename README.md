# Doorbell MCU

## Building

Put `server.cert.pem` into `certs/`.

```sh
# Set WiFi network SSID and passphrase
# Server is the full URL of the server, e.g. "https://myserver.local".
idf.py -DWIFI_SSID=<ssid> -DWIFI_PASSPHRASE=<passphrase> -DSERVER_URL=<server> reconfigure
# Run one or more commands: clean, build, flash, monitor
scripts/idf.sh [command...]
```
