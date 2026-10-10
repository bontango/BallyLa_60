/* 
 * File:   main.c
 * Author: th
 * BallyLa_60
 * PIC 18F25K42
 * READ Port / Write LAT
 * v001 initial version
 * v002 changed sequence check
 * v003 no IOC anymore for read data (inhibit); now after 20uS of strobe, refresh with 'one shot' after 2mS 
 * v004 Claude Session, delay redue to 3uS (see Readme..md)
 * v005 Claude Session, SCR-latch lamp model: Int0_Isr accumulates every strobe,
 *      refreshIsr commits free-running each ~13ms (zero-cross hold emulation;
 *      13ms > measured ~11.3ms worst-case per-lamp refresh gap from digital.csv).
 *      Fixes attract 4x4 matrix lamps staying dark/flickering on KISS.
 * v006 v004/v005 were broken (read data at +3us = always 1111 -> no lamp ON).
 *      Data is valid only while address lines are at 15 (+13..+37us after strobe),
 *      Int0_Isr now waits for address==15 and reads the data there.
 *  */

#include "def.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

//version
#define VERSION 6
#define S1_LONG_TIMEOUT 20
//global vars
volatile int count_int0 = 0;
volatile uint8_t lamp_activ;
volatile uint8_t lamp[60] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t lamp_int[60] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//onboard leds
uint8_t led_test_status, led_test_activ, led_activity, led_ready;     
uint8_t S1_pressed, S2_pressed, S1_pressed_long;
volatile uint8_t in_test = 0;


//all lamps OFF (without matrix)
void lamps_off(void) {
    
    uint8_t i,select;
    
    D1 = D2 = D3 = D4 = D5 = D6 = D7 = D8 = 0;    
    for ( i=0; i<=6; i++)
   {
    LATC = i;
    __delay_us(10);   
   }
    //keep LEDs on last flip flop
    D5 = led_test_status; //D5 test status
    D6 = led_test_activ; //D4 test activ
    D7 = led_activity; //D3 activity
    D8 = led_ready; //D2 ready
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 7
    select = PORTC;
    SET_BIT(select,0);
    SET_BIT(select,1);
    SET_BIT(select,2);
    LATC = select;    
}


//all lamps ON (without matrix)
void lamps_on(void) {
    
    uint8_t i,select;
    
    D1 = D2 = D3 = D4 = D5 = D6 = D7 = D8 = 1;    
   //LATC is D6 D7 D8 EN\ seg_clk A2 A1 A0
    for ( i=224; i<=230; i++)
   {
    LATC = i;
    __delay_us(10);   
   }
    //keep LEDs on last flip flop
    D5 = led_test_status; //D5 test status
    D6 = led_test_activ; //D4 test activ
    D7 = led_activity; //D3 activity
    D8 = led_ready; //D2 ready
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 7
    select = PORTC;
    SET_BIT(select,0);
    SET_BIT(select,1);
    SET_BIT(select,2);
    LATC = select;    
}

//update flip flop values
//different numberng for bally and ballyla
void update_ff(void) {
    uint8_t select;
    
    //LATC is D6 D7 D8 EN\ seg_clk A2 A1 A0
    //where A2..A0 driving a 74HC238 decoder
    //for this routine rest postion must be 7 -> 0b111 
    
    //data for flip-flop 0
    D1 = lamp[7]; //Q4
    D2 = lamp[11]; //Q7
    D3 = lamp[10]; //Q3
    D4 = lamp[9]; //Q2
    D5 = lamp[8]; //Q1
    D6 = lamp[3]; //Q8
    D7 = lamp[1]; //Q12
    D8 = lamp[0]; //Q14
    //wait for stable data    
    __delay_us(10);    
    //raise clock for flip-flop 0
    select = PORTC;
    CLEAR_BIT(select,0);
    CLEAR_BIT(select,1);
    CLEAR_BIT(select,2);    
    LATC = select;
    __delay_us(10);    
    
    //data for flip-flop 1    
    D1 = lamp[2]; //Q13
    D2 = lamp[6]; //Q11
    D3 = lamp[5]; //Q10
    D4 = lamp[4]; //Q9
    D5 = lamp[37]; //Q20
    D6 = lamp[26]; //Q21
    D7 = lamp[24]; //Q17
    D8 = lamp[20]; //Q22
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 1
    select = PORTC;
    SET_BIT(select,0);
    LATC = select;
    __delay_us(10);    
    
    //data for flip-flop 2    
    D1 = lamp[16]; //Q27
    D2 = lamp[17]; //Q28
    D3 = lamp[21]; //Q26
    D4 = lamp[22]; //Q25
    D5 = lamp[23]; //Q24
    D6 = lamp[25]; //Q23
    D7 = lamp[18]; //Q35
    D8 = lamp[19]; //Q34
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 2
    select = PORTC;
    CLEAR_BIT(select,0);
    SET_BIT(select,1);
    LATC = select;
    __delay_us(10);    

    //data for flip-flop 3    
    D1 = lamp[15]; //Q29
    D2 = lamp[36]; //Q32
    D3 = lamp[30]; //Q36
    D4 = lamp[31]; //Q38
    D5 = lamp[41]; //Q39
    D6 = lamp[35]; //Q37
    D7 = lamp[40]; //Q40
    D8 = lamp[38]; //Q42
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 3
    select = PORTC;
    SET_BIT(select,0);
    LATC = select;
    __delay_us(10);    

    //data for flip-flop 4    
    D1 = lamp[39]; //Q41
    D2 = lamp[32]; //Q44
    D3 = lamp[54]; //Q46
    D4 = lamp[33]; //Q49
    D5 = lamp[34]; //Q48
    D6 = lamp[47]; //Q51
    D7 = lamp[56]; //Q53
    D8 = lamp[55]; //Q52
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 4
    select = PORTC;
    CLEAR_BIT(select,0);
    CLEAR_BIT(select,1);
    SET_BIT(select,2);
    LATC = select;
    __delay_us(10);    

    //data for flip-flop 5    
    D1 = lamp[46]; //Q50
    D2 = lamp[48]; //Q54
    D3 = lamp[53]; //Q56
    D4 = lamp[49]; //Q55
    D5 = lamp[51]; //Q59
    D6 = lamp[50]; //Q60
    D7 = lamp[52]; //Q58
    D8 = lamp[45]; //Q57
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 5
    select = PORTC;
    SET_BIT(select,0);
    LATC = select;
    __delay_us(10);    

    //data for flip-flop 6    
    D1 = lamp[57]; //Q47
    D2 = lamp[42]; //Q33
    D3 = lamp[14]; //Q6
    D4 = lamp[29]; //Q19
    D5 = lamp[13]; //Q5
    D6 = lamp[28]; //Q18
    D7 = lamp[12]; //Q16
    D8 = lamp[27]; //Q15
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 6
    select = PORTC;
    CLEAR_BIT(select,0);
    SET_BIT(select,1);
    LATC = select;
    __delay_us(10);    

    //data for flip-flop 7
    D1 = lamp[59]; //Q45
    D2 = lamp[44]; //Q31
    D3 = lamp[43]; //Q30
    D4 = lamp[58]; //Q43
    D5 = led_test_status; //D5 test status
    D6 = led_test_activ; //D4 test activ
    D7 = led_activity; //D3 activity
    D8 = led_ready; //D2 ready
    //wait for stable data
    __delay_us(10);
    //raise clock for flip-flop 7
    select = PORTC;
    SET_BIT(select,0);
    LATC = select;
    
}

// interrupt timer for blinking
void __interrupt(irq(TMR0),low_priority) displayIsr(void)    {
                  
    PIR3bits.TMR0IF = 0;  // clear this interrupt condition   
    
    if ( led_test_status ) {         
        led_test_status=0; //LED ON
        if ( lamp_activ == 0) { //lamp_activ 0 means all lamps
            lamps_on();
        }
        else {
            update_ff();            
        }
    }
    else {
        led_test_status=1; //LED OFF
        lamps_off();        
    }

    
}

void __interrupt(irq(default)) DEFAULT_ISR(void)
{
// Unhandled interrupts go here
}

//big delay val * 10mS
void my_delay( int val) {
    int i;
        
    for( i=0; i<=val; i++) 
    {
        __delay_ms(100);  
    }
    
}    

//set 7seg to number x 
void set_7seg( uint8_t on_off, uint8_t x) {
    SEG_CLK = 0;
    __delay_us(10);
    
    //display to off?
    if (on_off == 0 ) {
        D1 = D2 = D3 = D4 = D5 = D6 = D7 = D8 = 1; //displays OFF
        
    }
    else {
    //Einer D2, D3, D4, D1
    switch(x%10) {
        case 0: D1 = D2 = D3 = D4 = 0; break;
        case 1: D2=0; D3=0; D4=0; D1=1; break;
        case 2: D2=0; D3=0; D4=1; D1=0; break;
        case 3: D2=0; D3=0; D4=1; D1=1; break;
        case 4: D2=0; D3=1; D4=0; D1=0; break;
        case 5: D2=0; D3=1; D4=0; D1=1; break;
        case 6: D2=0; D3=1; D4=1; D1=0; break;
        case 7: D2=0; D3=1; D4=1; D1=1; break;
        case 8: D2=1; D3=0; D4=0; D1=0; break;
        case 9: D2=1; D3=0; D4=0; D1=1; break;                        
    }
    
    //zehner D6, D7, D8, D5
    switch(x/10) {
        case 0: D5 = D6 = D7 = D8 = 0; break;
        case 1: D6=0; D7=0; D8=0; D5=1; break;
        case 2: D6=0; D7=0; D8=1; D5=0; break;
        case 3: D6=0; D7=0; D8=1; D5=1; break;
        case 4: D6=0; D7=1; D8=0; D5=0; break;
        case 5: D6=0; D7=1; D8=0; D5=1; break;
        case 6: D6=0; D7=1; D8=1; D5=0; break;
        case 7: D6=0; D7=1; D8=1; D5=1; break;
        case 8: D6=1; D7=0; D8=0; D5=0; break;
        case 9: D6=1; D7=0; D8=0; D5=1; break;                        
    }
    } //display ON
    
    
    SEG_CLK = 1;
    __delay_us(10);
    SEG_CLK = 0;
}

/* init */
void do_init(void) {
    
uint8_t i;


ENABLE_N = 1; //disable all flip flops
D1 = D2 = D3 = D4 = D5 = D6 = D7 = D8 = 0;
SEG_CLK = 0;
A0 = A1 = A2 = 0;
S1_pressed = S2_pressed = 0;

//set all outputs to zero ( lamps off / LEDs ON )
ENABLE_N = 0; //enable all chips this sets outputs of flip-flop #0 to zero
__delay_us(10);
//set the flip-flops 1..7 to zero, this will activate all LEDs on BallyFA
for ( i=1; i<=7; i++)
{
    LATC = i;
    __delay_us(10);   
}
//show version number on display
set_7seg(1, VERSION);
my_delay(5);
//turn leds off
led_test_status = led_test_activ = led_activity = led_ready = 1;
update_ff();
my_delay(10);
//display OFF
set_7seg(0, VERSION);
//ready LED ON
led_ready = 0;
update_ff();
}

// RB0 / Strobe interrupt
// read address at the strobe edge, then wait for the data phase and read data (inhibit)
//
// Bus protocol per address (verified against LogicPort scan lamps.LPF, 144 strobes):
//  phase 1: address = A,  data = 1111 (all off)  ~28us, strobe pulse latches the address
//  phase 2: address = 15, data = lamp data      from +13.1us to +37.3us after falling edge
// At the strobe edge the data is ALWAYS 1111 (that was the v004/v005 bug).
// Instead of a fixed delay (v003: 20us) we wait until the address lines go to 15,
// so the read point follows the protocol and not the timing of a specific MPU.
#define DATA_PHASE_TIMEOUT 50   // max wait for address==15 in us
void __interrupt(irq(INT0),high_priority) Int0_Isr(void)    {

uint8_t porta, address, timeout;


  PIR1bits.INT0IF = 0;  // clear this interrupt condition

  //read address, stable until ~13us after the falling edge
  address = PORTA & 0x0f; // lower 4 bits = address

  if ( address != 15) {      //address == 15 for this board is rest position
    //wait for data phase (address lines go to 15)
    timeout = DATA_PHASE_TIMEOUT;
    while ( (PORTA & 0x0f) != 0x0f ) {
        if ( --timeout == 0 ) return; //no data phase seen, ignore this strobe
        __delay_us(1);
    }
    //short settle time, address and data lines do not switch exactly together
    __delay_us(2);
    porta = PORTA;
    if ( (porta & 0x0f) != 0x0f ) return; //bus moved on already, ignore this strobe

    // SCR/thyristor latch: every lamp triggered ON (inhibit bit == 0) is latched
    // immediately into the accumulator lamp_int[] and held until the next
    // zero-cross commit (refreshIsr). Independent of WHEN in the multiplex cycle
    // we see the strobe, so fast attract patterns (e.g. a 4x4 matrix lit in quick
    // succession) no longer lose lamps to an ill-timed snapshot.
    if ( CHECK_BIT(porta, 4) == 0 ) lamp_int[address]      = 1; //U1 lamps 0..14
    if ( CHECK_BIT(porta, 5) == 0 ) lamp_int[address + 15] = 1; //U2 lamps 15..29
    if ( CHECK_BIT(porta, 6) == 0 ) lamp_int[address + 30] = 1; //U3 lamps 30..44
    if ( CHECK_BIT(porta, 7) == 0 ) lamp_int[address + 45] = 1; //U4 lamps 45..59
    }
}
    
// Interrupt on change 
// RB6 & RB7 for switches
// RA4,5,6,7 for inhibit
void __interrupt(irq(IOC),low_priority) Int_IOC_Isr(void)    { 
 
  if ( IOCBFbits.IOCBF6 ) {
      IOCBFbits.IOCBF6 = 0;  // clear this interrupt condition       
      S1_pressed = 1;
      S1_pressed_long = 0;
      in_test = 1;
      //wait for switch release
      while ( S1 == 0) {
          __delay_ms(100);
          S1_pressed_long++;
          //show that long press is recognized
          if ( S1_pressed_long == S1_LONG_TIMEOUT)
          {
             led_test_status = led_activity = led_test_activ = led_ready = 1;
             update_ff();
          }   
      }
  }      
  else if ( IOCBFbits.IOCBF7 ) {
      IOCBFbits.IOCBF7 = 0;  // clear this interrupt condition       
      S2_pressed = 1;
      in_test = 1;
  }   
}

// Zero-cross commit ISR, free-running every ~13ms (see Timer2 init below).
// The original board's SCRs are reset by the AC zero-cross (~10ms @ 50Hz) and the
// MPU re-triggers each lamp once per zero-cross; the SCR holds it lit in between.
// This ISR emulates that hold: it publishes the frame that Int0_Isr accumulated in
// lamp_int[] and then clears the accumulator. Because Int0_Isr latches every strobe
// as it happens, a fast multiplex (e.g. the attract 4x4 matrix) is captured no
// matter when it is scanned.
//
// Window size = 13ms, NOT 10ms: the capture (digital.csv, 24MS/s) shows the game
// refreshes most lamps only once per ~10ms zero-cross, and scan jitter stretches
// the gap between two triggers of the same lamp to up to ~11.3ms. The commit window
// must exceed that worst-case gap so every steadily-lit lamp is re-triggered within
// one window (else it flickers). 13ms clears 11.3ms with margin yet stays well below
// the ~26-30ms attract blink period, so real blink animations are preserved.
void __interrupt(irq(TMR2),low_priority) refreshIsr(void)    {
    
    int i;
    
    PIR4bits.TMR2IF = 0;  // clear this interrupt condition   
     
  // In test mode the main loop drives lamp[] directly (INT0 disabled), so do not
  // overwrite it; just keep the accumulator cleared for a clean return to normal.
  if ( in_test ) {
      for(i=0; i<=59; i++) lamp_int[i] = 0;
      return;
  }

  //publish accumulated frame to lamp[] and reset the accumulator (zero-cross reset)
  //INT0 (high prio) may preempt this loop; a lost read->clear race only defers a
  //single ON event to the next frame (self-heals), so INT0 is NOT masked here
  //(masking would overshoot the ~13us data-hold window on the next strobe).
  for(i=0; i<=59; i++)    {
         lamp[i] = lamp_int[i];
         lamp_int[i] = 0;         
     }         

  //do some signaling   
  if ( count_int0++ > 50) {
     if ( led_activity ==0) led_activity=1; else led_activity=0;     
     count_int0 = 0;
  }
}

/*
 * Main
 */
void main(void) {
    
uint8_t i;
uint8_t data_changed = 0;
uint8_t old_led_test_status = 0;
uint8_t old_led_test_activ = 0;
uint8_t old_led_activity = 0;
uint8_t old_led_ready = 0;     

uint8_t old_lamp[60] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//lamp order in internal matrix from Q1 to Q60
uint8_t lamp_order[60] = { 8,  9, 10,  7, 13,   14, 11, 3,  4,  5,
                           6,  1,  2,  0, 27,   12, 24, 28, 29, 37,
                          26, 20, 25, 23, 22,   21, 16, 17, 15, 43,
                          44, 36, 42, 19, 18,   30, 35, 31, 41, 40,
                          39, 38, 58, 32, 59,   54, 57, 34, 33, 46,
                          47, 55, 56, 48, 49,   53, 45, 52, 51, 50 };
/* Port init */
// 0 is output; 1 is input
// Port A
ANSELA = 0;   //All I/O pins are configured as digital
TRISA = 0b11111111;    //all input
//---------------------------------------------
// Port B
ANSELB = 0;   //All I/O pins are configured as digital
TRISB = 0b11000001;    // strobe and switches input 
WPUB = 0b11000000; //weak pull ups for switches
//---------------------------------------------
//Port C
ANSELC = 0;   //All I/O pins are configured as digital
TRISC = 0b00000000; //all output

// now enable Interrupts 
INTCON0bits.GIEH = 1; // Enable high priority interrupts
INTCON0bits.GIEL = 1; // Enable low priority interrupts
INTCON0bits.IPEN = 1; // Enable interrupt priority

//INT0 RB0 (Strobe)
IPR1bits.INT0IP = 1;  //high prority
INTCON0bits.INT0EDG = 0; //Interrupt on falling edge
PIE1bits.INT0IE = 1;  //Enables the INT1 external interrupt on startup

//enable IOC module
IPR0bits.IOCIP = 0;  // IOCI - low priority
PIE0bits.IOCIE = 1;     
//int on change RB7 and 6 for switches
IOCBNbits.IOCBN6 = 1; //INTERRUPT-ON-CHANGE NEGATIVE EDGE
IOCBNbits.IOCBN7 = 1; //INTERRUPT-ON-CHANGE NEGATIVE EDGE

//with the below settings one interrupt each 1,25ms ( 16 * 16 us ))
//T0CON1 = 0x66; // T0CS HFINTOSC; T0CKPS 1:64; T0ASYNC synchronised;  
T0CON1bits.CS = 2; //010 = FOSC/4 -> 62,58nS at 64MHz
T0CON1bits.ASYNC = 0; //0 = The input to the TMR0 counter is synchronized to FOSC/4
T0CON1bits.CKPS = 0b1111; //prescaler 1111 = 1:32768 -> 2,05mS
//T0CON1bits.CKPS = 8;
TMR0H = 250; //each 500mS
TMR0L = 0x00;    // TMR0L 0; 
T0CON0bits.EN = 1; //1 = The module is enabled and operating
T0CON0bits.MD16 = 0; //8bit timer mode
PIR3bits.TMR0IF = 0;   // Clear Interrupt flag before enabling the interrupt    
IPR3bits.TMR0IP = 0; //low priority
//PIE3bits.TMR0IE = 1; // Enabling TMR0 interrupt.
//T0CON0bits.T0EN = 1;// Start the Timer by writing to TMR0ON bit

//TIMER2 free-running, ~13ms period = zero-cross commit for the lamp refresh
    // TMRI - low priority
    IPR4bits.TMR2IP = 0;    
    // Set TMR2 to the options selected in the User Interface
    // T2CS MFINTOSC_31.25KHz; 
    T2CLKCON = 0x06;
    // T2PSYNC Not Synchronized; T2MODE Free Running Period; T2CKPOL Rising Edge; T2CKSYNC Not Synchronized;
    T2HLT = 0x00;    // free-running period mode (was 0x08 = software one-shot)
    // T2RSEL T2CKIPPS pin; 
    T2RST = 0x00;
    // PR2 203 (see below);
    T2PR = 203;      // prescaler 1:2 -> 64us tick; (203+1)*64us = 13.06ms (see note above ISR)
    // TMR2 0; 
    T2TMR = 0x00;
    // Clearing IF flag before enabling the interrupt.
    PIR4bits.TMR2IF = 0;
    // Enabling TMR2 interrupt.
    PIE4bits.TMR2IE = 1;
    // prescaler 1:2 (CKPS=001), postscaler 1:1, timer off for now
    T2CON = 0x10;
    // start free-running timer (previously started in Int0_Isr on address 0)
    T2CONbits.TMR2ON = 1;

do_init();

 while(1)  { 
     if (in_test) { //testmode
         PIE1bits.INT0IE = 0;  //Disable the INT1 external interrupt  
         PIE3bits.TMR0IE = 1; // Enabling TMR0 interrupt.
         T0CON0bits.T0EN = 1;// Start the Timer by writing to TMR0ON bit
         set_7seg(1, 0);
         //set all lamps to zero
         for(i=0; i<=59; i++) lamp[i] = 0;
         //ready and activity off
         update_ff();         
         led_test_status = led_activity = led_ready = 1;
         led_test_activ = 0;
         lamp_activ = 0;
         my_delay(10);
         S1_pressed = S2_pressed = S1_pressed_long = 0;
         while(1) {
          if (S2_pressed) {     //einer
              //wrap around?
              if  (lamp_activ == 60 )
              {
                 //unset old lamp 
                 lamp[lamp_order[lamp_activ-1]] = 0;
                 lamp_activ = 0;
                 set_7seg(1, lamp_activ);  
              }
              else
              {
              //unset old lamp 
              lamp[lamp_order[lamp_activ-1]] = 0;
              lamp_activ++;
              set_7seg(1, lamp_activ);              
              //set new lamp
              lamp[lamp_order[lamp_activ-1]] = 1;
              }
            my_delay(5);
            S2_pressed = 0;
          }
          if (S1_pressed) {  //zehner
              //wrap around?
              if  (lamp_activ > 50 )
              {
                 //unset old lamp 
                 lamp[lamp_order[lamp_activ-1]] = 0;
                 lamp_activ = lamp_activ % 10;
                 set_7seg(1, lamp_activ); 
                 //set new lamp
                 lamp[lamp_order[lamp_activ-1]] = 1;   
              }
              else                                    
              {
              //unset old lamp 
              lamp[lamp_order[lamp_activ-1]] = 0;              
              lamp_activ += 10;
              set_7seg(1, lamp_activ);    
              //set new lamp
              lamp[lamp_order[lamp_activ-1]] = 1;   
              }
            my_delay(5);
            S1_pressed = 0;
              
          }             
          if (S1_pressed_long >= S1_LONG_TIMEOUT) {  //back to normal operation
             PIE3bits.TMR0IE = 0; // Disable TMR0 interrupt.
             for (i=0; i<60; i++) lamp[i] = old_lamp[i] = 0;
             set_7seg(0, 0);               //display off
             led_test_status = led_activity = led_test_activ = 1;
             led_ready = 0;
             update_ff();
             PIE1bits.INT0IE = 1;  //enable the INT1 external interrupt  
             //back to normal mode
             my_delay(5);
             S1_pressed = 0;
             in_test = 0;
             break;
          }          
         }//endless loop
     }
     else {
    //if value change, update flip flops
     //memcmp gives error when using volatile
    //if ( memcmp( old_lamp, lamp, sizeof(old_lamp)) != 0) 
    for (i=0; i<60; i++) {
        if ( lamp[i] != old_lamp[i] ){
            data_changed = 1;
            //save new value
            old_lamp[i] = lamp[i];            
            }
        }     
     //check also LEDs
    if (old_led_test_status != led_test_status) {
        data_changed = 1;
        old_led_test_status = led_test_status;
    }
    if (old_led_test_activ != led_test_activ) {
        data_changed = 1;
        old_led_test_activ = led_test_activ;
    }
    if (old_led_activity != led_activity) {
        data_changed = 1;
        old_led_activity = led_activity;
    }
    if (old_led_ready != led_ready) {
        data_changed = 1;
        old_led_ready = led_ready;
    }

     if (data_changed) {
         data_changed = 0;
            update_ff();
        }
    
     } //not testmode
 }//endless loop
}//main
   