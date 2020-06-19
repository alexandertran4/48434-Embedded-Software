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
	//Creating semaphores to signal between threads
	fifo->ItemsAvailable = OS_SemaphoreCreate(0);
	fifo->SpaceAvailable = OS_SemaphoreCreate(FIFO_SIZE);

	return true;
}

/*! @brief Put one character into the FIFO.
 *
 *  @param fifo A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @return bool - TRUE if data is successfully stored in the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
void FIFO_Put(TFIFO* const fifo, const uint8_t data)
{
	OS_SemaphoreWait(fifo->SpaceAvailable, 0); //Check capacity of FIFO in order to put data in, if full, fail and return false
	OS_DisableInterrupts();

	fifo->Buffer[fifo->End] = data; //Put data in FIFO

	if (fifo->End+1 == FIFO_SIZE)
	{
		fifo->End = 0;  //Nearly out of space in FIFO
	}

	fifo->End++;

	OS_SemaphoreSignal(fifo->ItemsAvailable); //Increase number of items in FIFO
	OS_EnableInterrupts();
}

/*! @brief Get one character from the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @return bool - TRUE if data is successfully retrieved from the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
void FIFO_Get(TFIFO* const fifo, uint8_t* const dataPtr)
{
	OS_SemaphoreWait(fifo->ItemsAvailable, 0); //Wait for space to become available in FIFO
	OS_DisableInterrupts();
	//If data reading is successful, get data from FIFO buffer
	*dataPtr= fifo->Buffer[fifo->Start];
	//Decrease number of bytes in FIFO

	if (fifo->Start+1 == FIFO_SIZE)
	{
	    fifo->Start = 0;
    }
	fifo->Start++; //Data can be taken out the next available data
    OS_EnableInterrupts();

	OS_SemaphoreSignal(fifo->SpaceAvailable); //Increments number of spaces available in FIFO
}
