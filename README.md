# Doorbell MCU

## Building

Put `server.cert.pem` into `certs/`.

```
docker build -t esp-idf .
docker volume create doorbell-mcu-source
docker run --rm --interactive --tty \
    --device /dev/ttyUSB0 \
    --mount "type=volume,source=doorbell-mcu-source,target=/work/source" \
    --name doorbell-mcu \
    esp-idf &
tar -c certs main CMakeLists.txt partitions.csv sdkconfig version.txt \
    | docker cp - doorbell-mcu:/work/source/
fg
idf.py build
idf.py flash -p /dev/ttyUSB0 -b 921600 && idf.py monitor -p /dev/ttyUSB0
```

### WiFi Configuration

1. Power on the device and do a factory reset if needed.
2. Monitor the device log and look for the "proof of possession".
3. Use the ESP32 BLE Prov app to complete provisioning.
