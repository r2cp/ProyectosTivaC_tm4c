/*
 * main.c
 * Rodrigo Chang
 *
 * Programa de prueba para configurar las interrupciones por flanco y llevar una cuenta del numero de
 * veces que se interrumpe en un pin
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include "driverlib/gpio.h"

// Comentar para realizar la configuracion con la libreria de perifericos
#define __conRegistros__

// Variables glovales
unsigned long FallingEdges = 0;

/*
 * Configuracion de entrada en PF4, con pull-up debil interna, interrupciones en flanco de bajada y prioridad 5
 */
void EdgeCounter_Init(void){
#ifdef __conRegistros__
	// Configuracion utilizando registros
	SYSCTL_RCGC2_R |= 0x00000020; // (a) activate clock for port F
	FallingEdges = 0;             // (b) initialize count and wait for clock
	GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
	GPIO_PORTF_AFSEL_R &= ~0x10;  //     disable alt funct on PF4
	GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4
	GPIO_PORTF_PCTL_R &= ~0x000F0000; //  configure PF4 as GPIO
	GPIO_PORTF_AMSEL_R &= ~0x10;  //    disable analog functionality on PF4
	GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
	GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
	GPIO_PORTF_IBE_R &= ~0x10;    //     PF4 is not both edges
	GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event
	GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
	GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4
	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | 0x00A00000; // (g) priority 5
	NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
#else
	// Configuracion con la libreria
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	FallingEdges = 0;

	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
	GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
	IntEnable(INT_GPIOF);
	IntPrioritySet(INT_GPIOF, 5);
#endif

}

/*
 * Programa principal
 */
int main(void) {
	
	// Configurar el reloj a 40MHz
	SysCtlClockSet(SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN | SYSCTL_SYSDIV_5);

	// Configurar PF4 para detectar un flanco de bajada
	EdgeCounter_Init();

	// Configurar las interrupciones globales
	IntMasterEnable();

	for(;;) {
		// esperar interrupciones
	}

}

void IntGPIOPortF_Handler(void) {
	GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);

	// Quitar el rebote esperando 1ms
	SysCtlDelay(40000);
	if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) {
		FallingEdges += 1;
	}
}
