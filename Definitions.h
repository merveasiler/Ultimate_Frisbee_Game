/* 
 * File:   Definitions.h
 * Author: asimerve
 *
 * Created on April 17, 2023, 6:52 PM
 */

#ifndef DEFINITIONS_H
#define	DEFINITIONS_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef unsigned char bool;     // define bool here for readability and sanity.
#define true 1
#define false 0
#define number_of_players 5     // 4 (team players) + 1 (frisbee)

void __interrupt () isr_routine (void) ;
void initialize();
void move_me();
void compute_frisbee_position();
void generate_scene();
signed int no_conflict(unsigned short p);
void go_right(unsigned short p);
void go_left(unsigned short p);
void go_up(unsigned short p);
void go_down(unsigned short p);
void update_lcd();
void update_lcd_per_player(unsigned short p);
void update_lcd_for_me();
void update_frisbee_target_on_lcd();
void update_ssd();
void update_single_seven_segment_display(unsigned short display_id, unsigned short score);
void update_scores();
void update_speed();
unsigned short random_numerator(unsigned short shift, unsigned short mod);
void turn_on_timers();
void turn_off_timers();

unsigned short portb = 0; 
unsigned short me = 1;                                  // player id that is currently managed by me
unsigned short prev_me = 1;
unsigned short thrower = 1;   
unsigned short player_id = 0;
unsigned short positions[number_of_players][2];         // 0: frisbee, me: [1,2] = teamA or [3,4] = teamB
unsigned short prev_positions[number_of_players][2];
unsigned short frisbee_steps[15][2];                    // maximum 15 steps
unsigned short number_of_steps = 0;
unsigned short current_step_id = 0;
unsigned short tmr0_overflow_count = 0;
unsigned short tmr2_overflow_count = 0;
unsigned short frisbee_target_blink_count = 0;
unsigned short display_id = 2;
unsigned short target_x = 9;
unsigned short target_y = 3;
unsigned char digits[7] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01000000};
unsigned short scoreA = 0, scoreB = 0;
unsigned short adc_value = 1;
unsigned short scene_generator_period = 2500;   // 500
unsigned short frisbee_target_blink_period = 1000; // 200
bool blink_frisbee_target = true;
bool lcd_frisbee_target_flag = false;
bool lcd_update_flag = false;
bool lcd_move_me_flag = false;
bool lcd_draw_me_flag = false;
bool speed_update_flag = false;

byte teamA_player[] = {
                  0b10001,
                  0b10101,
                  0b01010,
                  0b00100,
                  0b00100,
                  0b00100,
                  0b01010,
                  0b01010
                };              // man

byte teamB_player[] = {
                  0b10001,
                  0b10101,
                  0b01010,
                  0b00100,
                  0b01110,
                  0b11111,
                  0b01010,
                  0b01010
                };              // woman

byte selected_teamA_player[] = {
                  0b10001,
                  0b10101,
                  0b01010,
                  0b00100,
                  0b00100,
                  0b00100,
                  0b01010,
                  0b11111
                };              // man

byte selected_teamB_player[] = {
                  0b10001,
                  0b10101,
                  0b01010,
                  0b00100,
                  0b01110,
                  0b11111,
                  0b01010,
                  0b11111
                };              // woman


byte frisbee[] = {
                  0b01110,
                  0b11111,
                  0b11111,
                  0b11111,
                  0b01110,
                  0b00000,
                  0b00000,
                  0b00000
                };              // x

byte frisbee_target[] = {
                  0b01110,
                  0b10001,
                  0b10001,
                  0b10001,
                  0b01110,
                  0b00000,
                  0b00000,
                  0b00000
                };              // x

byte selected_teamA_player_with_frisbee[] = {
                  0b11111,
                  0b10101,
                  0b01010,
                  0b00100,
                  0b00100,
                  0b00100,
                  0b01010,
                  0b11111
                };              // man

byte selected_teamB_player_with_frisbee[] = {
                  0b11111,
                  0b10101,
                  0b01010,
                  0b00100,
                  0b01110,
                  0b11111,
                  0b01010,
                  0b11111
                };              // woman


#ifdef	__cplusplus
}
#endif

#endif	/* DEFINITIONS_H */

