# openevse-esp8266-emoncms
OpenEVSE posting to emoncms via ESP8266


Prerequisites
===========

esp-open-sdk: https://github.com/pfalcon/esp-open-sdk

## Debian wheezy

Install tools
```
$ sudo apt-get install make unrar-free autoconf automake libtool gcc g++ gperf flex bison texinfo gawk ncurses-dev libexpat-dev python python-serial sed git libtool-bin
```

Install esp-open-sdk

```
$ git clone --recursive https://github.com/pfalcon/esp-open-sdk.git
$ unset LD_LIBRARY_PATH
$ make STANDALONE=n
```

Set path to esp-open-sdk and esptool

```
$ export PATH=<path-to-esp-open-sdk>/xtensa-lx106-elf/bin:<path-to-esp-open-sdk>/esptool:$PATH
```

Building openevse-esp8266-emoncms
=================================

1. edit Makefile and set ESP_OPEN_SDK to point to path to your esp-open-sdk

2. create user/user_config.h and add:
```
#define SSID "YOUR SSID"
#define PASS "YOUR PASSPHRASE"
#define NODE "0"
#define PRIVATEKEY "YOUR EMONCMS APIKEY"
```

3. Build firmware

```
 $ make
```

4. Flash into ESP8266
```
 $ sudo make flash
```







