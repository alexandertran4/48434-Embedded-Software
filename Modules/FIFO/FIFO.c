/*!
** @file FIFO.c
** @brief Routines to implement a FIFO buffer.
**
** This contains the structure and "methods" for accessing a byte-wide FIFO.
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-05-11
*/
/*!
**  @addtogroup FIFO_Module FIFO module documentation
**  @{
*/

//new types
#include "Types\types.h"
//FIFO functions
#include "FIFO\FIFO.h"
#include "MK64F12.h"
#include "OS.h"

/*! @brief Sets up the FIFO implementation before first use
 *
 * @param fifo :: A pointer to the FIFO that needs initializing.
 * @return bool - TRUE if the FIFO was sucessfully initialised.
 */
bool FIFO_Init(TFIFO* const fifo)
{
	fifo->Start = 0;
	fifo->End = 0;
	fifo->ItemsAvailable = OS_SemaphoreCreate(0);
	fifo->SpaceAvailable = OS_SemaphoreCreate(FIFO_SIZE);
}

/*! @brief Put one character into the FIFO.
 *
 *  @param fifo A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @return bool - TRUE if data is successfully stored in the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Put(TFIFO* const fifo, const uint8_t data)
{
	OS_SemaphoreWait(fifo->SpaceAvailable, 0);
	OS_DisableInterrupts();

	fifo->Buffer[fifo->End] = data;

	if (fifo->End+1 == FIFO_SIZE)
	{
		fifo->End = 0;
	}

	fifo->End++;

	OS_SemaphoreSignal(fifo->ItemsAvailable);
	OS_EnableInterrupts();

	return true;
}

/*! @brief Get one character from the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @return bool - TRUE if data is successfully retrieved from the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Get(TFIFO* const fifo, uint8_t* const dataPtr)
{
	OS_SemaphoreWait(fifo->ItemsAvailable, 0);
	OS_DisableInterrupts();
	*dataPtr= fifo->Buffer[fifo->Start];

	if (fifo->Start+1 == FIFO_SIZE)
	{
	    fifo->Start = 0;
    }
	fifo->Start++;

	OS_SempahoreSignal(fifo->SpaceAvailable);
	OS_EnableInterrupts();

	return true;

}
