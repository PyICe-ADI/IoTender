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
$Revision: 1757 $
$Date: 2018-01-18 17:38:13 -0500 (Thu, 18 Jan 2018) $

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

Generated on: 2018-01-18
*/


/*! @file
 *  @ingroup LTC4162-SAD
 *  @brief LTC4162-SAD communication library core header file defining
 *  prototypes, data structures and constants used by LTC4162-SAD.c
 *
 *  Functions  matching  the  prototypes  of  @ref  smbus_write_register and @ref
 *  smbus_read_register  must  be  provided  to this API. They will implement the
 *  SMBus  read  and write transactions on your hardware. If the register size of
 *  the  LTC4162  is 8 bits, functions should be provided that implement SMBus
 *  read  byte and write byte. If the register size of the LTC4162 is 16 bits,
 *  functions  should  be provided that implement SMBus read word and write word.
 *  smbus_read_register  needs  to  store the value read from the LTC4162 into
 *  the  variable  data.  Both  functions  should return 0 on success and a non-0
 *  error  code  on  failure.  The  API functions will return your error codes on
 *  failure and a 0 on success.
 */

#ifndef LTC4162_H_
#define LTC4162_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "LTC4162-SAD_reg_defs.h"
#include "LTC4162-SAD_formats.h"
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

  // Type declarations

  /*! Incomplete declaration of struct containing any hardware-specific information to pass to read and write functions
      Complete definition if/as appropriate where smbus_read_register and smbus_write_register function definitions are completed. */
  struct port_configuration;

  /*! Prototype of user supplied SMBus write_word function. Should return 0 on success and a non-0 error code on failure. */
  typedef int (*smbus_write_register)(uint8_t address,              //!< Target IC's 7-bit SMBus address
                                      uint8_t command_code,         //!< Command code to be written to
                                      uint16_t data,                //!< Data to be written
                                      struct port_configuration *pc //!< Pointer to additional implementation-specific port configuration struct, if required.
                                     );
  /*! Prototype of user supplied SMBus read_word function. Should return 0 on success and a non-0 error code on failure. */
  typedef int (*smbus_read_register)(uint8_t address,              //!< Target IC's 7-bit SMBus address
                                     uint8_t command_code,         //!< command code to be read from
                                     uint16_t *data,               //!< Pointer to data destination
                                     struct port_configuration *pc //!< Pointer to additional implementation-specific port configuration struct, if required.
                                    );

  /*! Information required to access hardware SMBus port */
  typedef struct
  {
    uint8_t address; //!< Target IC's 7-bit SMBus address
    smbus_read_register read_register;             //!< Pointer to a user supplied smbus_read_register function
    smbus_write_register write_register;           //!< Pointer to a user supplied smbus_write_register function
    struct port_configuration *port_configuration; //!< Pointer to additional implementation-specific port configuration struct, if required.
  } LTC4162_chip_cfg_t;

  // function declarations
  /*! Function to modify a bit field within a register while preserving the unaddressed bit fields. Returns 0 on success. */
  int LTC4162_write_register(LTC4162_chip_cfg_t *chip, //!< Pointer to chip configuration struct
                             uint16_t registerinfo,    //!< Bit field name from LTC4162_regdefs.h
                             uint16_t data             //!< Data to be written
                            );
  /*! Retrieves a bit field data into *data. Right shifts the addressed portion down to the LSB position. Returns 0 on success. */
  int LTC4162_read_register(LTC4162_chip_cfg_t *chip, //!< Pointer to chip configuration struct
                            uint16_t registerinfo,    //!< Register name from LTC4162_regdefs.h
                            uint16_t *data            //!< Pointer to the data destination
                           );
  /*! Functions to modify LTC4162 power-on defaults. */
  void configure_LTC4162_reg(LTC4162_chip_cfg_t *); //!< Modify settings in LTC4162_reg_config.c
  void configure_LTC4162_bf(LTC4162_chip_cfg_t *);  //!< Modify settings in LTC4162_bf_config.c

#ifdef __cplusplus
}
#endif
#endif /* LTC4162_H_ */
