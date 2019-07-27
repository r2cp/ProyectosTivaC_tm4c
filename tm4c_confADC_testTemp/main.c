/*
 * main.c
 * Rodrigo Chang
 *
 * Programa para inicializar el modulo ADC0 utilizando el secuenciador 3 (1 muestra)
 * para muestrear de forma periodica una señal del sensor de temperatura
 *
 * Todo utilizando la libreria Tivaware
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"

// definiciones
#define PF3		HWREG(GPIO_PORTF_BASE + 0x20)
#define PF2		HWREG(GPIO_PORTF_BASE + 0x10)
// control para configurar si se desean las interrupciones del timer, comentar si no se desean
#define __WithTimerInterrupts__

// variables globales
unsigned long valorSensor = 0, noConversiones = 0;


/*
 * Programa principal
 */
int main(void) {
	// Configurar el reloj principal a 40MHz con PLL
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	// 1.1 Configuracion de frecuencia de muestreo
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_125KSPS);

	// Configuracion de un led para toggle
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3);
	PF2 = 0;
	PF3 = 0;

	// Configuracion del modulo ADC0
	// 1. Configuracion de reloj al periferico
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

	// 2. Configurar el numero de secuenciador (=3) y el trigger
	ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);
	// 3. Configurar el unico paso del secuenciador 3 para sensar temperatura y generar interrupcion
	ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END);
	// 4. Configurar las interrupciones
	ADCIntEnable(ADC0_BASE, 3);
	IntEnable(INT_ADC0SS3);
	IntPrioritySet(INT_ADC0SS3, 2);
	// 4. Habilitar el secuenciador 3
	ADCSequenceEnable(ADC0_BASE, 3);

	// Configuracion del timer para muestreo
	// 1. Configuracion de reloj al periferico
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	// 2. Configurar el timer para modo de 32 bits periodico
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	// 3. Configurar el periodo de timeout para 0.5s@40MHz
	TimerLoadSet(TIMER0_BASE, TIMER_A, 20000000-1);
	// 4. Configurar trigger para ADC
	TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

#ifdef __WithTimerInterrupts__
	// 5. Configurar las interrupciones
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // configuracion en el modulo
	IntEnable(INT_TIMER0A);							 // configuracion en el NVIC
	IntPrioritySet(INT_TIMER0A, 2);
#endif
	// 6. Iniciar el timer
	TimerEnable(TIMER0_BASE, TIMER_A);

	// Habilitar interrupciones globales
	IntMasterEnable();

	while (1) {
		//esperar interrupciones
	}
}

/*
 * Manejador de interrupcion de timeout de Timer0 en modo de  32bits
 */
void Timer0_TimeoutHandler(void) {
#ifdef __WithTimerInterrupts__
	// Borrar la bandera de interrupcion
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	// Hacer toggle al led
	PF3 ^= 0xff;
#else
	while(1) {
		// como FaultISR
	}
#endif
}



/*
 * Manejador de interrupcion de conversion completa
 */
void ADC0SS3_Handler(void) {
	// Borrar la interrupcion
	ADCIntClear(ADC0_BASE, 3);

	// Obtener la lectura del ADC
	valorSensor = (ADC0_SSFIFO3_R & 0x00000fff);
#ifndef __WithTimerInterrupts__
	// Hacer toggle al led
	PF2 ^= 0xff;
#endif
	// aumentar el contador
	noConversiones++;
}
