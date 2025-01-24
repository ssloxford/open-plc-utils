#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <sys/time.h>
#include <string.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>

#include "adc_wrapper.h"

#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED 2560000 // 2.56 MHz, must be 256MHz / even number
//To sample the 1k PWM, 24 bits per transaction, 106 samples per cycle

uint16_t ADC_CAL_M12 = 139;
uint16_t ADC_CAL_0 = 512;
uint16_t ADC_CAL_P12 = 884;

int spi_fd;

void spi_init(uint16_t cal_m12, uint16_t cal_0, uint16_t cal_p12) {
    ADC_CAL_M12 = cal_m12;
    ADC_CAL_0 = cal_0;
    ADC_CAL_P12 = cal_p12;

    spi_fd = open(SPI_DEVICE, O_RDWR, 0);
    if (spi_fd < 0) {
        perror("Error opening SPI device");
        return;
    }

    uint8_t tmp_mode, tmp_bpw;
    uint32_t tmp_speed;
    if (ioctl(spi_fd, SPI_IOC_RD_MODE, &tmp_mode) == -1) {
        return;
    }

    if (ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &tmp_bpw) == -1) {
        return;
    }

    if (ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &tmp_speed) == -1) {
        return;
    }

    printf("%i %i %i\n", tmp_mode, tmp_bpw, tmp_speed);


    // Set SPI mode (CPOL = 0, CPHA = 0)
    uint8_t mode = SPI_MODE_0;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("Error setting SPI mode");
        return;
    }

    // Set SPI bits per word (8 bits)
    uint8_t bits = 8;
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("Error setting SPI bits per word");
        return;
    }

    // Set SPI speed
    uint32_t speed = SPI_SPEED;
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("Error setting SPI speed");
        return;
    }
}

static inline void set_tx_buf(uint8_t channel, uint8_t* tx_buf) {
    tx_buf[0] = 1;
    tx_buf[1] = (8 + (channel & 3)) << 4;
    tx_buf[2] = 0;
    //printf("%i %i %i", tx_buf[0], tx_buf[1], tx_buf[2]);
}

static inline uint16_t get_rx_val(const uint8_t* rx_buf) {
    return ((rx_buf[1] & 0x03) << 8) | rx_buf[2];
}

uint16_t read_adc_single(uint8_t channel) {
    uint8_t tx_buf[3] = {0};
    uint8_t rx_buf[3] = {0};

    set_tx_buf(channel, tx_buf);

    struct spi_ioc_transfer transfer;
    memset(&transfer, 0, sizeof(transfer));

    transfer.tx_buf = (uint64_t)tx_buf;
    transfer.rx_buf = (uint64_t)rx_buf;
    transfer.len = 3;
    transfer.delay_usecs = 0;
    transfer.speed_hz = SPI_SPEED;
    transfer.bits_per_word = 8;
#ifdef SPI_IOC_WR_MODE32
    transfer.tx_nbits = 0;
#endif
#ifdef SPI_IOC_RD_MODE32
    transfer.rx_nbits = 0;
#endif

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer) < 0) {
        perror("Error during SPI transfer");

        return 0;
    }

    uint16_t res = get_rx_val(rx_buf);

    return res;
}

struct CP_state read_pwm_signal(uint8_t channel) {
    float level_high_sum = 0;
    //float level_high_smallest = 0;
    int level_high_cnt = 0;

    float level_low_sum = 0;
    //float level_low_largest = 0;
    int level_low_cnt = 0;

    struct timeval start, now;

    gettimeofday(&start, NULL);

    while (1) {
        uint16_t result = read_adc_single(channel);
        float resultf;
        //Manually obtained calibration
        if (result > ADC_CAL_0) {
            resultf = (result - ADC_CAL_0) * 12.f / (ADC_CAL_P12 - ADC_CAL_0);
            level_high_sum += resultf;
            //if(level_high_cnt == 0 || resultf < level_high_smallest) {
            //    level_high_smallest = resultf;
            //}
            level_high_cnt += 1;
        }
        else {
            resultf = -12.f + (result - ADC_CAL_M12) * 12.f / (ADC_CAL_0 - ADC_CAL_M12);
            level_low_sum += resultf;
            //if(level_low_cnt == 0 || level_low_largest < resultf) {
            //    level_low_largest = resultf;
            //}
            level_low_cnt += 1;
        }
        //printf("%i\n", result);
        //printf("%f\n", resultf);

        gettimeofday(&now, NULL);
        int micros;
        if (now.tv_sec > start.tv_sec) {
            micros = 1000000L;
        }
        else {
            micros = 0;
        }
        micros = micros + (now.tv_usec - start.tv_usec);
        if(micros > 250000) break;
        //usleep(1000);
    }

    float duty = level_high_cnt * 1.f / (level_high_cnt + level_low_cnt);

    //Remove outliers
    /*
    if(level_high_cnt > 0) {
        level_high_cnt -= 1;
        level_high_sum -= level_high_smallest;
    }

    if(level_low_cnt > 0) {
        level_low_cnt -= 1;
        level_low_sum -= level_low_largest;
    }*/

    return (struct CP_state) {
        .level_high = level_high_sum / level_high_cnt,
            .level_low = level_low_sum / level_low_cnt,
            .duty = duty
    };

}
