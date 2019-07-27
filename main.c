/*
 * main.c
 *
 * Programa de configuracion de UART0 con el puerto virtual de la computadora para envio de datos y debugging
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

// headers para utilizar interrupciones de uart
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"

// leds para status
#define PF1		HWREG(GPIO_PORTF_BASE + 0x08)
#define PF2		HWREG(GPIO_PORTF_BASE + 0x10)

/*
 * Configura los leds en PF1 y PF2 para mostrar status
 */
void confLeds(void) {
	// Reloj al puerto F
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	// Poner como salidas
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2);
	// Apagar inicialmente
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2, 0xff);
}

/*
 * Programa principal
 */
int main(void) {
	// Configuracion de reloj a 40MHz
	SysCtlClockSet(SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_SYSDIV_5 | SYSCTL_XTAL_16MHZ);

	// Habilitacion de perifericos
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Configuracion de LEDs para status
	confLeds();
	PF2 = 0xf; // status

	// Configuracion de pines UART
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	// Configuracion del periferico
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

	// Configuracion de interrupciones, de recepcion de receive-timeout
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
	// Habilitar interrupcion del periferico
	IntEnable(INT_UART0);
	// Interrupciones globales
	IntMasterEnable();

	// Caracteres iniciales
	UARTCharPut(UART0_BASE, 'H');
	UARTCharPut(UART0_BASE, 'o');
	UARTCharPut(UART0_BASE, 'l');
	UARTCharPut(UART0_BASE, 'a');

	while (1) {
		// configuracion con interrupciones
		/*
		// hacer echo
		if (UARTCharsAvail(UART0_BASE)) {
			UARTCharPut(UART0_BASE, UARTCharGet(UART0_BASE));
		}
		*/
	}
}

void UART0_IntHandler(void) {
	// obtener el status de la interrupcion
	uint32_t status;
	status = UARTIntStatus(UART0_BASE, true);
	// borrar las interrupciones con el status obtenido
	UARTIntClear(UART0_BASE, status);

	// Manejar los caracteres mientras hayan
	while (UARTCharsAvail(UART0_BASE)) {
		// echo
		UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE));
		// hacer blinking al led 1ms
		PF1 = 1 << 1;
		SysCtlDelay(40000);  // para 1ms
		PF1 = 0 << 1;
	}
}
