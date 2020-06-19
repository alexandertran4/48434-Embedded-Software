/*!
** @file packet.c
** @brief Routines to implement packet encoding and decoding for the serial port.
**
** This contains the functions for implementing the Simple Serial Communication Protocol.
**
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-05-11
*/
/*!
**  @addtogroup packet_module Packet module documentation
**  @{
*/

// Packet handling
#include "Packet\packet.h"
// MCU Board
#include "MK64F12.h"
// UART communication
#include "UART\UART.h"
//RTOS Library
#include "OS.h"

static OS_ECB *PacketPutSemaphore;
TPacket Packet;

uint8_t Byte_Checksum(const uint8_t byte1, const uint8_t byte2, const uint8_t byte3, const uint8_t byte4)
{
	static uint8_t temp;                           //variable to store checksum
	temp = byte1 ^ byte2 ^ byte3 ^ byte4;          //XOR all packets

	return temp;                                   //return checksum
}

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param moduleClk The module clock rate in Hz.
 *  @param baudRate The desired baud rate in bits/sec.
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t moduelClk, const UARTSetup_t* const uartSetup)
{
	PacketPutSemaphore = OS_SemaphoreCreate(1);

	return UART_Init(moduelClk, &uartSetup->baudRate);
}

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
void Packet_Get(void)
{
	static uint8_t packetPosition = 0;
	static uint8_t uartInData;

	UART_InChar(&uartInData);

	switch (packetPosition)
	{
		case 0:
		Packet_Command = uartInData;
		packetPosition++;
		return false;
		break;

		case 1:
		Packet_Parameter1 = uartInData;
		packetPosition++;
		return false;
		break;

		case 2:
		Packet_Parameter2 = uartInData;
		packetPosition++;
		return false;
		break;

		case 3:
		Packet_Parameter3 = uartInData;
		packetPosition++;
		return false;
		break;

		case 4:
		//Check to see whether the entire packet was received
		if (Byte_Checksum(Packet_Command, Packet_Parameter1, Packet_Parameter2,
				Packet_Parameter3) == Packet_Checksum)
		{
				packetPosition = 0;	//resets the packet position to its original for the next packet
				return true;		//true if the entire packet was successfully retrieved
		}
		else
		{
				//iterate the data throughout the packet structures
				Packet_Command = Packet_Parameter1;
				Packet_Parameter1 = Packet_Parameter2;
				Packet_Parameter2 = Packet_Parameter3;
				Packet_Parameter3 = Packet_Checksum;
				return false;
				break;

		default:
		packetPosition = 0;
		break;
		}
	}
}

/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
void Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
     OS_SemaphoreWait(PacketPutSemaphore, 0);

	 //output all the commands and parameters
	 UART_OutChar(command);
     UART_OutChar(parameter1);
	 UART_OutChar(parameter2);
     UART_OutChar(parameter3);
     UART_OutChar(Byte_Checksum(command, parameter1, parameter2, parameter3));

	 OS_SemaphoreSignal(PacketPutSemaphore);
}
