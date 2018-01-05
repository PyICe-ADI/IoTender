/*!
LTC4162: Advanced Synchronous Switching Battery Charger and PowerPath Manager

@verbatim
The LTC®4162-S is an advanced synchronous switching battery charger and
PowerPath manager that seamlessly manages power distribution from input sources
such as wall adapters, backplanes, solar panels, etc. and a rechargeable
battery. A high resolution telemetry system provides extensive information on
circuit voltages, currents, battery resistance and temperatures which can all be
read back over the serial interface. The serial interface can also be used to
configure many of the charging parameters as well as numerous system status
alerts. The LTC4162-S can charge from one to four 6V Lead-Acid batteries or one
to two 12V Lead-Acid batteries with up to 3.2A of charge current. The power path
topology decouples the output voltage from the battery allowing a portable
product to start up under very low battery voltage conditions. The LTC4162-S is
available in an I²C adjustable version as well a fixed voltage version both with
and without MPPT enabled by default. The LTC4162-S is available in the 28-pin
4mm × 5mm × 0.75mm QFN surface mount package.
@endverbatim

http://www.linear.com/product/LTC4162

http://www.linear.com/product/LTC4162#demoboards

REVISION HISTORY
$Revision$
$Date$

Copyright (c) 2018, Linear Technology Corp.(LTC)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1.  Redistributions  of source code must retain the above copyright notice, this
    list  of conditions and the following disclaimer.

2.  Redistributions  in  binary  form must reproduce the above copyright notice,
    this  list of conditions and  the following disclaimer in the  documentation
    and/or other materials provided with the distribution.

THIS  SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY  EXPRESS  OR  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES   OF  MERCHANTABILITY  AND  FITNESS  FOR  A  PARTICULAR  PURPOSE  ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY  DIRECT,  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING,  BUT  NOT  LIMITED  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS  OF  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT  LIABILITY,  OR TORT
(INCLUDING  NEGLIGENCE  OR  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The  views and conclusions contained in the software and documentation are those
of  the authors and should not be interpreted as representing official policies,
either expressed or implied, of Linear Technology Corp.

The Linear Technology Linduino is not affiliated with the official Arduino team.
However,  the Linduino is only possible because of the Arduino team's commitment
to   the   open-source   community.   Please,  visit  http://www.arduino.cc  and
http://store.arduino.cc,  and consider  a purchase  that  will  help fund  their
ongoing work.

Generated on: 2018-01-02
*/


//! @defgroup LTC4162-SAD LTC4162: Advanced Synchronous Switching Battery Charger and PowerPath Manager

/*! @file
 *  @ingroup LTC4162-SAD
 *  @brief LTC4162-SAD lightweight, hardware ambiguous, embeddable C Communication
 *  Library.
 *
 * Communication  is  bit-field based as well as whole-register based. This library
 * automatically masks and right-justifies bit fields to  ease programmer workload.
 *
 * This C library provides a header file with the complete definitions of registers
 * and  bit  fields  within  those  registers, and routines to read and write those
 * registers and individual bit-fields.
 *
 * Outside  of  the  Linduino  environment,  this  library  can  be  built with any
 * standard  C  compiler  and  can  be  used with different I2C/SMBus communication
 * hardware  simply  by  swapping  out  the  pointers  to appropriate user-supplied
 * functions   *@ref   smbus_write_register   and   *@ref  smbus_read_register.  An
 * example  is  provided  using  the  i2c-tools  Linux kernel driver which has been
 * tested  on  the  BeagleBone  Black  Linux  single board computer. It can also be
 * readily  adapted  to  common  microcontrollers  with  minimal  memory  impact on
 * embedded systems.
 *
 * A higher level hardware ambiguous Python communication library is also available.
 *
 * Please   visit   http://www.linear.com/product/LTC4162#code  or  contact  the
 * factory at 408-432-1900 or www.linear.com for further information.
 */

#include "LTC4162-SAD.h"

#ifndef LTC4162_USE_MALLOC
int LTC4162_instances = 0;
LTC4162_chip_cfg_t LTC4162_chip_array[MAX_NUM_LTC4162_INSTANCES];
#endif


//private function
LTC4162 LTC4162_alloc(void)
{
  //! this function "allocates" a LTC4162_chip structure.
  //! It may or may not use malloc.
#ifdef LTC4162_USE_MALLOC
  return malloc(sizeof(LTC4162_chip_cfg_t));
#else
  if (LTC4162_instances < MAX_NUM_LTC4162_INSTANCES)
  {
    return &LTC4162_chip_array[LTC4162_instances++];
  }
  else
  {
    return 0;
  }
#endif
}

LTC4162 LTC4162_init(LTC4162_chip_cfg_t *cfg)
{
  LTC4162_chip_cfg_t *chip = LTC4162_alloc();
  if (chip == NULL) return NULL;
  chip->address = cfg->address;
  chip->write_register = cfg->write_register;
  chip->read_register = cfg->read_register;
  chip->port_configuration = cfg->port_configuration;
  return (LTC4162) chip;
}

static inline uint8_t get_size(uint16_t registerinfo)
{
  return ((registerinfo >> 8) & 0x0F) + 1;
}
static inline uint8_t get_subaddr(uint16_t registerinfo)
{
  return (registerinfo) & 0xFF;
}
static inline uint8_t get_offset(uint16_t registerinfo)
{
  return (registerinfo >> 12) & 0x0F;
}
static inline uint16_t get_mask(uint16_t registerinfo)
{
  uint16_t mask = 1 << get_offset(registerinfo);
  uint8_t size = get_size(registerinfo);
  uint8_t i;
  for (i=0; i<size-1; i++)
  {
    mask |= mask << 1;
  }
  return mask;
}

int LTC4162_write_register(LTC4162 chip_handle, uint16_t registerinfo, uint16_t data)
{
  LTC4162_chip_cfg_t *chip = (LTC4162_chip_cfg_t *) chip_handle;
  int failure;
  uint8_t command_code = get_subaddr(registerinfo);
  if (get_size(registerinfo) != 16)
  {
    uint8_t offset = get_offset(registerinfo);
    uint16_t mask = get_mask(registerinfo);
    uint16_t read_data;
    failure = chip->read_register(chip->address,command_code,&read_data,chip->port_configuration);
    if (failure) return failure;
    data = (read_data & ~mask) | (data << offset);
  }
  return chip->write_register(chip->address,command_code,data,chip->port_configuration);
}

int LTC4162_read_register(LTC4162 chip_handle, uint16_t registerinfo, uint16_t *data)
{
  LTC4162_chip_cfg_t *chip = (LTC4162_chip_cfg_t *) chip_handle;
  int result;
  uint8_t command_code = get_subaddr(registerinfo);
  uint8_t offset = get_offset(registerinfo);
  result = chip->read_register(chip->address,command_code,data,chip->port_configuration);
  if (get_size(registerinfo) == 16) return result;
  uint16_t mask = get_mask(registerinfo);
  *data &= mask;
  *data = *data >> offset;
  return result;
}
