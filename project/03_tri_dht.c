#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define DEBUG_PIN_CHECK (1)
#define DEBUG_FUNC_CHECK (0)

#define DHT_INNER_CNT   (2)
#define MAXTIMINGS      (85)
#define IN_DIFF_MAX     (2)
#define OUT_DIFF_MAX    (2)

struct dht_data
{
	float humidity;
	float celsius;
	short checksum;
	short valid;
};

// PIN Number
static const unsigned short dht22_12 = 18; // BCM (physical 12)
static const unsigned short dht11_35 = 19; // BCM - wPi(24), physical(35)
static const unsigned short dht11_37 = 26; // BCM - wPi(25), physical(37)
static const unsigned short dht11_pins[DHT_INNER_CNT] = {19, 26};
static const unsigned short led_r = 20; // BCM - physical(38) (fan)
static const unsigned short led_y = 21; // BCM - wPi(29),physical(38) (motor)


// global variables
unsigned short dht22_data[5] = {0, 0, 0, 0, 0};
int dht11_dat[5] = { 0, 0, 0, 0, 0 };

struct dht_data dht_data_out;
struct dht_data dht_datas_in[DHT_INNER_CNT];
// struct dht_data dht_data_indoor_avg;
// struct dht_data dht_data_indoor_avg;
// struct dht_data dht_data_indoor_avg;


static float calibration_cels = 0;
static float calibration_humid = 0;

// struct dht_data dht_data_inner_01;
// struct dht_data dht_data_inner_02;

//unsigned short data_[5] = {0, 0, 0, 0, 0};
//unsigned short data_[5] = {0, 0, 0, 0, 0};

// functions
short read_data_dht22(const unsigned short pin_num);
short get_dht22_data(const unsigned short pin_num, struct dht_data *data);
short get_dht11_data(const unsigned short pin_num, struct dht_data *data);
void calibrate(struct dht_data *data);
void get_indoor_avg(struct dht_data *data_1, struct dht_data *data_2);

int main(void)
{
    float innner_avg_cels;
    float innner_avg_humid;
    float inner_diff_cels;
    float inner_diff_humid;
    float outter_diff_cels;
    float outter_diff_humid;

    // GPIO Initialization
	if (wiringPiSetupGpio() == -1) { // BCM
		printf("GPIO Initialization FAILED.\n");
		return -1;
	}

    pinMode(led_r, OUTPUT);
    pinMode(led_y, OUTPUT);

    if (DEBUG_PIN_CHECK) {
        digitalWrite(led_r, HIGH);
        printf("checking led_r ...\n");
        delay(1000);
        digitalWrite(led_r, LOW);
        printf("checking led_r complete!!\n\n");

        digitalWrite(led_y, HIGH);
        printf("checking led_y ...\n");
        delay(1000);
        digitalWrite(led_y, LOW);
        printf("checking led_y complete!!\n\n");


        printf("\nchecking dht data init!!\n\n");
		get_dht22_data(dht22_12, &dht_data_out);
		while (dht_datas_in[0].valid == 0) {
			get_dht11_data(dht11_35, &dht_datas_in[0]);
			delay(2000);
		}
		while (dht_datas_in[1].valid == 0) {
			get_dht11_data(dht11_37, &dht_datas_in[1]);
			delay(2000);
		}

		innner_avg_cels = (dht_datas_in[0].celsius + dht_datas_in[1].celsius) / 2;
        innner_avg_humid = (dht_datas_in[0].humidity + dht_datas_in[1].humidity) / 2;
        inner_diff_cels = abs(dht_datas_in[0].celsius - dht_datas_in[1].celsius);
        inner_diff_humid = abs(dht_datas_in[0].humidity - dht_datas_in[1].humidity);
        outter_diff_cels = abs(dht_data_out.celsius - dht_data_out.celsius);
        outter_diff_humid = abs(dht_data_out.humidity - dht_data_out.humidity);
        
        printf("++----------------------init-------------------------++\n");
        printf("||   DHT #no   ||    Temperature    |    Humidity    ||\n");
        printf("||-------------||-------------------|----------------||\n");
        printf("||  DHT22_out  ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", dht_data_out.celsius, dht_data_out.humidity);
        printf("||  DHT11_in_1 ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", dht_datas_in[0].celsius, dht_datas_in[0].humidity);
        printf("||  DHT11_in_2 ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", dht_datas_in[1].celsius, dht_datas_in[1].humidity);
        printf("||  DHT_in_avg ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", innner_avg_cels, innner_avg_humid);
        printf("||-------------||-------------------|----------------||\n");
        printf("|| DHT_in_diff ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", inner_diff_cels, inner_diff_humid);
        printf("||-------------||-------------------|----------------||\n");
        printf("|| DHT_out_diff||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", outter_diff_cels, outter_diff_humid);
        printf("++---------------------------------------------------++\n\n\n");
		
		printf("\nsynchronizing innner sensors...\n\n");
		printf("|| DHT_in_diff ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n\n\n", inner_diff_cels, inner_diff_humid);
		calibration_cels = dht_datas_in[1].celsius - dht_datas_in[0].celsius;
		calibration_humid = dht_datas_in[1].humidity - dht_datas_in[0].humidity;

    }

    while (1) {
		get_dht22_data(dht22_12, &dht_data_out);
		get_dht11_data(dht11_35, &dht_datas_in[0]);
		delay(2000);
		get_dht11_data(dht11_37, &dht_datas_in[1]);
		delay(2000);
		calibrate(&dht_datas_in[1]);
        // dht_data_outter = get_dht22_data(dht22_12);
        // dht_datas_inner[0] = get_dht11_data(dht11_35);
        // delay(2000);
        // dht_datas_inner[1] = get_dht11_data(dht11_37);
        // delay(2000);
		// for (int i = 0; i < DHT_INNER_CNT; ++i) {
        //     dht_datas_inner[i] = get_dht11_data(dht11_pins[i]);
        // }

        innner_avg_cels = (dht_datas_in[0].celsius + dht_datas_in[1].celsius) / 2;
        innner_avg_humid = (dht_datas_in[0].humidity + dht_datas_in[1].humidity) / 2;
        inner_diff_cels = abs(dht_datas_in[0].celsius - dht_datas_in[1].celsius);
        inner_diff_humid = abs(dht_datas_in[0].humidity - dht_datas_in[1].humidity);
        outter_diff_cels = abs(dht_data_out.celsius - dht_data_out.celsius);
        outter_diff_humid = abs(dht_data_out.humidity - dht_data_out.humidity);
        
        printf("++---------------------------------------------------++\n");
        printf("||   DHT #no   ||    Temperature    |    Humidity    ||\n");
        printf("||-------------||-------------------|----------------||\n");
        printf("||  DHT22_out  ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", dht_data_out.celsius, dht_data_out.humidity);
        printf("||  DHT11_in   ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", dht_datas_in[0].celsius, dht_datas_in[0].humidity);
        printf("||  DHT11_in   ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", dht_datas_in[1].celsius, dht_datas_in[1].humidity);
        printf("||  DHT_in_avg ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", innner_avg_cels, innner_avg_humid);
        printf("||-------------||-------------------|----------------||\n");
        printf("|| DHT_in_diff ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", inner_diff_cels, inner_diff_humid);
        printf("||-------------||-------------------|----------------||\n");
        printf("|| DHT_out_diff||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", outter_diff_cels, outter_diff_humid);
        printf("++---------------------------------------------------++\n\n\n");

        if (innner_avg_cels > IN_DIFF_MAX) {
            digitalWrite(led_r, HIGH);
        } else {
            digitalWrite(led_r, LOW);
        }

        if (outter_diff_cels > OUT_DIFF_MAX) {
            digitalWrite(led_y, HIGH);
        } else {
            digitalWrite(led_y, LOW);
        }

        delay(5000);
    }

    return 0;
}














short read_data_dht22(const unsigned short pin_num)
{
	unsigned short val = 0x00;
	unsigned short signal_length = 0;
	unsigned short val_counter = 0;
	unsigned short loop_counter = 0;

	while (1)
	{
		// Count only HIGH signal
		while (digitalRead(pin_num) == HIGH) {
			signal_length++;

			// When sending data ends, high signal occur infinite.
			// So we have to end this infinite loop.
			if (signal_length >= 200) {
				return -1;
			}

			delayMicroseconds(1);
		}

		// If signal is HIGH
		if (signal_length > 0) {
			loop_counter++;	// HIGH signal counting

			// The DHT22 sends a lot of unstable signals.
			// So extended the counting range.
			if (signal_length < 10) {
				// Unstable signal
				val <<= 1;		// 0 bit. Just shift left
			} else if (signal_length < 30) {
				// 26~28us means 0 bit
				val <<= 1;		// 0 bit. Just shift left
			} else if (signal_length < 85) {
				// 70us means 1 bit
				// Shift left and input 0x01 using OR operator
				val <<= 1;
				val |= 1;
			} else {
				// Unstable signal
				return -1;
			}

			signal_length = 0;	// Initialize signal length for next signal
			val_counter++;		// Count for 8 bit data
		}

		// The first and second signal is DHT22's start signal.
		// So ignore these data.
		if (loop_counter < 3) {
			val = 0x00;
			val_counter = 0;
		}

		// If 8 bit data input complete
		if (val_counter >= 8)
		{
			// 8 bit data input to the data array
			dht22_data[(loop_counter / 8) - 1] = val;

			val = 0x00;
			val_counter = 0;
		}
	}
}

short get_dht22_data(const unsigned short pin_num, struct dht_data *data)
{
	//struct dht_data dht;
	pinMode(pin_num, OUTPUT);

	// Send out start signal
	digitalWrite(pin_num, LOW);
	delay(20);					// Stay LOW for 5~30 milliseconds
	pinMode(pin_num, INPUT);		// 'INPUT' equals 'HIGH' level. And signal read mode

	read_data_dht22(pin_num);		// Read DHT22 signal

	// The sum is maybe over 8 bit like this: '0001 0101 1010'.
	// Remove the '9 bit' data using AND operator.
	data->checksum = (dht22_data[0] + dht22_data[1] + dht22_data[2] + dht22_data[3]) & 0xFF;

	// If Check-sum data is correct (NOT 0x00), display humidity and temperature
	if (dht22_data[4] == data->checksum && data->checksum != 0x00) {
		data->valid = 1;
		// * 256 is the same thing '<< 8' (shift).
		data->humidity = ((dht22_data[0] * 256) + dht22_data[1]) / 10.0;
		
		// found that with the original code at temperatures > 25.4 degrees celsius
		// the temperature would print 0.0 and increase further from there.
		// Eventually when the actual temperature drops below 25.4 again
		// it would print the temperature as expected.
		// Some research and comparisin with other C implementation suggest a
		// different calculation of celsius.
		//celsius = data[3] / 10.0; //original
		data->celsius = (((dht22_data[2] & 0x7F)*256) + dht22_data[3]) / 10.0; //Juergen Wolf-Hofer

		// If 'data[2]' data like 1000 0000, It means minus temperature
		if (dht22_data[2] == 0x80) {
			data->celsius *= -1;
		}

		// Display all data
		//printf("TEMP: %6.2f *C (%6.2f *F) | HUMI: %6.2f %%\n\n", dht.celsius, dht.humidity);
	} else {
		//printf("[T_T] Invalid Data. Try again.\n\n");
		data->valid = 0;
	}

	// Initialize data array for next loop
	for (unsigned char i = 0; i < 5; i++) {
		dht22_data[i] = 0;
	}

	delay(2000);	// DHT22 average sensing period is 2 seconds

	return 0;
}

short get_dht11_data(const unsigned short pin_num, struct dht_data *data)
{
    if (DEBUG_FUNC_CHECK) {
        printf("entered get_dht11_data()\n");
    }

	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j		= 0, i;

	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    /* pull pin down for 18 milliseconds */
	pinMode(pin_num, OUTPUT);
	digitalWrite(pin_num, LOW);
	delay(18);
	/* prepare to read the pin */
	pinMode(pin_num, INPUT);

	/* detect change and read data */
	for ( i = 0; i < MAXTIMINGS; i++ ) {
		counter = 0;
		while (digitalRead(pin_num) == laststate) {
			counter++;
			delayMicroseconds(1);
			if (counter == 255) {
				break;
			}
		}
		laststate = digitalRead(pin_num);

		if (counter == 255) {
			break;
        }

		/* ignore first 3 transitions */
		if ((i >= 4) && (i % 2 == 0)) {
			/* shove each bit into the storage bytes */
			dht11_dat[j / 8] <<= 1;
			if (counter > 55) {
				dht11_dat[j / 8] |= 1;
            }
			j++;
		}
	}

	/* check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
	 * print it out if data is good
	 */
	if ( (j >= 40) &&
	     (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) ) {

		data->valid = 1;
		data->celsius = dht11_dat[2] + dht11_dat[3] / 10;
        data->humidity = dht11_dat[0] + dht11_dat[1] / 10;
        // if (DEBUG_FUNC_CHECK) {
        //     printf("in the funcion \"get_dht11_data(%d)\"...\n", pin_num);
        //     printf( "Humidity = %d.%d %% Temperature = %d.%d *C \n",
		// 	dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
        // }
        if (DEBUG_FUNC_CHECK) {
            printf("in the funcion \"get_dht11_data(%d)\"...\n", pin_num);
            printf("TEMP: %6.2f *C (%6.2f *F) | HUMI: %6.2f %%\n\n", data->celsius, data->humidity);
        }
		return 0;
	} else {
		data->valid = 0;
		return -1;

        if (DEBUG_FUNC_CHECK) {
            printf("in the funcion \"get_dht11_data(%d)\"...\n", pin_num);
		    printf( "Data not good, skip\n\n" );
			
			
        }
	}
}

void calibrate(struct dht_data *data)
{
	data->celsius -= calibration_cels;
	data->humidity -= calibration_humid;
}

// void get_dht_inner_avg(struct dht_data *p_data)
// {
//     for(int i = 0; i < DHT_INNER_CNT; ++i) {
//         p_data->
//     }
// }