/*!
** @file UART.c
** @brief I/O routines for UART communications on the K64.
**
** This contains the functions for operating the UART (serial port)
**
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-05-11
*/
/*!
**  @addtogroup UART_module UART module documentation
**  @{
*/
#include "MK64F12.h"
#include "system_MK64F12.h"
#include "core_cm4.h"
#include "FIFO\FIFO.h"
// Serial Communication Interface
#include "UART\UART.h"
#include "Critical\critical.h"
//K64 module registers
#include "fsl_common.h"
//Port definitions
#include "fsl_port.h"
#include "fsl_clock.h"
#include "OS.h"

const port_pin_config_t UART_PORT_PIN_CONFIG =
{
	.pullSelect = kPORT_PullDisable,
	.slewRate = kPORT_SlowSlewRate,
	.passiveFilterEnable = kPORT_PassiveFilterDisable,
	.openDrainEnable = kPORT_OpenDrainDisable,
	.driveStrength = kPORT_LowDriveStrength,
	.mux = kPORT_MuxAlt3,
	.lockRegister = kPORT_UnlockRegister
};

OS_ERROR Error;

/*! @brief Sets up the UART interface before first use.
 *
 *  @param moduleClk The module clock rate in Hz.
 *  @param uartSetup is a structure containing the parameters to be used in setting up the UART.
 *  @return true if the UART was successfully initialized.
 */
bool UART_Init(const uint32_t moduleClk, const UARTSetup_t* const uartSetup)
{
	FIFO_Init(&TxFIFO);
	FIFO_Init(&RxFIFO);

	RxSemaphore = OS_SemaphoreCreate(0);
	TxSemaphore = OS_SemaphoreCreate(0);

	CLOCK_EnableClock(kCLOCK_Uart0);
	CLOCK_EnableClock(kCLOCK_PortB);

	PORT_SetPinConfig(PORTB, 16, &UART_PORT_PIN_CONFIG);
	PORT_SetPinConfig(PORTB, 17, &UART_PORT_PIN_CONFIG);

	static uint16_t setBaudRate;
	static uint16_t BaudRateFineAdjust;
	setBaudRate = moduleClk / (16 * uartSetup->baudRate);

	BaudRateFineAdjust = ((moduleClk * 32) / (16 * uartSetup->baudRate));

	if (setBaudRate > 8191)	//setBaudRate maxes out at 8191 as stated in datasheet
	{
		return false;
	}

	UART0->C2 &= ~UART_C2_TE_MASK;		//Turn off UART
	UART0->C2 &= ~UART_C2_RE_MASK;

	UART0->BDH = UART_BDH_SBR(setBaudRate >> 8);		//Shift answer to leave only 5 high bits
	UART0->BDL = setBaudRate;						//Set remaining bits of BDL register
	UART0->C4 |= UART_C4_BRFA(BaudRateFineAdjust);

	UART0->C2 |= UART_C2_TE_MASK;		//Turn on UART
	UART0->C2 |= UART_C2_RE_MASK;

	UART0->C2 |= UART_C2_RIE_MASK;		//Enable Interrupt
	UART0->C2 &= ~UART_C2_TIE_MASK;		//Disable Interrupt transmit enable

	NVIC_ClearPendingIRQ(UART0_RX_TX_IRQn);
	NVIC_EnableIRQ(UART0_RX_TX_IRQn);

	return true;
}
/*! @brief If FIFO is not empty, return a character from it.
 *
 *  @return TRUE if UART is processed successfully
 *          FALSE if UART is unsuccessfully processed
 */
bool UART_InChar(uint8_t* const dataPtr)
{
	return FIFO_Get(&RxFIFO, dataPtr);				//Place data provided in TxFIFO
}

/*! @brief if FIFO is not empty, insert data into it.
 *
 *  @return TRUE if UART is processed successfully
 *          FALSE if UART is unsuccessfully processed
 */

bool UART_OutChar(const uint8_t data)
{
	bool status = false;

	if (FIFO_Put(&TxFIFO, data))
	{
		UART0->C2 |= UART_C2_TIE_MASK;	//enable interrupt transmit
		status = true;
	}

	return status; //Place data provided in TxFIFO
}

void UART_Poll(void)
{
  if (UART0->S1 & UART_S1_RDRF_MASK)
  {
	  FIFO_Put(&RxFIFO, UART0->D);
  }

  if (UART0->S1 & UART_S1_TDRE_MASK)
  {
	  FIFO_Get(&TxFIFO, (uint8_t*)&UART0->D);
  }
}

/*! @brief Interrupt service routine for the UART.
 *
 *  @note Assumes the transmit and receive FIFOs have been initialized.
 */
void UART0_RX_TX_IRQHandler(void)
{
	OS_ISREnter();

	if (UART0->C2 & UART_C2_RIE_MASK)
	{
	   if (UART0->S1 & UART_S1_RDRF_MASK)
	   {
		   OS_SemaphoreSignal(RxSemaphore);
		   UART0->C2 &= ~UART_C2_RIE_MASK;
	   }
	}

	if (UART0->C2 & UART_C2_TIE_MASK)
	{
	   if (UART0->S1 & UART_S1_TDRE_MASK)
	   {
		   OS_SemaphoreSignal(TxSemaphore);
		   UART0->C2 &= ~UART_C2_TIE_MASK;
	   }
	}

	OSR_ISRExit();
}
