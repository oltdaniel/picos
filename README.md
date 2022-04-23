# `picOS`

The Pico Operating System (`pico` + `OS` = `picOS`).

## Introduction

I always wanted to start development on an ARM based chip, and the rp2040 is the perfect start. Thank you @klaxxon for creating [rp2040os](https://github.com/klaxxon/rp2040os). With that project I wouldn't be able to learn that quickly about how the ARM cortex-m0+ model works.

The goal of this project is, to create a simple framework that allows for quick usage of both cores with different features of the scheduling process itself.

## Getting started

#### Project setup

```bash
$ git clone https://github.com/oltdaniel/picos
$ cd picos
$ cd toolchains
$ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
$ tar -xf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
$ cd ..
```

#### Manual build

```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ make
```

## Example

> The current example can be found in [`src/main.c`](./src/main.c).

## References

- [**Raspberry Pi Pico Datasheet**](https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf)
- [**Raspberry Pi Pico C/C++ SDK**](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf)
- [**ARM® v6-M Architecture Reference Manual**](https://documentation-service.arm.com/static/5f8ff05ef86e16515cdbf826)

## License

![GitHub](https://img.shields.io/github/license/oltdaniel/picos)

Feel free to create your own version of PicOS.
