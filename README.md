# Doorbell MCU

## Building

```sh
# Set WiFi network SSID and passphrase
idf.py "-DWIFI_SSID=<ssid>" "-DWIFI_PASSPHRASE=<passphrase>" reconfigure
# Run one or more commands: clean, build, flash, monitor
scripts/idf.sh [command...]
```
