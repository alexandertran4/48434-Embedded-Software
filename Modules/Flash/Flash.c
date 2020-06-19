/*!
** @file Flash.c
** @brief This module purpose is create routines for erasing and writing to the Flash
** @author by Alexander Tran 12610655 and Hubert Chau 12568681
** @date 2020-04-26
*/
/*!
**  @addtogroup Flash_Module Flash module documentation
**  @{
*/

//New types
#include "Types\types.h"
//MCU Board
#include "MK64F12.h"
//Flash functions
#include "Flash\Flash.h"
OS_ECB *Flash_Semaphore;

const int8_t FLASH_NUMBER_BYTES = 8;

typedef struct{

	uint8_t FCMD;
	union
	{
		struct				//Address structure
		{
			uint8_t b0, b1, b2, b3; 	// Flash addresses

		} bytes;

		uint32_t word;

	} address;

	uint8_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7;
} FCCOB_t;

FCCOB_t commandContent;

/*! @brief Sets the command object to the registers for the MCU to modify the flash
 * returns bool - TRUE if the command was successful
 */
static bool LaunchCommand(FCCOB_t * commonCommandObject)
{
	while ((FTFE_FSTAT_CCIF_MASK & FTFE->FSTAT) == 0)
		;

	if ((FTFE_FSTAT_ACCERR_MASK & FTFE->FSTAT) | (FTFE_FSTAT_FPVIOL_MASK & FTFE->FSTAT))
	{
			FTFE->FSTAT = 0x30;
	}

	//Write to the FCCOB registers
	FTFE->FCCOB0 = commonCommandObject->FCMD;
	FTFE->FCCOB1 = commonCommandObject->address.bytes.b2;
	FTFE->FCCOB2 = commonCommandObject->address.bytes.b1;
	FTFE->FCCOB3 = commonCommandObject->address.bytes.b0;

	//Writes to data bytes in array
	FTFE->FCCOB4 = commonCommandObject->byte7;
	FTFE->FCCOB5 = commonCommandObject->byte6;
	FTFE->FCCOB6 = commonCommandObject->byte5;
	FTFE->FCCOB7 = commonCommandObject->byte4;
	FTFE->FCCOB8 = commonCommandObject->byte3;
	FTFE->FCCOB9 = commonCommandObject->byte2;
	FTFE->FCCOBA = commonCommandObject->byte1;
	FTFE->FCCOBB = commonCommandObject->byte0;

	FTFE->FSTAT = FTFE_FSTAT_CCIF_MASK;

	while ((FTFE_FSTAT_CCIF_MASK & FTFE->FSTAT) == 0)
			//Waits for the command to execute for data integrity purposes.
			;

	return true;
}

/*! @brief Removes a sector of the flash
 * returns bool - TRUE if the sector is deleted.
 */

static bool EraseSector(const uint32_t address)
{
	FCCOB_t eraseObject;

	eraseObject.FCMD = 0x09;	//sets the object with the byte
	eraseObject.address.word = address;	//sets the address for launchCommand so it can be executed

	return LaunchCommand(&eraseObject);
}

bool Flash_Erase(void)
{
	return EraseSector(FLASH_DATA_START);
}

/*! @brief Writes a phrase to a register of the flash.
 * returns bool - TRUE if the phrase has been set to a register
 */
static bool WritePhrase(const uint32_t address, const uint64union_t phrase)
{
	FCCOB_t writeObject;
	uint32_t phraseAddress;

	writeObject.FCMD = 0x07;
	writeObject.address.word = phraseAddress;
	writeObject.byte0 = (uint8_t)(phrase.s.Lo >> 0);
	writeObject.byte1 = (uint8_t)(phrase.s.Lo >> 8);
	writeObject.byte2 = (uint8_t)(phrase.s.Lo >> 16);
	writeObject.byte3 = (uint8_t)(phrase.s.Lo >> 24);
	writeObject.byte4 = (uint8_t)(phrase.s.Hi >> 0);
	writeObject.byte5 = (uint8_t)(phrase.s.Hi >> 8);
	writeObject.byte6 = (uint8_t)(phrase.s.Hi >> 16);
	writeObject.byte7 = (uint8_t)(phrase.s.Hi >> 24);

	return LaunchCommand(&writeObject);
}

/*! @brief Erases the address and writes to an address in the flash
 * returns bool - TRUE if the command was successful and runs the WritePhrase function
 */

static bool ModifyPhrase(const uint32_t address, const uint64union_t phrase)
{
	if((EraseSector(FLASH_DATA_START)) == 0)
	{
		return false;
	}

	return WritePhrase(address, phrase);
}

/*! @brief Initialises the flash
 * returns bool - TRUE if the Flash was setup successfully
 */
bool Flash_Init(void)
{
	Flash_Semaphore = OS_SemaphoreCreate(1);
	return true;
}

/*! @brief Allocates space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *         The pointer will be allocated to a relevant address:
 *         If the variable is a byte, then any address.
 *         If the variable is a half-word, then an even address.
 *         If the variable is a word, then an address divisible by 4.
 *         This allows the resulting variable to be used with the relevant Flash_Write function which assumes a certain memory address.
 *         e.g. a 16-bit variable will be on an even address
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory. Valid values are 1, 2 and 4.
 *  @return bool - TRUE if the variable was allocated space in the Flash memory.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
	static bool FlashSectorsAllocated[8] = { false };
	uint32_t addressForVariable;

	if (size == 1 || size == 2 || size == 4) //check that only accepted data in inputted
	{
		for (int i = 0; i < FLASH_NUMBER_BYTES; i += size)
		{
			if (FlashSectorsAllocated[i] == 0x00) //check if the sector available
			{
				addressForVariable = FLASH_DATA_START + i; //calculate address will be assigned

				for (int x = 0; x < size; x++)
				{
					FlashSectorsAllocated[i] = 0x01; //block out remaining sectors that are no longer usable
				}

				*variable = (void*)addressForVariable; //assign pointer variable to allocated flash address
				return true;
			 }
		}
	}
	else
	{
		return false;
	}

	return true;
}

/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{
	static uint64union_t phraseToWrite;
	static uint32_t* phraseAddress;

	//Checks whether if the address provided is eligible
	if((uint32_t)address < FLASH_DATA_START || (uint32_t)address > FLASH_DATA_END)
	{
		return false;
	}

	phraseToWrite.l = _FW(address);

	if((uint32_t)address % 16 == 8)//Checks whether an address is high by checking if its equal an even number of 8
	{
		phraseAddress = (uint32_t*)((uint32_t)address - 8);
		phraseToWrite.s.Hi = data;
	}
	else
	{
		phraseAddress = (uint32_t*)address;
		phraseToWrite.s.Lo = data;
	}

	return ModifyPhrase((uint32_t)address, phraseToWrite); //modifies the phrase and sets it to 64 bits.
}

/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
	uint32union_t wordToWrite;
	uint32_t* wordAddress;

	//Checks whether if the address provided is eligible
	if ((uint32_t)address < FLASH_DATA_START || (uint32_t)address > FLASH_DATA_END) //check if the address is valid
	{
		return false;
	}

	wordToWrite.l = _FH(address); //sets the byte to *(uint16_t volatile *)

	if ((uint32_t)address % 4 == 2) //Checks whether an address is high by checking if its equal an even number of 2
	{
		wordAddress = (uint32_t*)((uint32_t)address - 2);
		wordToWrite.s.Hi = data;
	}
	else
	{
		wordAddress = (uint32_t*)address;
		wordToWrite.s.Lo = data;
	}
		// Writes the modified address to 32 bit
	return Flash_Write32(wordAddress, wordToWrite.l);
}

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{
	uint16union_t halfWordToWrite;
	uint16_t* halfWordAddress;

	//Checks whether if the address provided is eligible
	if ((uint32_t)address < FLASH_DATA_START || (uint32_t)address > FLASH_DATA_END) //check if the address is valid
	{
		return false;
	}

	halfWordToWrite.l = _FB(address); //sets the byte to *(uint8_t volatile *) address

	if ((uint32_t)address % 2 == 1) //Checks whether an address is high by checking if its equal an even number of 1
	{
		halfWordAddress = (uint16_t*)((uint32_t)address - 1);
		halfWordToWrite.s.Hi = data;
	}
	else
	{
		halfWordAddress = (uint16_t*)address;
		halfWordToWrite.s.Lo = data;
	}
		// Writes the modified address to 16 bit
	return Flash_Write16(halfWordAddress, halfWordToWrite.l);
}
