/*
 * main.c
 * Rodrigo Chang
 *
 * Configuracion del ADC0 del microcontrolador para usar el secuenciador 3 (1 muestra) para tomar el valor
 * del sensor de temperatura. Secuenciador activado por timer para tomar la muestra periodicamente
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"


// definicion de registros
#define PF3		HWREG(GPIO_PORTF_BASE + 0x20)
#define PF1		HWREG(GPIO_PORTF_BASE + 0x08)

// variables globales
unsigned long noInterrupciones = 0;		// solamente para debugging
unsigned long noConversiones = 0;
unsigned long tempSensor = 0;

/*
 * Configura el modulo ADC0, secuenciador 3 para muestrear temperatura, trigger por timer
 */
void ADC0SS3_TimerTrigger_Init(void) {
	volatile uint32_t delay;
	// I. Connfiguracion del modulo ADC0

	// 1. Habilitar el reloj para el modulo ADC0
	SYSCTL_RCGCADC_R  |= SYSCTL_RCGCADC_R0;	//tm4c specific
	//SYSCTL_RCGC0_R |= SYSCTL_RCGC0_ADC0; //legacy

	// 1.1 Agregar un pequeño retardo para poder modificar los
	// registros del ADC, sino genera bus fault
	delay = SYSCTL_RCGCADC_R;

	// 3. Configurar el GPIO del canal a utilizar como entrada analogica

	// 4. Cambiar las prioridades de los secuenciadores, se utilizara el secuenciador 3, que por default tiene la
	//	prioridad mas baja (0x3), se le pondra la mas alta (0x0)
	ADC0_SSPRI_R = 0x00000123;

	// 2. Configurar 125kSPS como frecuencia de muestreo, menor frecuencia menor energia
	// 	El maximo se configura en ADC0_PP_R y la que se utiliza en ADC0_PC_R
	//  La tasa de muestreo por defecto es 1MSPS
	ADC0_PC_R &= ~0x0f;		// tm4c specific
	ADC0_PC_R |= 0x1;
	//SYSCTL_RCGC0_R &= ~SYSCTL_RCGC0_ADC0SPD_M;	// legacy
	//SYSCTL_RCGC0_R += SYSCTL_RCGC0_ADC0SPD_125K;


	// II. Configuracion del secuenciador 3

	// 1. Deshabilitar el secuenciador para evitar errores o activaciones durante la programacion
	ADC0_ACTSS_R &= ~0x08;

	// 2. Configurar el evento del trigger por timer
	ADC0_EMUX_R &= ~0xf000;
	ADC0_EMUX_R |= 0x5000;

	// 3. Configurar la fuente de entrada para cada muestra en el secuenciador
	//ADC0_SSMUX3_R &= ~0xf;
	//ADC0_SSMUX3_R |= 0x5; // canal 5

	// 4. Para cada muestra configurar los bits de control
	ADC0_SSCTL3_R = 0xe; //ts0 para sensar la temperatura, ie0 para generar interrupcion, end0 fin de la secuencia

	// 5. Configurar las interrupciones
	// 5.1 Habilitar interrupciones del secuenciador 3
	ADC0_IM_R |= 0x8;

	// 5.2 Configurar la prioridad del secuenciador 3, nivel 2 de prioridad
	NVIC_PRI4_R = (NVIC_PRI4_R & 0xffff00ff) | 0x00004000;

	// 5.3 Habilitar la interrupcion 17
	//NVIC_EN0_R = NVIC_EN0_INT17;
	NVIC_EN0_R = 0x1 << 17;

	// Borrar la bandera IN3 del ADC0_ISC_R escribiendo un 1
	// ADC0_ISC_R |= 0x8;

	// 6. Habilitar el secuenciador
	ADC0_ACTSS_R |= 0x08;


}


/*
 * Configura el TIMER0 en modo periodico de 32 bits con interrupciones
 */
void Timer0_Init(unsigned long periodo) {
	// Activar el reloj para el modulo de TIMER0
	SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R0;
	SysCtlDelay(5);

	// 1. Deshabilitar el timer antes de hacer cualquier cambio
	//	borrando el bit TAEN en GPTMCTL
	TIMER0_CTL_R &= ~0x01;

	// 2. Escribir el registro de configuracion GPTMCFG con un valor de 0
	TIMER0_CFG_R = 0x00;

	// 3. Configurar el campo TnMR en el GPTM Timer n Mode Register GPTMTAMR para modo periodico
	TIMER0_TAMR_R |= 0x2;

	// 4. Cargar el valor del timer
	//TIMER0_TAILR_R = 40000; // para 1ms@40MHz
	//TIMER0_TAILR_R = 20000000; // para 500ms@40MHz
	TIMER0_TAILR_R = periodo - 1;

	// 5. Configurar las interrupciones
	// 5.1 Armar la interrupcion de timeout del timer A
	TIMER0_IMR_R |= 0x01;
	// 5.2 Poner la prioridad en los registros del NVIC
	NVIC_PRI4_R = (NVIC_PRI4_R & 0x00ffffff) | 0x40000000;
	// 5.3 Habilitar la interrupcion correcta en los registros de habilitacion NVIC
	NVIC_EN0_R = 0x1 << 19;
	// 5.4 Borrar la bandera poniendo a 1 TATOCINT en GPMTICR
	TIMER0_ICR_R = 0x01;

	// 6. Iniciar el timer, activando el trigger para ADC y empezar a contar
	TIMER0_CTL_R |= 0x21;
}


/*
 * Programa principal, configura el modulo ADC, el timer y luego espera las interrupciones
 */
int main(void) {
	// configurar el reloj para 40MHz
	SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_RCC_XTAL_16MHZ|SYSCTL_OSC_MAIN);

	// configurar un led
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x04);

	// configurar el adc0ss3
	ADC0SS3_TimerTrigger_Init();
	// configurar el timer con el tiempo de muestreo
	Timer0_Init(20000000);
	// habilitar las interrupciones globales
	IntMasterEnable();

	while (1) {
		// hacer algo, esperar interrupciones
		/*
		PF1 ^= 0xff;
		SysCtlDelay(10000000);
		*/
	}
}

/*
 * Maneja la interrupcion causada por el timeout del timer0 A
 */
void Timer0IntHandler(void) {
	// Borrar la bandera poniendo a 1 TATOCINT en GPMTICR
	TIMER0_ICR_R = 0x01;

	// hacer toggle a un led
	PF3 ^= 0x08;

	// aumentar el numero de interrupciones
	noInterrupciones++;
}

/*
 * Maneja la interrupcion causada por el ss3 del adc0
 */
void ADCSS3_IntHandler(void) {
	// Borrar la bandera IN3 del ADC0_ISC_R escribiendo un 1
	ADC0_ISC_R |= 0x8;

	// Leer el valor convertido de la memoria
	tempSensor = ADC0_SSFIFO3_R & 0x00000fff;

	noConversiones++;
}
