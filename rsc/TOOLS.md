
# ESP32 program flashing

[esptool](https://docs.espressif.com/projects/esptool/en/latest/esp32/index.html)
```pip install esptool```

```
python esptool.py --chip esp32s3 --port "COM4" --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0000 D:\proj\IoT\uMiner\.pio\build\uMiner\bootloader.bin 0x8000 D:\proj\IoT\uMiner\.pio\build\uMiner\partitions.bin 0xe000 C:\Users\chr\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin 0x10000 .pio\build\uMiner\firmware.bin
```

### Merge Binaries Into a Single File

```
esptool --chip esp32s3 merge_bin -o __flash\uMINER.bin --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0000 .pio\build\uMiner\bootloader.bin 0x8000 .pio\build\uMiner\partitions.bin 0xe000 %userprofile%\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin 0x10000 .pio\build\uMiner\firmware.bin
```

### Flash a Single File Image

```
esptool --port "COM4" write_flash 0x0000 __flash\uMINER.bin
```

