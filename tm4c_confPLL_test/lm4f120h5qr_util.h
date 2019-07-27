/*
 * Header file for LM4F120H5QR microcontroller
 * Funciones de utilidad
 * Rodrigo Chang
 * Ultima fecha de modificacion: 12/06/2014
 */

// Para desbloquear los registros GPIO_CR
#define GPIO_LOCK_KEY	0x4C4F434B

// Definir el divisor del sistema, la division es 400MHz/(SYSDIV2 + 1)
#define SYSDIV2	4

// Inicializa el reloj del sistema para trabajar con el PLL a 80MHz
void PLL_init(void);
