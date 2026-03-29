/**************************************************************************************************
  Filename:       hal_i2c.h
  Revised:        $Date: 2009-01-20 06:41:30 -0800 (Tue, 20 Jan 2009) $
  Revision:       $Revision: 18809 $

  Description:    Interface for I2C driver.

  Copyright 2006 - 2008 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

#ifndef HAL_I2C_H
#define HAL_I2C_H

/*********************************************************************
 * CONSTANTS
 *********************************************************************/

#define I2C_ERROR       1
#define I2C_SUCCESS     0

#define OCM_READ        0x01
#define OCM_WRITE       0x00

/*********************************************************************
 * FUNCTION PROTOTYPES
 *********************************************************************/

/*********************************************************************
 * @fn      HalI2CInit
 * @brief   Initializes two-wire serial I/O bus
 * @param   void
 * @return  void
 */
void HalI2CInit(void);

/*********************************************************************
 * @fn      HalI2CReceive
 * @brief   Receives data into a buffer from an I2C slave device
 * @param   address - address of the slave device
 * @param   buf - target array for read characters
 * @param   len - max number of characters to read
 * @return  zero when successful
 */
int8 HalI2CReceive(uint8 address, uint8 *buf, uint16 len);

/*********************************************************************
 * @fn      HalI2CSend
 * @brief   Sends buffer contents to an I2C slave device
 * @param   address - address of the slave device
 * @param   buf - ptr to buffered data to send
 * @param   len - number of bytes in buffer
 * @return  zero when successful
 */
int8 HalI2CSend(uint8 address, uint8 *buf, uint16 len);

/*********************************************************************
 * @fn      I2C_ReadMultByte
 * @brief   Reads multiple bytes from an I2C device register
 * @param   address - I2C device address
 * @param   reg - internal register address
 * @param   buffer - target array for read data
 * @param   len - number of bytes to read
 * @return  I2C_SUCCESS or I2C_ERROR
 */
int8 I2C_ReadMultByte(uint8 address, uint8 reg, uint8 *buffer, uint16 len);

/*********************************************************************
 * @fn      I2C_WriteMultByte
 * @brief   Writes multiple bytes to an I2C device register
 * @param   address - I2C device address
 * @param   reg - internal register address
 * @param   buffer - source array of data to write
 * @param   len - number of bytes to write
 * @return  I2C_SUCCESS or I2C_ERROR
 */
int8 I2C_WriteMultByte(uint8 address, uint8 reg, uint8 *buffer, uint16 len);

#endif /* HAL_I2C_H */