/*! @file
 *
 *  @brief I/O routines for UART communications on the TWR-K70F120M.
 *
 *  This contains the functions for operating the UART (serial port).
 *
 *  @author PMcL
 *  @date 2015-07-23
 */

#ifndef UART_H
#define UART_H

// new types
#include "Types\types.h"
#include "OS.h"
extern TFIFO RxFIFO;
extern TFIFO TxFIFO;
extern OS_ECB *RxSemaphore;
extern OS_ECB *TxSemaphore;

typedef struct
{
  uint32_t baudRate;   /*!< The desired baud rate in bits/sec. */
  uint8_t rxPriority;  /*!< rxPriority is the receiver thread priority for the OS. */
  uint8_t txPriority;  /*!< txPriority is the transmitter thread priority for the OS. */
} UARTSetup_t;

/*! @brief Sets up the UART interface before first use.
 *
 *  @param moduleClk The module clock rate in Hz.
 *  @param uartSetup is a structure containing the parameters to be used in setting up the UART.
 *  @return true if the UART was successfully initialized.
 */
bool UART_Init(const uint32_t moduleClk, const UARTSetup_t* const uartSetup);
 
/*! @brief Get a character from the receive FIFO if it is not empty.
 *  Blocking implementation - if there is no character to get, the thread will be suspended
 *
 *  @param dataPtr A pointer to memory to store the retrieved byte.
 *  @return void
 *  @note blocking function - threads will be suspended if there is no character to get
 *  @note Assumes that UART_Init has been called.
 */
void UART_InChar(uint8_t* const dataPtr);
 
/*! @brief Put a byte in the transmit FIFO if it is not full.
 *  Blocking implementation - if there is no free space, the thread will be suspended
 *
 *  @param data The byte to be placed in the transmit FIFO.
 *  @return void
 *  @note blocking function - threads will be suspended until they are able to put a character
 *  @note Assumes that UART_Init has been called.
 */
void UART_OutChar(const uint8_t data);

/*! @brief Poll the UART status register to try and receive and/or transmit one character.
 *
 *  @return void
 *  @note Assumes that UART_Init has been called.
 */
void UART_Poll(void);

#endif
