# `picOS`

The Pico Operating System (`pico` + `OS` = `picOS`).

## Introduction

I always wanted to start development on an ARM based chip, and the rp2040 is the perfect start. Thank you @klaxxon for creating [rp2040os](https://github.com/klaxxon/rp2040os). With that project I wouldn't be able to learn that quickly about how the ARM cortex-m0+ model works.

The goal of this project is, to create a simple framework that allows for quick usage of both cores with different features of the scheduling process itself.

## Getting started

This project is based on my [rp2040-remote](https://github.com/oltdaniel/rp2040-remote) project. But you can decide youself how to run this.

#### Project setup

```bash
$ git clone https://github.com/oltdaniel/picos
$ cd picos
$ cd toolchains
$ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
$ tar -xf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
$ cd ..
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ make
```

## License

![GitHub](https://img.shields.io/github/license/oltdaniel/picos)

Feel free to create your own version of PicOS.