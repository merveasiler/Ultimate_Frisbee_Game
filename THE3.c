/*
 * File:   THE3.c
 * Author: Merve Asiler
 *
 * Created on April 14, 2023, 12:13 PM
 */


#define _XTAL_FREQ 40000000L
#include "Pragmas.h"
#include "LCD.h"
#include "Definitions.h"

#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>



void main(void) {
    
    initialize();
    update_lcd();
    T2CONbits.TMR2ON = 1;
    
    while(1)
    {
        update_ssd();
        
        if (lcd_update_flag) {
            lcd_update_flag = false;
            update_lcd_per_player(player_id);
            player_id++;
            if (player_id == number_of_players) {
                player_id = 0;
                if (number_of_steps == 0) {
                    update_scores();
                    turn_off_timers();
                    INTCONbits.INT0F = 0;
                    INTCONbits.INT0E = 1;
                }
            }
            else
                lcd_update_flag = true;
        }
        
        update_ssd();
        
        if (lcd_draw_me_flag) {
            lcd_draw_me_flag = false;
            update_lcd_for_me();
        }
        
        update_ssd();
        
        if (lcd_move_me_flag) {
            lcd_move_me_flag = false;
            update_lcd_per_player(me);
            PORTB = 0xFF;
            LATB = 0xFF;
            INTCONbits.RBIF = 0;
            INTCONbits.RBIE = 1;
        }
        
        update_ssd();
        
        if (lcd_frisbee_target_flag) {
            lcd_frisbee_target_flag = false;
            update_frisbee_target_on_lcd();
        }
        /*
        if (speed_update_flag) {
            update_speed();
            speed_update_flag = false;
        }
         * */
        
    }
    return;
}

void __interrupt () isr_routine (void) 
{   
    if (INTCONbits.T0IF) {  // controls scene generations steps: 1 step per interrupt

        TMR0L = 36;
        TMR0H = 250;
        tmr0_overflow_count++;
        if (tmr0_overflow_count == scene_generator_period) {  // 75 ms
            tmr0_overflow_count = 0;
            generate_scene();
        }
        
        INTCONbits.T0IF = 0;
    }
    
    else if (PIR1bits.TMR2IF) { // controls the blinking of frisbee target

        tmr2_overflow_count++;
        /*
        if (tmr2_overflow_count == 117) { // 2.99 ms
            update_ssd();
            tmr2_overflow_count = 0;
            
            frisbee_target_blink_count++;
            if (frisbee_target_blink_count == 17) {  // 50.83 ms
                lcd_frisbee_target_flag = true;
                frisbee_target_blink_count = 0;
            }
        }
        */
        
        if (tmr2_overflow_count == frisbee_target_blink_period) {  // 30 ms
            lcd_frisbee_target_flag = true;
            tmr2_overflow_count = 0;
        }
         
        
        PIR1bits.TMR2IF = 0;
    }
    
    else if (PIR1bits.ADIF) {
        adc_value = (unsigned short)((ADRESH << 8)+ADRESL);
        speed_update_flag = true;
        PIR1bits.ADIF = 0;
    }
    
    else if (INTCONbits.RBIF) {
        
        portb = PORTB;
        INTCONbits.RBIE = 0;
        move_me();
              
        PORTB = 0xFF;
        INTCONbits.RBIF = 0;
    }
    
    else if (INTCONbits.INT0F) {
        
        compute_frisbee_position();
        
        INTCONbits.INT0F = 0;   
    }
    
    else if (INTCON3bits.INT1F) {
        
        if (number_of_steps > 0) {
            if (!(positions[me][0] == positions[0][0] && positions[me][1] == positions[0][1])) {
                prev_me = me;
                me++;
                if (me == number_of_players)
                    me = 1;
                lcd_draw_me_flag = true; // "me" has changed, update lcd
            }
        }
        
        INTCON3bits.INT1F = 0;
        
    }
    
    return;
    
}

void initialize() {
    
    PLLEN = 0;  //PLL disabled...
    //40MHZ
    IRCF2 = 1;
    IRCF1 = 1;
    IRCF0 = 1;
    
    /* *********** GENERAL INTERRUPTS *********** */
  
    INTCONbits.GIE = 1;     // enable global interrupts
    INTCONbits.PEIE = 1;    // enable peripheral interrupts
    INTCON2bits.RBPU = 0;   // enable pull-ups
    RCONbits.IPEN = 1;      // enable interrupt priority
    INTCONbits.GIEH = 1;    // enable high priority interrupt
    INTCONbits.GIEL = 1;    // enable low priority interrupt
    
    /* **************** TIMER0 **************** */
    
    /*  1 second takes 10x10^6 instruction cycles
        150 ms takes 15x10^5 instruction cycles
        In other words 1500 x 1000 instruction cycles
        Counter counts 2^16 = 65536 instruction cycles
        I should initialize the counter from 65536 - 1500 = 64036.
        Do the operations at every 1000th interrupt. 
     */
    T0CONbits.T08BIT = 0;   // 16 bit counter
    T0CONbits.T0CS = 0;     // internal clock
    T0CONbits.T0SE = 0;     // increment on low to high
    T0CONbits.PSA = 1;      // do not use prescaler
    TMR0L = 36;//60;
    TMR0H = 250;//246;
    
    INTCONbits.T0IF = 0;
    INTCONbits.TMR0IE = 1;  // enable timer0 interrupt
    INTCON2bits.TMR0IP = 1; // high priority
    T0CONbits.TMR0ON = 0;   // this will be on when the frisbee is thrown
    
    /* **************** TIMER1 **************** */
    
    T1CONbits.RD16 = 0;     // read write in two 8 bit ops
    T1CONbits.T1RUN = 1;    // device clock is derived from Timer1 oscillator (?)
    
    T1CONbits.T1CKPS1 = 0;  
    T1CONbits.T1CKPS0 = 0;  // do not use prescaler
    T1CONbits.T1OSCEN = 1;  // timer1 oscillator enabled
    T1CONbits.TMR1CS = 0;   // internal clock
    
    TMR1H = 0;
    TMR1L = 0;
    
    PIR1bits.TMR1IF = 0;   
    PIE1bits.TMR1IE = 0;    // no need to manage any interrupt
    T1CONbits.TMR1ON = 1;
    
    /* **************** TIMER2 **************** */
    
    T2CON = 0;
    TMR2 = 0;
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
    IPR1bits.TMR2IP = 1;    // high priority interrupt
    T2CONbits.TMR2ON = 0;   // this will be on when the frisbee is thrown
    
    /* ************ A/D Converter ************* */
/*    
    ADCON1bits.PCFG3 = 1;   // RA0 = Analog, RA1=1 Analog, RA2 = Analog
    ADCON1bits.PCFG2 = 1;
    ADCON1bits.PCFG1 = 0;
    ADCON1bits.PCFG0 = 0;
        
    ADCON1bits.VCFG0 = 0;   // Vref+=5.0, Vref=0
    ADCON1bits.VCFG1 = 0;
        
    TRISA = 0x00;
    TRISAbits.RA0 = 1;      
    TRISAbits.RA1 = 1;
    TRISAbits.RA2 = 1;
        
    // Tad options (2xTosc, 4xTosc, 8xTosc, 16xTosc, 32xTosc, 64xTosc)
    // min Tad 0.7 us - max Tad 25 us (Keep as short as possible)
    // for 40mhz -> Tosc = 1/40 us
    // use 32xTosc to get 32/40 = 0.8 us, larger than min req, and not too large.
    // for 32 => ADCS = '010'b
    ADCON2bits.ADCS2 = 1;   //Tad 0.8 us
    ADCON2bits.ADCS1 = 1;
    ADCON2bits.ADCS0 = 0;
        
    // Acquisition time options (0xTad, 2xTad, 4xTad, 6xTad, 8xTad, 12xTad, 16xTad, 20xTad)
    // Min acquisition time = 2.4 us
    // Our Tad = 0,8 us, so use 4xTad = 3.2 us, closest one to 2.4us
    ADCON2bits.ACQT2 = 0;
    ADCON2bits.ACQT1 = 0;
    ADCON2bits.ACQT0 = 1;
        
    ADCON2bits.ADFM = 1;    // Right justified...
    ADCON0bits.CHS = 0x00;  // Channel 0
    //ADRESL = 0x00;
    //ADRESH = 0x00;
    
    PIR1bits.ADIF = 0;      // Clear AD interrupt flag.
    PIE1bits.ADIE = 1;      // enable AD interrupt
    ADCON0bits.ADON = 1;    // this will be on when the game is started
    __delay_us(10);
    ADCON0bits.GODONE = 1;  // start the conversion (since the acquisition time is set, there is no need to wait to set GODONE until acquisition time passes)
*/    
    /* **************** PORTA ***************** */
    TRISAbits.RA3 = 0;      // DISP 2,3,4
    TRISAbits.RA4 = 0;
    TRISAbits.RA5 = 0;
    
    /* **************** PORTB ***************** */
    TRISB = 0xFF;
    PORTB = 0xFF;
    portb = 0xFF;
    
    INTCON2bits.RBIP = 1;   // high priority
    INTCONbits.RBIF = 0;    // clear PORTB on change interrupt flag
    INTCONbits.RBIE = 1 ;   // enable PORTB on change interrupt
    
    /* *********** INT-X Interrupts *********** */
    INTCONbits.INT0F = 0;
    INTCONbits.INT0IE = 1;  // enable RB0 button interrupt
    
    INTCON3bits.INT1F = 0;
    INTCON3bits.INT1E = 1;  // enable RB1 button interrupt
    
    /* ****** General Purpose Variables ******* */
    positions[0][0] = 9;    positions[0][1] = 2;
    positions[1][0] = 3;    positions[1][1] = 2;
    positions[2][0] = 3;    positions[2][1] = 3;
    positions[3][0] = 14;   positions[3][1] = 2;
    positions[4][0] = 14;   positions[4][1] = 3;
    
    /* ***************** LCD ****************** */
    ADCON1bits.PCFG3 = 1;
    ADCON1bits.PCFG2 = 1;
    ADCON1bits.PCFG1 = 1;
    ADCON1bits.PCFG0 = 1;
    
    InitLCD();
    LCDAddSpecialCharacter(1, teamA_player);
    LCDAddSpecialCharacter(2, teamB_player);
    LCDAddSpecialCharacter(3, frisbee);
    LCDAddSpecialCharacter(4, frisbee_target);
    LCDAddSpecialCharacter(5, selected_teamA_player);
    LCDAddSpecialCharacter(6, selected_teamB_player);
    LCDAddSpecialCharacter(7, selected_teamA_player_with_frisbee);
    LCDAddSpecialCharacter(0, selected_teamB_player_with_frisbee);
    
}

void move_me() 
{   // 433 micro seconds
    
    lcd_move_me_flag = true;
    
    prev_positions[me][0] = positions[me][0];
    prev_positions[me][1] = positions[me][1];
    
    portb = portb & 0xF0;
    if (portb == 0xE0)              // RB4  up
        go_up(me);
    else if (portb == 0xD0)         // RB5  right
        go_right(me);
    else if (portb == 0xB0)         // RB6  down
        go_down(me);
    else if (portb == 0x70)         // RB7  left
        go_left(me);
    else 
        lcd_move_me_flag = false;    // button released
    
    if (lcd_move_me_flag) {
        if (no_conflict(me) > 0) {  // conflict with frisbee is ok, the others not
            positions[me][0] = prev_positions[me][0];
            positions[me][1] = prev_positions[me][1];
            lcd_move_me_flag = false;
        }
    }
    
    if (!lcd_move_me_flag) {
        PORTB = 0xFF;
        LATB = 0xFF;
        INTCONbits.RBIF = 0;
        INTCONbits.RBIE = 1;
    }
    
}

void compute_frisbee_position() {
    
    if (number_of_steps > 0)    // the previous throw is not completed yet
        return;
    
    if (positions[thrower][0] != positions[0][0] || positions[thrower][1] != positions[0][1])
        return;
        
    INTCONbits.INT0E = 0;
    
    // we can throw the frisbee and switch to the active mode
    turn_on_timers();
    
    int x_step = 1, y_step = 1;
    unsigned short x_step_size, y_step_size;
    
    // compute target position
    while(1) {
        
        target_x = random_numerator(3, 16) + 1;
        target_y = random_numerator(1, 4) + 1;
        
        if (target_x < positions[0][0]) {
            x_step_size = positions[0][0] - target_x;
            x_step = -1;
        }
        else
            x_step_size = target_x - positions[0][0];
        
        if (target_y < positions[0][1]) {
            y_step_size = positions[0][1] - target_y;
            y_step = -1;
        }
        else
            y_step_size = target_y - positions[0][1];
        
        if (x_step_size <= 2 && y_step_size <= 2)
            continue;
        
        if (x_step_size > y_step_size)
            number_of_steps = x_step_size;
        else
            number_of_steps = y_step_size;
        
        break;
    }
    
    // compute path of the frisbee
    
    unsigned short x = positions[0][0];
    if (target_x < positions[0][0]) {
        for (unsigned short i = 0; i < x_step_size; i++) {
            x = x - 1;
            frisbee_steps[i][0] = x;
        }  
    }
    else {
        for (unsigned short i = 0; i < x_step_size; i++) {
            x = x + 1;
            frisbee_steps[i][0] = x;
        } 
    }
    for (unsigned short i = x_step_size; i < number_of_steps; i++)
        frisbee_steps[i][0] = x; 
    
    unsigned short y = positions[0][1];
    if (target_y < positions[0][1]) {
        for (unsigned short i = 0; i < y_step_size; i++) {
            y = y - 1;
            frisbee_steps[i][1] = y;
        }  
    }
    else {
        for (unsigned short i = 0; i < y_step_size; i++) {
            y = y + 1;
            frisbee_steps[i][1] = y;
        } 
    }
    for (unsigned short i = y_step_size; i < number_of_steps; i++)
        frisbee_steps[i][1] = y; 
    
    /*
    int x = (int)positions[0][0];
    for (unsigned short i = 0; i < x_step_size; i++) {
        x += x_step;
        frisbee_steps[i][0] = (unsigned short)x;
    }
    
    for (unsigned short i = x_step_size; i < number_of_steps; i++)
        frisbee_steps[i][0] = (unsigned short)x;    // fill the remaining parts with x
    
    int y = (int)positions[0][1];
    for (unsigned short i = 0; i < y_step_size; i++) {
        y += y_step;
        frisbee_steps[i][1] = (unsigned short)y;
    }
    for (unsigned short i = y_step_size; i < number_of_steps; i++)
        frisbee_steps[i][1] = (unsigned short)y;    // fill the remaining parts with y
    */
    current_step_id = 0;
    return;
}

void generate_scene() {
    
    if (number_of_steps > 0) {

        // save frisbee move
        prev_positions[0][0] = positions[0][0];
        prev_positions[0][1] = positions[0][1];
        positions[0][0] = frisbee_steps[current_step_id][0];
        positions[0][1] = frisbee_steps[current_step_id][1];

        // save player moves
        for (unsigned short p = 1; p < number_of_players; p++) {

            if (p == me)
                continue;

            // save the previous moves
            prev_positions[p][0] = positions[p][0];
            prev_positions[p][1] = positions[p][1];
            
            unsigned short step_type = random_numerator(1, 9);
            
            while (true) {
                positions[p][0] = prev_positions[p][0];
                positions[p][1] = prev_positions[p][1];
                
                switch(step_type) {
                    case 0:
                        go_right(p);
                        break;
                    case 1:
                        go_up(p);
                        break;
                    case 2:
                        go_left(p);
                        break;
                    case 3:
                        go_down(p);
                        break;
                    case 4:
                        go_right(p);
                        go_up(p);
                        break;
                    case 5:
                        go_right(p);
                        go_down(p);
                        break;
                    case 6:
                        go_left(p);
                        go_up(p);
                        break;
                    case 7:
                        go_left(p);
                        go_down(p);
                        break;
                    case 8:
                        // no move
                        break;
                }
                if (no_conflict(p) == -1)
                    break;
                step_type++;
                step_type %= 9;
                
            }
            
        }

        number_of_steps--;
        current_step_id++;
        lcd_update_flag = true;
        
    }
    
    return;
}

signed int no_conflict(unsigned short p) {
    
    for (unsigned short i = 0; i < number_of_players; i++) {
        if (i == p)
            continue;
        if (positions[p][0] == positions[i][0] && positions[p][1] == positions[i][1])    // conflicting players
            return i;
    }
    
    return -1;
    
}

void go_right(unsigned short p) {
    
    positions[p][0]++;
    
    if (positions[p][0] == 17)  // out of area
        positions[p][0]--; 
    
}

void go_left(unsigned short p) {
    
    positions[p][0]--;
    
    if (positions[p][0] == 0)   // out of area
        positions[p][0]++;

}

void go_up(unsigned short p) {
    
    positions[p][1]--;
    
    if (positions[p][1] == 0)   // out of area
        positions[p][1]++;
    
}

void go_down(unsigned short p) {
    
    positions[p][1]++;
    
    if (positions[p][1] == 5)   // out of area
        positions[p][1]--;
    
}

void update_lcd() {
    
    for (unsigned short p = 0; p < number_of_players; p++) {
        LCDGoto(prev_positions[p][0], prev_positions[p][1]);
        LCDStr(" ");    // delete previous position
        
        LCDGoto(positions[p][0], positions[p][1]);
        if (p == 0) {
            LCDDat(3);
        }
        else if (p == me) {
            if (me < 3)
                LCDDat(5);
            else
                LCDDat(6);
        }
        else if (p < 3) {
            LCDDat(1);
        }
        else {
            LCDDat(2);
        }
        
        prev_positions[p][0] = positions[p][0];
        prev_positions[p][1] = positions[p][1];
    }
    
    __delay_us(3000);
    
}

void update_lcd_per_player(unsigned short p) 
{   // 29.6 ms
    
    LCDGoto(prev_positions[p][0], prev_positions[p][1]);
    update_ssd();
    if (p > 0 && prev_positions[p][0] == positions[0][0] && prev_positions[p][1] == positions[0][1])    
        LCDDat(3);      // do not touch frisbee
    else
        LCDStr(" ");    // delete previous position
    update_ssd();
        
    LCDGoto(positions[p][0], positions[p][1]);  update_ssd();
    if (p == 0)
        LCDDat(3);
    
    else if (p == me && positions[p][0] == positions[0][0] && positions[p][1] == positions[0][1]) {
        if (me < 3)
            LCDDat(7);
        else
            LCDDat(0);
    }
    
    else {
        if (p == me) {
            if (me < 3)
                LCDDat(5);
            else
                LCDDat(6);
        }
        else if (p < 3)
            LCDDat(1);
        else
            LCDDat(2);
    }
    update_ssd();
    
    __delay_ms(2);
                                                         
    
} 

void update_lcd_for_me() {
    
    LCDGoto(positions[prev_me][0], positions[prev_me][1]);  update_ssd();
    if (prev_me < 3)
        LCDDat(1);
    else
        LCDDat(2);
    update_ssd();
    
    LCDGoto(positions[me][0], positions[me][1]);    update_ssd();
    if (positions[me][0] == positions[0][0] && positions[me][1] == positions[0][1]) {
        if (me < 3)
            LCDDat(7);
        else
            LCDDat(0);
    }
    else {
        if (me < 3)
            LCDDat(5);
        else
            LCDDat(6);
    }
    update_ssd();
    
    __delay_ms(2);
    
}

void update_frisbee_target_on_lcd() {
    
    if (number_of_steps > 0) {
        // frisbee target position
        LCDGoto(target_x, target_y);    update_ssd();

        if (blink_frisbee_target)
            LCDDat(4);
        else
            LCDStr(" ");
        update_ssd();

        blink_frisbee_target = !blink_frisbee_target;
        
        //LCDCursorReset(positions[me][0], positions[me][1]);
        __delay_ms(2);   
    }
    
}

void update_ssd() {
    
    switch (display_id) {
        case 2: 
            update_single_seven_segment_display(2, scoreA);
            break;
        case 3:
            update_single_seven_segment_display(3, 6);
            break;
        case 4:
            update_single_seven_segment_display(4, scoreB);
            break;
    }
    display_id++;
    if (display_id == 5)
        display_id = 2;
    
}

void update_single_seven_segment_display(unsigned short display_id, unsigned short score)
{ // 2.3 micro seconds
    PORTAbits.RA3 = 0;
    PORTAbits.RA4 = 0;
    PORTAbits.RA5 = 0;
    
    if (display_id == 2)
        PORTAbits.RA3 = 1;
    else if (display_id == 3)
        PORTAbits.RA4 = 1;
    else if (display_id == 4)
        PORTAbits.RA5 = 1;
    else
        ;
    PORTD = digits[score];
    //__delay_ms(2);
    
}

void update_scores() {
    
    for (unsigned short p = 1; p < number_of_players; p++) {
        
        if (positions[p][0] == positions[0][0] && positions[p][1] == positions[0][1]) {
            if (p < 3) {
                if (thrower > 2)
                    ;
                else if (positions[p][0] > 14)
                    scoreA++;
            }
            else {
                if (thrower < 3)
                    ;
                else if (positions[p][0] < 3)
                    scoreB++;
            }
            thrower = p;
            return;
        }
        
    }
    
    prev_me = me;
    
    if (thrower < 3)
        me = 3;
    else
        me = 1;
    
    thrower = me;
    lcd_draw_me_flag = true; // "me" has changed, update lcd
    
}

void update_speed() {
    
    unsigned short coeff = adc_value / 256 + 1; // 1, 2, 3, 4
    scene_generator_period = 500;// * coeff;
    //frisbee_target_blink_period = 200;// * coeff;
    ADCON0bits.GODONE = 1;
    
}

unsigned short random_numerator(unsigned short shift, unsigned short mod) {
    
    int seed = TMR1H;
    seed = seed << 8;
    seed += TMR1L;
    
    unsigned short number = seed % mod;
    seed = seed >> shift;
    
    TMR1L = seed;
    seed = seed >> 8;
    TMR1H = seed;
    
    return number;
    
    /*
    char datetime[10];
    sprintf(datetime, "%s", __TIME__);
    return (datetime[0] * 10 + datetime[1]) * 3600 + (datetime[3] * 10 + datetime[4]) * 60 + (datetime[6] * 10 + datetime[7]);
    */
}
    
void turn_on_timers() {
    
    TMR0L = 36;
    TMR0H = 250;
    T0CONbits.TMR0ON = 1;
    
    //TMR2 = 0;
    //T2CONbits.TMR2ON = 1; 

}

void turn_off_timers() {
    
    T0CONbits.TMR0ON = 0;
    //T2CONbits.TMR2ON = 0; 
    
}
