#include <stdint.h>

void spi_init(uint16_t cal_p12, uint16_t cal_0, uint16_t cal_m12);

uint16_t read_adc_single(uint8_t channel);

struct CP_state {
	float level_high;
	float level_low;
	float duty;
};
struct CP_state read_pwm_signal(uint8_t channel);