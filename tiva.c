#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "inc/hw_types.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/pwm.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "driverlib/adc.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include <string.h> // Para strlen
#include "driverlib/fpu.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>



uint32_t ui32SysClock;

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {}
#endif

#define LED_PORT      GPIO_PORTK_BASE
#define LED_PIN       GPIO_PIN_0
#define SYSTEM_CLOCK  120000000      // 120 MHz
#define INITIAL_DELAY (SYSTEM_CLOCK * 5)  // 5 segundos
#define BLINK_DELAY   (SYSTEM_CLOCK / 2)  // 0.5 segundos

void ConfigureLED(void);
void ConfigureTimer1A(void);
void Timer1IntHandler(void);

volatile bool initialDelayDone = false;  // Variable de control




void UARTConfig(void){
    // Configurar UART para mostrar los datos en la consola
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, ui32SysClock, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

}
void interrupcio_UART(void){
    // Habilitar interrupciones UART
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
}
#define BUFFER_SIZE 128
char buffer[BUFFER_SIZE];
uint32_t bufferIndex = 0;
char receivedChar;
uint32_t contador = 0;
uint32_t estado;
uint32_t buzzer;
uint32_t valor;
void UARTIntHandler(void)
{
    uint32_t ui32Status;
    int numLength = 0;     // Variable para almacenar el número de dígitos del número recibido
    int contadorLength = 0; // Variable para almacenar el número de dígitos del contador

    //
    // Get the interrrupt status.
    //
    ui32Status = UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(UART0_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    // Reiniciar el índice del buffer
    bufferIndex = 0;
    memset(buffer, 0, BUFFER_SIZE);  // Limpiar el buffer
    contador = 0;
    while(UARTCharsAvail(UART0_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //
        receivedChar = UARTCharGetNonBlocking(UART0_BASE);
        // Solo almacenar si hay espacio en el buffer
        if (bufferIndex < BUFFER_SIZE - 1) {
            buffer[bufferIndex++] = receivedChar; // Almacena el carácter
            buffer[bufferIndex] = '\0'; // Asegúrate de que el string esté terminado
            // Si recibimos un salto de línea o un espacio (indicador de fin de palabra)
                // Comprobar si la palabra recibida es "adelante"
            if (isdigit(receivedChar)) {
                // Convertir la cadena a entero usando atoi
                contador = contador * 10 + (receivedChar - '0'); 
                numLength++;  // Increment the count of numeric characters
            }
            if (strcmp(buffer, "on") == 0) {
                estado = 1; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "off") == 0) {
                estado = 2; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "alerta") == 0) {
                buzzer = 1; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "safe") == 0) {
                buzzer = 0; // Asignar 1 si la palabra es "adelante"
            }
            else if (strcmp(buffer, "izquier") == 0) {
                estado = 4; // Asignar 1 si la palabra es "adelante"
            }
            
        }
    }
    int tempContador = contador;
    do
    {
        contadorLength++;       // Incrementa la longitud del contador
        tempContador /= 10;     // Divide el número entre 10 para remover el último dígito
    } while (tempContador != 0);
    if (numLength == contadorLength)
    {
        // Si las longitudes coinciden, puedes tomar alguna acción
        valor = contador;
    }
}

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}




uint32_t distance;
uint32_t start_time, end_time;
volatile bool echo_received = false;
// ISR para capturar los eventos de la señal Echo
void EchoIntHandler(void)
{
    // Limpiar la interrupción
    GPIOIntClear(GPIO_PORTB_BASE, GPIO_PIN_4);

    if (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_4))
    {
        // Señal de Echo sube: inicio del pulso
        TimerEnable(TIMER0_BASE, TIMER_A);
        start_time = TimerValueGet(TIMER0_BASE, TIMER_A);
    }
    else
    {
        // Señal de Echo baja: fin del pulso
        end_time = TimerValueGet(TIMER0_BASE, TIMER_A);
        echo_received = true;
        TimerDisable(TIMER0_BASE, TIMER_A);
    }
}

/// configuracion de pines
void config_ultrasonic(void){
    // Configurar Trigger y Echo
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);  // Trigger pin
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_4);   // Echo pin

    // Configurar interrupciones para el pin Echo
    GPIOIntRegister(GPIO_PORTB_BASE, EchoIntHandler);
    GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_BOTH_EDGES);
    GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_4);

    // Configurar Timer0 para medir el tiempo del Echo
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT_UP);
}
void distancia(void){
    // Generar pulso de Trigger
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, GPIO_PIN_5);
    SysCtlDelay(ui32SysClock / (1000000 * 3));  // Delay 10 us
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0);

        // Esperar hasta que se reciba la señal Echo
    while (!echo_received);

        // Calcular la distancia en cm
    uint32_t time_diff = end_time - start_time;
    distance = ((time_diff/120) * 0.034) / 2;  // Usar la fórmula de la velocidad del sonido
    echo_received = false;
    return distance;
}



void ConfigurePWM(void)
{
    uint32_t ui32PWMClockRate;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    
    GPIOPinConfigure(GPIO_PF1_M0PWM1);
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinConfigure(GPIO_PF3_M0PWM3);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    
    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_8);
    ui32PWMClockRate = ui32SysClock / 8;
    
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
    
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, (ui32PWMClockRate / 250));
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, (ui32PWMClockRate / 250));
    
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 0);
    
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_2_BIT | PWM_OUT_3_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
}

void ConfigureGPIO(void)
{
    // Habilitar los periféricos de los pines GPIO.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG); // Para PG1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK); // Para PK4 y PK5
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM); // Para PM0

    // Configurar los pines como salida.
    GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_1); // PG1
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_4 | GPIO_PIN_5); // PK4, PK5
    GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, GPIO_PIN_0); // PM0
}
// Variable para almacenar el valor leído del ADC
uint32_t ui32ADCValue;
int main(void)
{
    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                         SYSCTL_OSC_MAIN |
                                         SYSCTL_USE_PLL |
                                         SYSCTL_CFG_VCO_240), 120000000);

                                         
    UARTConfig();
    interrupcio_UART();
    ConfigurePWM();
    ConfigureGPIO();
    config_ultrasonic();

    ConfigureLED();
    ConfigureTimer1A();

    // Habilitar periféricos
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Habilitar ADC0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE); // Habilitar GPIOE
    // Configurar el pin PE3 como entrada ADC (Canal 0)
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); // Configura PE3 como entrada del ADC

    float dutyCycle;
    while (1)
    {
        distancia();
        dutyCycle = (float)valor/100;
        if (distance <= 7){
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * 90) / 100);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_4, GPIO_PIN_4);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, 0);
        }
        else if (distance > 7 && distance <= 15) {

            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * 90) / 100);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_4, 0);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, GPIO_PIN_5);

        }
        else {

            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_0) * 0) / 100);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_4, 0);
            GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, 0);

        }
        if(estado == 1){
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle) / 100);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * dutyCycle) / 100);
        }
        if(estado == 2){
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * 0) / 100);
            PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, (PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * 0) / 100);
        }
    }
}

void ConfigureLED(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK));
    GPIOPinTypeGPIOOutput(LED_PORT, LED_PIN);
    GPIOPinWrite(LED_PORT, LED_PIN, LED_PIN); // Encender LED inicialmente
}

void ConfigureTimer1A(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1));
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, INITIAL_DELAY - 1); // Configurar 5s inicial
    TimerIntRegister(TIMER1_BASE, TIMER_A, Timer1IntHandler);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER1A);
    IntMasterEnable();
    TimerEnable(TIMER1_BASE, TIMER_A);
}

void Timer1IntHandler(void)
{
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    if (buzzer == 1){
        if (!initialDelayDone)
        {
            initialDelayDone = true;
            TimerLoadSet(TIMER1_BASE, TIMER_A, BLINK_DELAY - 1); // Configurar 0.5s
        }
        
        uint8_t ledState = GPIOPinRead(LED_PORT, LED_PIN);
        GPIOPinWrite(LED_PORT, LED_PIN, ledState ^ LED_PIN); // Alternar LED
        UARTSend((uint8_t *)"alerta", 6);
        UARTSend((uint8_t *)"\n", 1);
    }
    else if (buzzer == 0){
        if (!initialDelayDone)
        {
            initialDelayDone = true;
            TimerLoadSet(TIMER1_BASE, TIMER_A, BLINK_DELAY - 1); // Configurar 0.5s
        }
        
        uint8_t ledState = GPIOPinRead(LED_PORT, LED_PIN);
        GPIOPinWrite(LED_PORT, LED_PIN, 0); // Alternar LED
    }
    
}
