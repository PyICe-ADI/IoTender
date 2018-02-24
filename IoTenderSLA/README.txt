
LTC4162 C Library
==============================

Introduction:
-------------

This  C library includes headers and several functions to simplify communication
with  the  LTC4162.  This  low  level library provides a header file with the
definitions  of registers and bit fields within those registers, and routines to
read and write those registers and individual bit-fields.

Files:
------

LTC4162-SAD_reg_defs.h - Header file containing definitions of registers and bit
fields in the LTC4162.  Names and addresses of registers are defined here.
Names, addresses, sizes, offsets, and masks are defined for bit fields in the
LTC4162.  This information is also packed into a definition that is used by
LTC4162-SAD.c

LTC4162-SAD.c - Functions to initialize a LTC4162 instance, read and write
registers and bit-fields

LTC4162-SAD.h - Header file defining prototypes, data structures and constants
used by LTC4162-SAD.c

LTC4162-SAD_formats.h - File defining constants and macros that can be used by
LTC4162-SAD.c to convert real values to LTC4162 integer values at compile-time
(optimized away) and to convert LTC4162 integer values to real values at run-time
for UI display or debug (floating point arithmetic required).

LTC4162-SAD_pec.c - File containing bit-wise and table lookup CRC-8 functions to
compute SMBus Packet Error Check byte.

LTC4162-SAD_pec.h - File containing Packet Error Check function headers.

LTC4162-SAD_example_dummy.c - An example showing how to use the LTC4162.c
library. Dummy functions containing print statements are used in place of
hardware reads and writes.

LTC4162-SAD_example_linux.c - An example using the Linux kernel i2c-dev
interface to communicate with the LTC4162

LTC4162-SAD_bf_config.c - An example re-writing all LTC4162 writable
bit fields to their power-on default state. Can be used as a template to
quickly modify settings from their default.

LTC4162-SAD_reg_config.c - An example re-writing all LTC4162 writable
registers to their power-on default state. Can be used as a template to
quickly modify settings from their default. This program is smaller, but
less readable than LTC4162-SAD_bf_config.c.

LTC4162-SAD_example.ino - An example using the LT_SMBus Linduino library to
communicate with the LTC4162.  This example can also be easily adapted to
use other TWI/I2C/SMBus Arduino libraries. To build the Arduino project
successfully, the following files must be placed in a folder called
'LTC4162-SAD_example':
  LTC4162-SAD_example.ino
  LTC4162-SAD.c
  LTC4162-SAD.h
  LTC4162-SAD_formats.h
  LTC4162-SAD_reg_defs.h

Makefile - Makefile to build the examples.  Targets are dummy, linux and clean.
  "make dummy" builds LTC4162-SAD_example_dummy.c.
  "make linux" builds LTC4162-SAD_example_linux.c.
