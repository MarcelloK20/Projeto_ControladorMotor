
#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define MAX_RPM 5300
#define MAX_PWM 255
#define PULSES_PER_REV 28
volatile uint32_t count = 0;
volatile uint32_t pulse_count = 0;

void Timer1_init()
{
	// Definir o Timer1 em modo CTC
	TCCR1 |= (1 << CTC1) | (1 << CS13) | (1 << CS10);  // Prescaler de 256

	// Calcular o valor do OCR1C para 10ms
	OCR1C = 39;  // Para 1MHz: (1MHz / (256 * 100Hz)) - 1 = 38

	// Habilitar interrupção de comparação do Timer1
	TIMSK |= (1 << OCIE1A);
}

void Timer0_init()
{
	// Configurar o Timer0 para Fast PWM, modo não invertido
	TCCR0A |= (1<<COM0A1) | (1<<WGM00) | (1<<WGM01) | (1 << COM0B1);
	TCCR0B |= (1<<CS01);  // Prescaler de 8 para o Timer0
	// Inicializar o valor do PWM
	OCR0B = 0;
	// Configurar o PB0 e PB1 como saída
	DDRB |= (1 << PB0) | (1 << PB1);
}

ISR(INT0_vect)
{
	pulse_count++;  // Contar os pulsos do encoder
}

ISR(TIMER1_COMPA_vect)
{
	count++;
	if (count >= 100)  // 100 * 10ms = 1 segundo
	{
		// Calcular RPM
		cli();
		uint32_t temp = pulse_count;
		pulse_count = 0;
		sei();
		uint32_t rpm = (temp * 60) / PULSES_PER_REV;

		// Calcular o valor do PWM
		uint32_t pwm_value = (rpm * MAX_PWM) / MAX_RPM;
		if (pwm_value > MAX_PWM) {
			pwm_value = MAX_PWM;
		}
		OCR0A = pwm_value;  // Aplicar o PWM no PB0

		// Resetar contadores
		count = 0;
	}
}

void adc_init() {
	// Selecionar Vref = AVcc e selecionar o canal ADC3 (PB3)
	ADMUX = 0b00000011;
	
	// Habilitar o ADC, definir o prescaler para 64 (1MHz/64 = 15625Hz)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint16_t adc_read() {
	// Iniciar a conversão
	ADCSRA |= (1 << ADSC);
	
	// Aguardar a conversão completar
	while (ADCSRA & (1 << ADSC));
	
	// Retornar o valor ADC (10 bits)
	return ADC;
}


int main(void) {
	adc_init();
	Timer1_init();
	Timer0_init();

	// Configurar PB2 (INT0) como entrada e ativar o pull-up interno
	DDRB &= ~(1 << PB2);
	PORTB |= (1 << PB2);

	// Configurar a interrupção externa no PB2 (INT0) para borda de subida
	MCUCR |= (1 << ISC00) | (1 << ISC01);  // Configura INT0 para borda de subida
	GIMSK |= (1 << INT0);  // Habilita a interrupção INT0

	// Habilitar interrupções globais
	sei();

	
	while (1) {
		// Ler o valor do ADC (0-1023)
		uint16_t adc_value = adc_read();
		
		// Converter o valor ADC para 0-255 (8 bits)
		uint8_t valor_motor = adc_value >> 2;  // Equivalente a dividir por 4
		
		// Definir o valor do PWM
		OCR0B = valor_motor;
	}
}
