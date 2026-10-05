/* 
 * File:   def.h
 * Author: th
 * part of BallyLA
 * Version 100
 * 14.11.2024
 * * READ Port / Write LAT
 */

#ifndef DEF_H
#define	DEF_H

// PIC18F25K42 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1L
#pragma config FEXTOSC = OFF    // External Oscillator Selection (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_64MHZ// Reset Oscillator Selection (HFINTOSC with HFFRQ = 64 MHz and CDIV = 1:1)

// CONFIG1H
#pragma config CLKOUTEN = OFF   // Clock out Enable bit (CLKOUT function is disabled)
#pragma config PR1WAY = ON      // PRLOCKED One-Way Set Enable bit (PRLOCK bit can be cleared and set only once)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2L
#pragma config MCLRE = EXTMCLR  // MCLR Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTS = PWRT_OFF // Power-up timer selection bits (PWRT is disabled)
#pragma config MVECEN = ON      // Multi-vector enable bit (Multi-vector enabled, Vector table used for interrupts)
#pragma config IVT1WAY = ON     // IVTLOCK bit One-way set enable bit (IVTLOCK bit can be cleared and set only once)
#pragma config LPBOREN = OFF    // Low Power BOR Enable bit (ULPBOR disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG2H
#pragma config BORV = VBOR_2P45 // Brown-out Reset Voltage Selection bits (Brown-out Reset Voltage (VBOR) set to 2.45V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config DEBUG = OFF      // Debugger Enable bit (Background debugger disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31// WDT Period selection bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF       // WDT operating mode (WDT Disabled; SWDTEN is ignored)

// CONFIG3H
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG4L
#pragma config BBSIZE = BBSIZE_512// Boot Block Size selection bits (Boot Block size is 512 words)
#pragma config BBEN = OFF       // Boot Block enable bit (Boot block disabled)
#pragma config SAFEN = OFF      // Storage Area Flash enable bit (SAF disabled)
#pragma config WRTAPP = OFF     // Application Block write protection bit (Application Block not write protected)

// CONFIG4H
#pragma config WRTB = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-30000Bh) not write-protected)
#pragma config WRTC = OFF       // Boot Block Write Protection bit (Boot Block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)
#pragma config WRTSAF = OFF     // SAF Write protection bit (SAF not Write Protected)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored)

// CONFIG5L
#pragma config CP = OFF         // PFM and Data EEPROM Code Protection bit (PFM and Data EEPROM code protection disabled)

// CONFIG5H

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

//functions

#define _XTAL_FREQ 64000000      // 16 MHz internal Clock // Fosc  frequency for _delay()  library
//#define _XTAL_FREQ 16000000 

//Port Definitions
//PORTA 
#define AD0 PORTAbits.RA0 //lamp address
#define AD1 PORTAbits.RA1
#define AD2 PORTAbits.RA2
#define AD3 PORTAbits.RA3
#define PD0 PORTAbits.RA4 //lamp data
#define PD1 PORTAbits.RA5
#define PD2 PORTAbits.RA6
#define PD3 PORTAbits.RA7

//PortB
#define STROBE PORTBbits.RB0
#define D5 LATBbits.LB1 //data for 74HC574
#define D4 LATBbits.LB2
#define D3 LATBbits.LB3
#define D2 LATBbits.LB4
#define D1 LATBbits.LB5
#define S1 PORTBbits.RB6
#define S2 PORTBbits.RB7

//PortC 
#define A0 LATCbits.LC0
#define A1 LATCbits.LC1
#define A2 LATCbits.LC2
#define SEG_CLK LATCbits.LC3
#define ENABLE_N LATCbits.LC4
#define D8 LATCbits.LC5
#define D7 LATCbits.LC6
#define D6 LATCbits.LC7

//
//some bit settings routines
//postion(pos) goes from 0..7
//
#define CHECK_BIT(var, pos)   ((var) & (1 << (pos)))

#define SET_BIT(var, pos)     var |= (1 << pos)

#define CLEAR_BIT(var, pos)   var &= (~(1 << pos))

#define TOGGLE_BIT(var, pos)  var ^= (1 << pos)

#endif	/* DEF_H */

