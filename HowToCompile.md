# Complie G2core for dual Y-Axis setup
```
cd g2core
make PLATFORM=DUE BOARD=gShield SETTINGS_FILE=settings_shapeokoMAXdualY.h
```

# flash to arduino due
see also `https://github.com/synthetos/g2/wiki/Flashing-g2core-with-Linux`
```
stty -F /dev/ttyACM0 1200 hup
stty -F /dev/ttyACM0 9600

bossac  --port=ttyACM0  -U true -e -w -v -i -b -R ./bin/gShield/g2core.bin

```

