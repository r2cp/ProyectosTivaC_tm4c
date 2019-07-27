/*
 * lm4f120h5qr_util.c
 *
 *  Created on: 12/06/2014
 *      Author: Rodrigo
 */

#include "inc/lm4f120h5qr.h"
#include "lm4f120h5qr_util.h"

/*
 * Configura el reloj del sistema a 80MHz
 */
void PLL_init(void) {
	/*
	 * 0. Configura el sistema para utilizar RCC2 por caracteristicas avanzadas
	 * tales como 400MHz PLL y divisor del reloj del sistema no entero.
	 */
	SYSCTL_RCC2_R |= SYSCTL_RCC2_USERCC2;

	/*
	 * 1. bypass el PLL mientras se inicializa
	 */
	SYSCTL_RCC2_R |= SYSCTL_RCC2_BYPASS2;

	/*
	 * 2. Seleccionar el valor del cristal y la fuente de oscilador
	 */
	SYSCTL_RCC_R &= ~SYSCTL_RCC_XTAL_M;			// Poner a cero el campo XTAL
	SYSCTL_RCC_R += SYSCTL_RCC_XTAL_16MHZ;		// Configurar para cristal de 16MHz
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_OSCSRC2_M;	// Poner a cero el campo de oscilador
	SYSCTL_RCC2_R += SYSCTL_RCC2_OSCSRC2_MO;	// Configurar para fuente de oscilador principal

	/*
	 * 3. Activar el PLL poniendo a cero el campo PWRDN
	 */
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_PWRDN2;

	/*
	 * 4. Poner el divisor del sistema deseado y el bit menos significante
	 */
	SYSCTL_RCC2_R |= SYSCTL_RCC2_DIV400;
	SYSCTL_RCC2_R = (SYSCTL_RCC2_R & ~0x1FC00000) + (SYSDIV2 << 22);

	/*
	 * 5. Esperar que el PLL cierre haciendo polling a PLLLRIS
	 */
	while ((SYSCTL_RIS_R & SYSCTL_RIS_PLLLRIS) == 0) {};

	/*
	 * 6. Habilitar el PLL poniendo a cero BYPASS
	 */
	SYSCTL_RCC2_R &= ~SYSCTL_RCC2_BYPASS2;
}
