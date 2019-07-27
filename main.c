/*
 * main.c
 *
 * Autor: Rodrigo Chang
 * Fecha: 26 de julio de 2014
 *
 * Programa de envio de señales periodicas por medio de valores de 8 bits a traves de la interfaz UART con la PC.
 * Las muestras se envian de forma periodica, y es posible cambiar la forma de onda presionando un boton en la Tiva Launchpad.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "inc/tm4c123gh6pm.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"

// Definiciones utiles
#define LEDS	HWREG(GPIO_PORTF_BASE + 0x08 + 0x10 + 0x20)
#define ESTADOS	4
#define ROJO	0x02
#define AZUL	0x04
#define VERDE	0x08
#define VIOLETA	0x06

// Prototipos de funciones
void ConfigurarLedsBotones(void);
void ConfigurarTimer(unsigned long load);
void ConfigurarUART(void);

// Variables globales
unsigned short estadoActual = 0;	// para llevar cual es el estado actual
unsigned short contMuestras = 0;	// para llevar la cuenta de la muestra que toca

// Maquina de estado
struct estado {
	unsigned char salidaLeds;
	unsigned char longitudMuestras;
	unsigned char muestras [100];
};

typedef const struct estado t_estado;

t_estado maquinaEstados [ESTADOS] = {
	// Diente de sierra
	{ROJO, 75, {0,3,6,10,13,17,20,24,27,31,34,38,41,44,48,51,55,58,62,65,69,72,76,79,83,86,89,93,96,100,103,107,110,114,117,121,124,128,131,134,138,141,145,148,152,155,159,162,166,169,172,176,179,183,186,190,193,197,200,204,207,211,214,217,221,224,228,231,235,238,242,245,249,252,0}},
	// Onda estacionaria
	{AZUL, 100, {192,210,225,233,235,229,215,194,167,136,105,74,46,24,9,2,4,14,31,55,81,109,137,161,181,194,202,202,197,188,175,161,148,136,128,123,123,127,134,142,151,158,163,163,159,149,135,116,95,74,54,37,25,20,22,32,50,74,103,134,166,196,221,240,251,253,247,233,213,187,160,132,106,84,67,56,52,54,62,73,87,101,113,124,130,132,130,125,117,109,100,94,92,94,101,113,129,149,170,191}},
	// Onda rectificada
	{VERDE, 50, {0,16,32,48,64,80,96,111,125,139,153,165,178,189,200,209,218,226,234,240,245,249,252,254,255,255,254,252,249,245,240,234,226,218,209,200,189,178,165,153,139,125,111,96,80,64,48,32,16,0}},
	// Pulso gaussiano
	{VIOLETA, 100, {128,128,129,130,131,131,131,129,126,123,120,118,116,117,120,126,133,141,149,153,154,149,139,125,108,92,80,75,78,92,115,143,172,197,212,213,198,170,131,88,49,22,11,21,49,93,143,193,231,253,253,231,193,143,93,49,21,11,22,49,88,131,170,198,213,212,197,172,143,115,92,78,75,80,92,108,125,139,149,154,153,149,141,133,126,120,117,116,118,120,123,126,129,131,131,131,130,129,128,128}}
};

/*
 * Programa principal
 */
int main(void) {
	// Configurar el reloj a 40MHz
	SysCtlClockSet(SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ | SYSCTL_USE_PLL | SYSCTL_SYSDIV_5);

	// Configurar los leds y botones
	ConfigurarLedsBotones();
	// Configura el timer periodico para 5ms
	ConfigurarTimer(200000);
	// Configurar UART0 (puerto virtual) como 8N1@9600
	ConfigurarUART();

	// Habilitar interrupciones globales
	IntMasterEnable();

	for (;;) {
		// ciclo principal
	}
}

/*
 * Configura el puerto F con los leds y botones a utilizar
 */
void ConfigurarLedsBotones(void) {
	// Configurar LEDs de salida
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0x02);

	// Configurar el boton para cambio de señal
	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;	// Desbloquear PF0
	GPIO_PORTF_CR_R = 0x0f;
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0);	// Configuracion como entrada
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // Configuracion Pull-up resistor

	// Configurar las interrupciones en el boton al flanco de bajada
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE);
	GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_0);
	IntEnable(INT_GPIOF);
	IntPrioritySet(INT_GPIOF, 2);	// la prioridad del gpio es mas alta porque la cantidad de muestras desigual puede causar un fault
}

/*
 * Configura el timer periodico con un perido variable
 */
void ConfigurarTimer(unsigned long load) {
	// Activar el reloj del periferico
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	// Configurar el timer para 32 bits en modo periodico
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	// Cargar el valor del timeout
	TimerLoadSet(TIMER0_BASE, TIMER_A, load - 1);
	// Activar la interrupcion de timeout
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntEnable(INT_TIMER0A);
	IntPrioritySet(INT_TIMER0A, 5);
	// Iniciar el timer
	TimerEnable(TIMER0_BASE, TIMER_A);
}

/*
 * Configura el periferico UART0
 */
void ConfigurarUART(void) {
	// Habilitar reloj al periferico
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	// Configurar los pines de UART
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	// Configurar la velocidad
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600, UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
	// Configuracion de interrupciones...[pendiente]
}

/*
 * Rutina de interrupcion de puerto F cuando se presione el boton
 */
void Int_GPIOF_Handler(void) {
	// Borrar la interrupcion
	GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_0);

	// Quitar el rebote esperando 0.5ms
	SysCtlDelay(20000);
	if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0) {
		// Cambiar de estado
		estadoActual = (estadoActual + 1) % ESTADOS;
		LEDS = maquinaEstados[estadoActual].salidaLeds;
		contMuestras = 0;
	}
}

/*
 * Rutina de interrupcion de TIMER0 periodico
 */
void Int_Timer0_Handler(void) {
	// Borrar la interrupcion
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	// Enviar muestra por el puerto serial
	UARTCharPutNonBlocking(UART0_BASE, maquinaEstados[estadoActual].muestras[contMuestras]);
	contMuestras = (contMuestras + 1) % maquinaEstados[estadoActual].longitudMuestras;
}
