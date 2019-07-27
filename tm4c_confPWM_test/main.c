/*
 * main.c
 *
 * Autor: Rodrigo Chang
 * Fecha: 6 de agosto de 2014
 *
 * Programa para probar la configuracion del modulo 1 PWM en el pin PD0 (M1PWM0) para controlar un motor servo.
 * El programa configura el modulo para trabajar a 50Hz y entre 1ms y 2ms de ciclo de trabajo
 *
 * IMPORTANTE. La librería configura de la siguiente forma el modulo PWM en modo descendente:
 * 		- En el valor de carga pone en alto la salida del generador
 * 		- En el valor de comparacion pone en bajo la salida
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"

#include "inc/hw_gpio.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"

// Definiciones
#define __CONFIGURACION_CON_REGISTROS__
#define PERIODO_PWM	50000
#define MAX			6000
#define MIDPOINT	4250
#define	MIN			2500
#define STEP		400
#define PF0	HWREG(GPIO_PORTF_BASE + 4)
#define PF4	HWREG(GPIO_PORTF_BASE + 64)

// Prototipos de funciones
void PWM1_Init(uint16_t periodo, uint16_t duty);
void PWM1_Duty(uint16_t duty);
void Demonstration_routine(uint16_t current);
void configurarGPIO(void);

// Variables globales
uint16_t duty_cycle;

/*
 * Funcion principal
 */
int main(void) {
	// Configuracion del reloj a 40MHz
	SysCtlClockSet(SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ | SYSCTL_USE_PLL | SYSCTL_SYSDIV_5);
	// Configuracion del modulo PWM1 con ciclo de trabajo inicial de 1.5ms
	PWM1_Init(50000, MIDPOINT);
	// Configurar botones GPIO
	configurarGPIO();
	duty_cycle = MIDPOINT;

	while (1) {
		//Demonstration_routine();

		// Rutina de control de posicion
		if (!PF4) {
			duty_cycle -= STEP;
			if (duty_cycle < MIN) {
				duty_cycle = MIN;
			}
			PWM1_Duty(duty_cycle);
		}
		if (!PF0) {
			duty_cycle += STEP;
			if (duty_cycle > MAX) {
				duty_cycle = MAX;
			}
			PWM1_Duty(duty_cycle);
		}
		if (!PF0 && !PF4) {
			Demonstration_routine(duty_cycle);
		}
		SysCtlDelay(533333); // 40ms
		//SysCtlDelay(266666); // 20ms menor tiempo hace que el motor ejecute mas rapido
	}
}

/*
 * Configura el modulo PWM1 en el pin PD0 (M1PWM0)
 * El reloj para el modulo PWM es el divisor de sistema / 16
 */
void PWM1_Init(uint16_t periodo, uint16_t duty) {
	#ifdef __CONFIGURACION_CON_REGISTROS__
		volatile uint32_t delay;
		// Activar el reloj para el modulo PWM y el puerto D
		//SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;
		SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
		//SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD;
		SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;
		delay = SYSCTL_RCGC2_R;

		// Configurar los pines
		GPIO_PORTD_AFSEL_R |= 0x01;	// Activa las funciones alternativas en PD0
		GPIO_PORTD_DEN_R |= 0x01;	// Activa la funcion digital en el pin
		GPIO_PORTD_PCTL_R = 0x00000005; // Configura M1PWM0 en el PD0

		// Configurar el reloj para el modulo PWM
		SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV;	// Usar divisor para PWM
		SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M;	// Borrar el campo PWMDIV
		SYSCTL_RCC_R |= SYSCTL_RCC_PWMDIV_16;	// Divisor de reloj por 16

		// Configurar el generador de PWM
		PWM1_0_CTL_R = 0;	// Modo de recarga
		PWM1_0_GENA_R = (PWM_0_GENA_ACTCMPAD_ONE | PWM_0_GENA_ACTLOAD_ZERO);	// Configurar acciones de comparador y carga
		//PWM1_0_GENA_R = (PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE);	// Configuracion como la libreria
		// No configuré PWM1_0_GENB_R porque al final no se mapeó en pines

		// Cargar el periodo y el ciclo de trabajo inicial
		PWM1_0_LOAD_R = periodo - 1;
		PWM1_0_CMPA_R = duty - 1;

		// Iniciar los timers en el generador 0 del modulo 1
		PWM1_0_CTL_R |= PWM_0_CTL_ENABLE;
		PWM1_ENABLE_R |= PWM_ENABLE_PWM0EN; // Habilita el generador 0
	#else
		// Configurar el reloj para el modulo
		SysCtlPWMClockSet(SYSCTL_PWMDIV_16);
		// Habilitar el reloj de los perifericos necesarios
		SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
		// Configurar los pines del periferico
		GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);
		GPIOPinConfigure(GPIO_PD0_M1PWM0);
		// Configurar el modulo 1 PWM 0 en modo descendente
		PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
		// Cargar el periodo
		PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, periodo - 1);
		// Configurar el duty cycle
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, duty - 1);
		PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
		PWMGenEnable(PWM1_BASE, PWM_GEN_0);
	#endif
}

/*
 * Cambia el ciclo de trabajo del modulo PWM1 en el pin PD0 (M1PWM0)
 */
void PWM1_Duty(uint16_t duty) {
	PWM1_0_CMPA_R = duty - 1;
}


/*
 * Crea una rutina de demostración
 */
void Demonstration_routine(uint16_t current) {
	#ifdef __CONFIGURACION_CON_REGISTROS__
		PWM1_Duty(MIN);
		SysCtlDelay(13333333); // 1s
		PWM1_Duty(MIDPOINT);
		SysCtlDelay(13333333);
		PWM1_Duty(MAX);
		SysCtlDelay(13333333);
		PWM1_Duty(current);
		SysCtlDelay(13333333);
	#else
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, MIN);
		SysCtlDelay(13333333); // 1s
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, MIDPOINT);
		SysCtlDelay(13333333);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, MAX);
		SysCtlDelay(13333333);
		PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, current);
		SysCtlDelay(13333333);
	#endif
}


/*
 * Configura los botones en PF0 y en PF1
 */
void configurarGPIO(void) {
	// Habilitar el reloj al modulo
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	// Configuracion de GPIO
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0xff);
	// Desbloquear PF0
	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;	// Desbloquear PF0
	GPIO_PORTF_CR_R = 0x0f;
	// Configuracion de entradas y pull-up
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}
