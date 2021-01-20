#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

//******************
// Common Pin masks
//******************

#define CLEAR       0
#define RED         GPIO_PIN_0
#define YELLOW      GPIO_PIN_1
#define GREEN       GPIO_PIN_2

//******************
// Different Pin masks
//******************

#define P_EAST_RED  GPIO_PIN_1
#define P_GREEN     GPIO_PIN_3
#define P_NORTH_RED GPIO_PIN_4

#define Button_N    GPIO_PIN_4
#define Button_E    GPIO_PIN_0

//******************
// Globals
//******************

//
// Constants
//
#define FREQUENCY   16000000

//
// Variables
//

//int NorthCurrentDelay = 5000;
//int EastCurrentDelay = 1000;
char NorthState = 'G';
char EastState = 'R';

//
// Function signatures
//

void TrafficNorthRED(void);
void TrafficNorthYELLOW(void);
void TrafficNorthGREEN(void);
void TrafficEastRED(void);
void TrafficEastYELLOW(void);
void TrafficEastGREEN(void);

void NorthTimerHandler(void);
void EastTimerHandler(void);
void ButtonHandler(void);
void ButtonNorthHandler(void);
void ButtonEastHandler(void);

void setupNorthTimer(void);
void setupEastTimer(void);
void setupPTimer(void);

void printUART(char* string);

//********************************
// Traffic system setup functions
//********************************

//
// North Traffic system setup
//
void setupNorthTraffic(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)));

    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE,RED|YELLOW|GREEN|P_NORTH_RED|P_GREEN);
    setupNorthTimer();
}

//
// East Traffic system setup
//
void setupEastTraffic(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)));

    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE,RED|YELLOW|GREEN);
    setupEastTimer();
}

//
// Pedestrian system setup
//
void setupPedestrianSystem(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)));

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,P_EAST_RED|P_GREEN);
    
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE,GPIO_PIN_4);
    GPIOPinTypeGPIOInput(GPIO_PORTD_BASE,GPIO_PIN_3);
    
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOIntTypeSet(GPIO_PORTF_BASE,Button_N|Button_E,GPIO_FALLING_EDGE);

    IntMasterDisable();
    IntDisable(INT_GPIOF); 
    IntDisable(INT_GPIOD);
    GPIOIntDisable(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_PIN_3);
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_3);
    GPIOIntRegister(GPIO_PORTF_BASE, ButtonNorthHandler);
    GPIOIntRegister(GPIO_PORTD_BASE, ButtonEastHandler);
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_3, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE,GPIO_INT_PIN_4);
    GPIOIntEnable(GPIO_PORTD_BASE,GPIO_INT_PIN_3);
    
    IntPrioritySet(INT_GPIOF,0x40);
    IntPrioritySet(INT_GPIOD,0x40);
    IntEnable(INT_GPIOF);
    IntEnable(INT_GPIOD);
    IntMasterEnable(); 

    GPIOPinWrite(GPIO_PORTF_BASE, P_EAST_RED, P_EAST_RED);
    GPIOPinWrite(GPIO_PORTE_BASE, P_NORTH_RED, P_NORTH_RED);

    setupPTimer();
}

//**********************
// Timer Setup Functions
//**********************

//
// North Timer Setup
//
void setupNorthTimer(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)));

    TimerDisable(TIMER0_BASE, TIMER_BOTH);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT);
    TimerControlStall(TIMER0_BASE, TIMER_A, true);

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntRegister(TIMER0_BASE, TIMER_A, NorthTimerHandler);
}

//
// East Timer Setup
//
void setupEastTimer(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1)));

    TimerDisable(TIMER1_BASE, TIMER_BOTH);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_ONE_SHOT);
    TimerControlStall(TIMER1_BASE, TIMER_A, true);

    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntRegister(TIMER1_BASE, TIMER_A, EastTimerHandler);
}

//
// Pedestrian Timers Setup
//
void setupPTimer(void){
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER2)));
    
    TimerDisable(TIMER2_BASE, TIMER_BOTH);
    TimerConfigure(TIMER2_BASE, TIMER_CFG_ONE_SHOT);
    TimerControlStall(TIMER2_BASE, TIMER_A, true);
    
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER3)));
    
    TimerDisable(TIMER3_BASE, TIMER_BOTH);
    TimerConfigure(TIMER3_BASE, TIMER_CFG_ONE_SHOT);
    TimerControlStall(TIMER3_BASE, TIMER_A, true);
    TimerLoadSet(TIMER3_BASE, TIMER_BOTH, FREQUENCY - 1);

}

//**********************
// UART setup functions
//**********************
void setupUART(void){  
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    
    GPIOPinConfigure(GPIO_PIN_1);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_1);
    
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    
    UARTEnable(UART0_BASE);
}

//**********************
// Timer Delay Functions
//**********************

//
// North traffic Delay
//
void TrafficNorthDelay(int timeS){
    TimerDisable(TIMER0_BASE, TIMER_BOTH);

    TimerLoadSet(TIMER0_BASE, TIMER_BOTH, timeS*FREQUENCY - 1);

    TimerEnable(TIMER0_BASE, TIMER_A);
}

//
// East traffic Delay
//
void TrafficEastDelay(int timeS){
    TimerDisable(TIMER1_BASE, TIMER_BOTH);

    TimerLoadSet(TIMER1_BASE, TIMER_BOTH, timeS*FREQUENCY - 1);

    TimerEnable(TIMER1_BASE, TIMER_A);
}

//
// Uninterruptible Pedestrian Delay
//
void PedestrianDelay(void){
    IntMasterDisable();

    uint32_t LoadValue = (2*FREQUENCY) - 1;
    
    TimerDisable(TIMER2_BASE, TIMER_BOTH);

    TimerLoadSet(TIMER2_BASE, TIMER_BOTH, LoadValue);

    TimerEnable(TIMER2_BASE, TIMER_BOTH);

    while ((TimerValueGet(TIMER2_BASE, TIMER_A) > 0) && (TimerValueGet(TIMER2_BASE, TIMER_A) != LoadValue));
    
    IntMasterEnable();
}


//*********************************
// Traffic events handler Functions
//*********************************

void NorthTimerHandler(void){
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    switch (NorthState)
    {
    case 'R':
        NorthState = 'Y';
        printUART("CARS EAST WEST");
        TrafficEastGREEN();
        break;

    case 'Y':
        NorthState = 'G';
        TrafficEastRED();
        break;

    case 'G':
        NorthState = 'R';
        TrafficNorthYELLOW();
        break;
    
    default:
        break;
    }
}
void EastTimerHandler(void){
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    switch (EastState)
    {
    case 'R':
        EastState = 'Y';
        printUART("CARS NORTH SOUTH");
        TrafficNorthGREEN();
        break;

    case 'Y':
        EastState = 'G';
        TrafficNorthRED();
        break;

    case 'G':
        EastState = 'R';
        TrafficEastYELLOW();
        break;
    
    default:
        break;
    }
}
void ButtonNorthHandler(void){
    if(TimerValueGet(TIMER3_BASE, TIMER_A) == (FREQUENCY - 1)){
      uint32_t current = TimerValueGet(TIMER0_BASE, TIMER_A) ;
      TimerDisable(TIMER0_BASE, TIMER_BOTH);
      
      GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
      
      GPIOPinWrite(GPIO_PORTE_BASE, P_NORTH_RED|P_GREEN, P_GREEN);
      GPIOPinWrite(GPIO_PORTE_BASE, GREEN|RED,RED);
      
      printUART("PEDESTRIAN NORTH SOUTH");
      
      PedestrianDelay();

      GPIOPinWrite(GPIO_PORTE_BASE, P_NORTH_RED|P_GREEN, P_NORTH_RED);
      GPIOPinWrite(GPIO_PORTE_BASE, GREEN|RED,GREEN);
      
      TimerLoadSet(TIMER0_BASE, TIMER_BOTH, current);

      TimerEnable(TIMER0_BASE, TIMER_A);
      printUART("CARS NORTH SOUTH");
      
      TimerEnable(TIMER3_BASE, TIMER_A);
    }
}
void ButtonEastHandler(void){
    if(TimerValueGet(TIMER3_BASE, TIMER_A) == (FREQUENCY - 1)){
      uint32_t current = TimerValueGet(TIMER1_BASE, TIMER_A);
      TimerDisable(TIMER1_BASE, TIMER_BOTH);
      
      GPIOIntClear(GPIO_PORTD_BASE, GPIO_INT_PIN_3);
      
      GPIOPinWrite(GPIO_PORTF_BASE, P_EAST_RED|P_GREEN, P_GREEN);
      GPIOPinWrite(GPIO_PORTD_BASE, GREEN|RED,RED);
      
      printUART("PEDESTRIAN EAST WEST");
      
      PedestrianDelay();

      GPIOPinWrite(GPIO_PORTF_BASE, P_EAST_RED|P_GREEN, P_EAST_RED);
      GPIOPinWrite(GPIO_PORTD_BASE, GREEN|RED,GREEN);
      
      TimerLoadSet(TIMER1_BASE, TIMER_BOTH, current);

      TimerEnable(TIMER1_BASE, TIMER_A);
      printUART("CARS EAST WEST");
      
      TimerEnable(TIMER3_BASE, TIMER_A);
    }
}

//***************************
// Traffic control Functions
//***************************

void TrafficNorthGREEN(void){
    IntPrioritySet(INT_GPIOF, 0);
    IntPrioritySet(INT_GPIOD, 1);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_INT_PIN_3);
    GPIOPinWrite(GPIO_PORTE_BASE, RED|YELLOW, CLEAR);
    GPIOPinWrite(GPIO_PORTE_BASE, GREEN, GREEN);

    TrafficNorthDelay(5);
}
void TrafficNorthYELLOW(void){
    GPIOIntDisable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_INT_PIN_3);
    GPIOPinWrite(GPIO_PORTE_BASE, RED|GREEN, CLEAR);
    GPIOPinWrite(GPIO_PORTE_BASE, YELLOW, YELLOW);

    TrafficEastDelay(2);
}
void TrafficNorthRED(void){
    GPIOIntDisable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_INT_PIN_3);
    GPIOPinWrite(GPIO_PORTE_BASE, YELLOW|GREEN, CLEAR);
    GPIOPinWrite(GPIO_PORTE_BASE, RED, RED);

    TrafficNorthDelay(1);
}

void TrafficEastGREEN(void){
    IntPrioritySet(INT_GPIOD, 0);
    IntPrioritySet(INT_GPIOF, 1);
    GPIOIntDisable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    GPIOIntEnable(GPIO_PORTD_BASE, GPIO_INT_PIN_3);
    GPIOPinWrite(GPIO_PORTD_BASE, RED|YELLOW, CLEAR);
    GPIOPinWrite(GPIO_PORTD_BASE, GREEN, GREEN);
    

    TrafficEastDelay(5);
}
void TrafficEastYELLOW(void){
    GPIOIntDisable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_INT_PIN_3);
    GPIOPinWrite(GPIO_PORTD_BASE, RED|GREEN, CLEAR);
    GPIOPinWrite(GPIO_PORTD_BASE, YELLOW, YELLOW);

    TrafficNorthDelay(2);
}
void TrafficEastRED(void){
    GPIOIntDisable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_INT_PIN_3);
    GPIOPinWrite(GPIO_PORTD_BASE, YELLOW|GREEN, CLEAR);
    GPIOPinWrite(GPIO_PORTD_BASE, RED, RED);

    TrafficEastDelay(1);
}

//*********************
// UART print Function
//*********************
void printUART(char* str){
    while(*str){
        UARTCharPut(UART0_BASE, *(str++));
    };
    UARTCharPut(UART0_BASE, '\n');
    UARTCharPut(UART0_BASE, '\r');
}
//
// Main loop
//
int main(void){
    setupNorthTraffic();
    setupEastTraffic();
    setupPedestrianSystem();
    setupUART();

    __asm("CPSIE  I");

    GPIOPinWrite(GPIO_PORTD_BASE, RED|YELLOW|GREEN, CLEAR);
    GPIOPinWrite(GPIO_PORTE_BASE, RED|YELLOW|GREEN, CLEAR);

    TrafficEastRED();
    while(1){
        __asm("      wfi\n");
    }
}