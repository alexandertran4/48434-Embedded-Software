/*!
** @file LEDs.c
** @brief
**
**		   Functions are created to configure the LED of the MCU
**         @Created by Alexander Tran 12610655 and Hubert Chau 12568681
**		   @date 2020-04-26
*/
/*!
**  @addtogroup LEDs_Module LED module documentation
**  @{
*/
//Clocks
#include "fsl_clock.h"
// GPIO ports
#include "fsl_gpio.h"
// PORTS
#include "fsl_port.h"
//LEDs
#include "LEDs\LEDs.h"
//MCU Board
#include "MK64F12.h"

//Port configuration from fsl_port.h library header
const port_pin_config_t LED_PORT_PIN_CONFIG =
{
		.pullSelect = kPORT_PullDisable,
		.slewRate = kPORT_SlowSlewRate,
		.passiveFilterEnable = kPORT_PassiveFilterDisable,
		.openDrainEnable = kPORT_OpenDrainDisable,
		.driveStrength = kPORT_LowDriveStrength,
		.mux = kPORT_MuxAsGpio,
		.lockRegister = kPORT_UnlockRegister
};

//LED GPIO configuration from fsl_gpio.h library header
const gpio_pin_config_t GPIO_LED_PIN_CONFIG =
{
	.pinDirection = kGPIO_DigitalOutput,
	.outputLogic = 1U
};

const uint32_t BOARD_LED_GREEN_GPIO_PIN = 26U; //Green LED GPIO Pin no.26 from schematic diagram
const uint32_t BOARD_LED_BLUE_GPIO_PIN = 21U; //Blue LED GPIO Pin no.21 from schematic diagram
const uint32_t BOARD_LED_RED_GPIO_PIN = 22U; //Red LED GPIO Pin no.22 from schematic diagram

/*! @brief Sets up the LEDs before first use.
 *
 *  @return bool - TRUE if the LEDs were successfully initialized.
 */
bool LEDs_Init(void)
{
	CLOCK_EnableClock(kCLOCK_PortB); //Enable Port B
	CLOCK_EnableClock(kCLOCK_PortE); //Enable Port E

	GPIO_PinInit(GPIOE, BOARD_LED_GREEN_GPIO_PIN, &GPIO_LED_PIN_CONFIG); //Initialise GPIOE with Green LED Pin and LED configuration
	GPIOE->PDDR |= LED_GREEN; //activating GPIO port to turn on Green LED

	GPIO_PinInit(GPIOB, BOARD_LED_BLUE_GPIO_PIN, &GPIO_LED_PIN_CONFIG); //Initialise GPIOB with Blue LED Pin and LED configuration
	GPIOB->PDDR |= LED_BLUE; //activating GPIO port to turn on Blue LED

	GPIO_PinInit(GPIOB, BOARD_LED_RED_GPIO_PIN, &GPIO_LED_PIN_CONFIG); //Initialise GPIOB with Red LED Pin and LED configuration
	GPIOB->PDDR |= LED_RED; //activating GPIO port to turn on Red LED

	PORT_SetPinConfig(PORTB, BOARD_LED_BLUE_GPIO_PIN, &LED_PORT_PIN_CONFIG); //Set Blue LED to Port B
	PORT_SetPinConfig(PORTB, BOARD_LED_RED_GPIO_PIN, &LED_PORT_PIN_CONFIG); //Set Red LED to Port B
	PORT_SetPinConfig(PORTE, BOARD_LED_GREEN_GPIO_PIN, &LED_PORT_PIN_CONFIG); //Set Green LED to Port E

	LEDs_Off(LED_GREEN | LED_BLUE | LED_RED);
	return true;
}

/*! @brief Turns an LED on.
 *
 *  @param color The color of the LED to turn on.
 *  @note Assumes that LEDs_Init has been called.
 */

void LEDs_On(const LED_t color)
{
	GPIOB->PCOR = color;
	GPIOE->PCOR = color;
}

/*! @brief Turns off an LED.
 *
 *  @param color THe color of the LED to turn off.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Off(const LED_t color)
{
	GPIOB->PSOR = color;
	GPIOE->PSOR = color;
}

/*! @brief Toggles an LED.
 *
 *  @param color THe color of the LED to toggle.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Toggle(const LED_t color)
{
	GPIOE->PTOR = color;
	GPIOB->PTOR = color;
}
