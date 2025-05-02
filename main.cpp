

#define F_CPU 1000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void setRelay(int8_t range);
void show(uint16_t num, int range);
void write(char data);
void write(uint16_t num, int range);

uint8_t digit[10] = {	0b0111111, // -> 0
	0b0000110, // -> 1
	0b1011011, // -> 2
	0b1001111, // -> 3
	0b1100110, // -> 4
	0b1101101, // -> 5
	0b1111100, // -> 6
	0b0000111, // -> 7
	0b1111111, // -> 8
	0b1101111  // -> 9
};

int8_t range = 1;
uint8_t ms40 = 0;

double vin_adc = 0.0;
double vin_real = 0.0;

int main(void)
{
	// IO DDR Settings:
	DDRA = 0b00000000;
	DDRB = 0b10011111;
	DDRC = 0b11111111;
	DDRD = 0b11110000;
	
	// USART Settings:
	UCSRA = 0b00100000;
	UCSRB = 0b10011000;
	UCSRC = 0b10000110;
	
	uint16_t speed = F_CPU / (16.0 * 4800) - 1;
	UBRRL = speed;
	UBRRH = speed >> 8;
	
	// TIMER0 Settings:
	TCCR0 = 0b00001101;
	TCNT0 = 0;
	OCR0  = 38;
	TIMSK = 0b00000010;
	TIFR  = 0b00000010;
	
	// EXT INT Settings;
	MCUCR = 0b00001111;
	GICR  = 0b11000000;
	GIFR  = 0b11000000;
	
	// ADC Settings
	ADMUX = 0b11000000;
	ADCSRA= 0b10011000;
	
	sei();
	
	uint8_t holdKeyLock =  0;
	
	setRelay(1);
	
	/* Replace with your application code */
	while (1)
	{
		if ((PINB & 0b01000000) == 0b01000000 && holdKeyLock == 0){
			holdKeyLock = 1;
			if ( (TCCR0 & 0b00000111) == 0 ){
				TCNT0 = 0;
				TCCR0 = 0b00001101;
				PORTB &= ~0b10000000;
				} else {
				TCCR0 = 0b00001000;
				PORTB |= 0b10000000;
			}
		}
		
		if ((PINB & 0b01000000) == 0b00000000){
			holdKeyLock = 0;
		}
		
		show(vin_real * pow(10.0, 4 - range), range);
		write(vin_real * pow(10.0, 4 - range), range);
	}
}

void setRelay(int8_t range)
{
	if (range < 1){
		range = 1;
	}
	else if (range > 5){
		range = 5;
	}
	
	PORTB = PORTB & ~0b00011111;
	PORTB = PORTB | (0b00000001 << (range - 1));
}

void show(uint16_t num, int range){
	for(uint8_t i = 0; i < 4; i++){
		PORTD |= 0b11110000;
		PORTD &= ~(0b10000000 >> i);
		PORTC = digit[num % 10];
		if (range == 4 - i){
			PORTC |= 0b10000000;
			} else {
			PORTC &= ~0b10000000;
		}
		num = num / 10;
		_delay_ms(5);
	}
}

void write(char data){
	while((UCSRA & 0b00100000) == 0){}
	UDR = data;
}

void write(uint16_t num, int range) {
	uint16_t k = 1000;
	for (uint8_t i = 1; i < 5; i++){
		write( num / k + 48);
		if (range == i){
			write('.');
		}
		num = num % k;
		k = k / 10;
	}
	write('\r');
	write('\n');
}

ISR(INT0_vect){
	range++;
	setRelay(range);
}

ISR(INT1_vect){
	range--;
	setRelay(range);
}

ISR(TIMER0_COMP_vect){
	ms40++;
	if(ms40 == 1){
		ms40 = 0;
		ADCSRA |= 0b01000000;
	}
}

ISR(ADC_vect){
	vin_adc = ADC * 2.56 / 1023.0;
	if (range == 1){
		if (vin_adc > 2){
			range++;
			setRelay(range);
			return;
		}
		else{
			vin_real = vin_adc;
		}
	}
	else if(range == 2){
		if (vin_adc > 2){
			range++;
			setRelay(range);
			return;
		}
		else if (vin_adc < 0.2){
			range--;
			setRelay(range);
			return;
		}
		else{
			vin_real = 10 * vin_adc;
		}
	}
	else if(range == 3){
		if (vin_adc > 2){
			range++;
			setRelay(range);
			return;
		}
		else if (vin_adc < 0.2){
			range--;
			setRelay(range);
			return;
		}
		else{
			vin_real = 100 * vin_adc;
		}
	}
	else if(range == 4){
		if (vin_adc > 2){
			range++;
			setRelay(range);
			return;
		}
		else if (vin_adc < 0.2){
			range--;
			setRelay(range);
			return;
		}
		else{
			vin_real = 1000 * vin_adc;
		}
	}
	else if(range == 5){
		if (vin_adc < 0.2){
			range--;
			setRelay(range);
			return;
		}
		else{
			vin_real = 10000 * vin_adc;
		}
	}
	else
	{
		range = 1;
	}
}