#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define F_CPU 16000000UL  // Clock frequency
#define TEMP_INTERVAL_MS 100
#define HUMIDITY_INTERVAL_MS 200
#define TARGET_TEMP 25    // Ideal temperature in degrees Celsius
#define BAUD 9600
#define MY_UBRR F_CPU/16/BAUD-1

volatile uint16_t current_temp = 0;  // Current temperature reading
volatile uint16_t current_humidity = 0; // Current humidity reading
volatile uint8_t fan_speed = 0;      // Fan speed percentage

void uart_init(unsigned int ubrr) {
    UBRRH = (unsigned char)(ubrr >> 8);
    UBRRL = (unsigned char)ubrr;
    UCSRB = (1 << RXEN) | (1 << TXEN); // Enable RX and TX
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // 8-bit data format
}

void uart_transmit(unsigned char data) {
    while (!(UCSRA & (1 << UDRE))); // Wait until buffer is empty
    UDR = data; // Send data
}

void uart_print(const char *str) {
    while (*str) {
        uart_transmit(*str++);
    }
}
void uart_print_number(uint16_t number) {
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%u", number);
    uart_print(buffer);
}

void adc_init() {
    ADMUX = (1 << REFS0); // AVCC with external capacitor at AREF pin
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // ADC enable, prescaler 64
}

uint16_t read_adc(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    ADCSRA |= (1 << ADSC); // Start conversion
    while (ADCSRA & (1 << ADSC)); // Wait for conversion to complete
    return ADC;
}

void pwm_init() {
    DDRD |= (1 << DDD7); // Set PD7 as output (OC2)
    TCCR2 = (1 << WGM20) | (1 << COM21) | (1 << CS20); // Fast PWM mode, non-inverted
    OCR2 = 0; // Set initial PWM duty cycle to 0%
}

void set_fan_speed(uint8_t speed) {
    OCR2 = speed; // Set the PWM duty cycle
    fan_speed = speed; // Update fan speed percentage
}

uint8_t map_adc_to_pwm(uint16_t adc_value) {
    return (uint8_t)((adc_value * 255) / 1023); // Map ADC value (0-1023) to PWM duty cycle (0-255)
}

int main() {
    uart_init(MY_UBRR);
    adc_init();
    pwm_init();
    char x[]="Hello, World!\r\n";
    uart_print(x);
    uint16_t temp_timer = 0;
    uint16_t humidity_timer = 0;

    while (1) {
        _delay_ms(1);
        temp_timer++;
        humidity_timer++;

        if (temp_timer >= TEMP_INTERVAL_MS) {
            temp_timer = 0;
            uint16_t adc_value = read_adc(0); // Read from channel 0 (PA0)
            current_temp = (adc_value * 500UL) / 1024; // Convert ADC value to temperature in Celsius

            if (current_temp > TARGET_TEMP) {
                uint8_t pwm_value = (uint8_t)((current_temp - TARGET_TEMP) * 255 / 50); // Adjust as needed
                if (pwm_value > 255) pwm_value = 255;
                set_fan_speed(pwm_value); // Set the PWM duty cycle based on the temperature
            } else {
                set_fan_speed(0); // Turn off fan if temperature is at or below target
            }

            uart_print("Temp: ");
            uart_print_number(current_temp);
            uart_print(" C, Fan Speed: ");
            uart_print_number(fan_speed * 100 / 255); // Convert PWM value to percentage
            uart_print("%\r\n");
        }


        if (humidity_timer >= HUMIDITY_INTERVAL_MS) {
            humidity_timer = 0;
            uint16_t adc_value = read_adc(1); // Read from channel 1 (PA1)
            current_humidity = (adc_value * 100UL) / 1024; // Convert ADC value to humidity percentage

            uart_print("Humidity: ");
            uart_print_number(current_humidity);
            uart_print("%\r\n");
        }
    }
}
