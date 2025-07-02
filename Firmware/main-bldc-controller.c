#include <18F46k22.h> 
#device ADC=10        // ADC resolution is defined as 10 bit
#use delay(crystal=8000000) // Clock speed defined for microcontroller

#use FIXED_IO(C_outputs=PIN_C2,PIN_C1,PIN_C0) // PIN_C2 and PIN_C0 Configured as output


#use fast_io(C) // C port is defined as fast io

#use fast_io(D) // D port is defined as fast io

#define START_BUTTON pin_a1
#define STOP_BUTTON pin_a2
#define REVERSE_BUTTON pin_a3
#define LED_GREEN pin_c0
#define LED_RED pin_c1

int16 duty=0; // Holds the PWM duty cycle value.
const int bldc_move_table[8]    = {0, 18, 9, 24, 36, 6, 33, 0};
const int bldc_move_table_rev[8]= {0, 33, 6, 36, 24, 9, 18, 0};

int hall_input=0, rev=0, run=0; // Track rotor position, direction, and running state.
#INT_RB
void RB_isr(void)
{
    hall_input=(input_b()>>4) & 7;  // Reads RB7:RB4 as Hall sensor input (3-bit).
    if(rev==0) // If rev = 0 Motor rotation is clockwise
    {
        output_d(bldc_move_table[hall_input]); // Apply required instruction
    }
    else // Else Motor rotation is counter clockwise
    {
        output_d(bldc_move_table_rev[hall_input]); // Apply required instruction
    }
}

void main()
{
    set_tris_a(0b00001111); // Configure A7,A6,A5,A4 as output and A3,A2,A1,A0 as input
    set_tris_b(0xf0);       // Configure B7,B6,B5,B4 as input and B3,B2,B1,B0 as output
    set_tris_c(0x00);       // Configure PORTC as output
    set_tris_d(0x00);       // Configure PORTD as output

    setup_adc_ports(sAN0);  // Set AN0 channel as analog output
    setup_adc(ADC_CLOCK_INTERNAL|ADC_TAD_MUL_0); // ADC configuration
    set_adc_channel(0);     // Selecting adc conversion source

    delay_us(20);           // For acquisition time
    
    
    setup_timer_2(T2_DIV_BY_16,249,1); // 2 ms overflow, 2ms interrupt or 500Hz

    setup_ccp1(CCP_PWM|CCP_SHUTDOWN_AC_L|CCP_SHUTDOWN_BD_L); // CCP1 Configured as PWM output
    set_pwm1_duty((int16)0); // PWM duty is set at 0

    disable_interrupts(INT_RB); // Interrupt is passive
    enable_interrupts(GLOBAL);

    while(TRUE)
    {
        if(input(START_BUTTON))
        {
            if(run==0) // Check if motor already running
            {
                run=1;
                enable_interrupts(INT_RB); // Interrupt is active
                hall_input=(input_b()>>4) & 7; // First rotor position

                if(rev==0) // If motor not rotating in reverse direction
                {
                    output_low(LED_RED);     // Red light off
                    output_high(LED_GREEN);  // Green light on
                    output_d(bldc_move_table[hall_input]);
                }
                if(rev==1)
                {
                    output_low(LED_GREEN);   // Green light off
                    output_high(LED_RED);    // Red light on
                    output_d(bldc_move_table_rev[hall_input]);
                }
            }
            while(input(START_BUTTON));
        }
        else if(input(STOP_BUTTON)) // Stopping motor
        {
            disable_interrupts(INT_RB); // Interrupt disabled
            run=0;
            output_low(LED_GREEN); // Led1 off
            output_low(LED_RED);   // Led2 off
            output_d(0x00);        // Outputs off
            set_pwm1_duty((int16)0);
            while(input(STOP_BUTTON));
        }
        else if(input(REVERSE_BUTTON)) // Reversing motor direction
        {
            disable_interrupts(INT_RB); // Interrupt is passive
            if(rev==0) // If motor rotation is not reverse indicate by making rev = 1
            {
                rev=1;
                output_low(LED_GREEN); // Led1 off
                output_high(LED_RED);  // Led2 off
            }
            else // If motor rotation is already reverse indicate by making rev = 0
            {
                rev=0;
                output_low(LED_RED);   // Led1 off
                output_high(LED_GREEN);// Led2 off
            }
            run=0;
            set_pwm1_duty((int16)0);
            delay_ms(500);
            while(input(REVERSE_BUTTON));
        }

        if(run==1) // If motor is running (START_BUTTON pressed) Start Adc conversion
        {
            read_adc(ADC_START_ONLY);
            delay_us(100);
            while(!adc_done()); // Wait until conversion complete
            duty = read_adc(ADC_READ_ONLY); // Read ADC value (last value)
            duty = duty * 0.976; // 1000/1024 = 0.976
            set_pwm1_duty((int16)duty); // Set PWM duty cycle
        }
    }
}
