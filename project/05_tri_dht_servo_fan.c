#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <signal.h>

#define DEBUG (1)
#define DEBUG_PIN_CHECK (1)
#define DEBUG_FUNC_CHECK (0)

#define DHT_INNER_CNT   (2)
#define MAXTIMINGS      (85)
#define IN_DIFF_MAX_T	(3)
#define OUT_DIFF_MAX_T	(3)
#define IN_DIFF_MAX_H 	(20)
#define OUT_DIFF_MAX_H	(10)

#define MOTOR_CLOSE		(1)
#define MOTOR_OPEN		(0)
#define CLOSED			(0)
#define OPENED			(1)

#define TURN_ON		(1)
#define TURN_OFF	(0)
#define FAN_ON		(1)
#define FAN_OFF		(0)

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
static const unsigned short servo = 6; // BCM - wPi(22),physical(31) (motor)
static const unsigned short fan = 24; // BCM - physical(7) (fan)

// global variables
unsigned short dht22_data[5] = {0, 0, 0, 0, 0};
int dht11_dat[5] = { 0, 0, 0, 0, 0 };

struct dht_data dht_data_out;
struct dht_data dht_datas_in[DHT_INNER_CNT];
struct dht_data dht_data_in_avg;
struct dht_data dht_data_in_diff;
struct dht_data dht_data_out_diff;
struct dht_data dht_data_cali;

struct dht_data *out = &dht_data_out;
struct dht_data *in_1 = &dht_datas_in[0];
struct dht_data *in_2 = &dht_datas_in[1];
struct dht_data *in_avg = &dht_data_in_avg;
struct dht_data *in_diff = &dht_data_in_diff;
struct dht_data *out_diff = &dht_data_out_diff;
struct dht_data *cali = &dht_data_cali;
float out_error = 0;

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
void set_all_datas();
void set_cali_val();

void control_servo_motor(short option);
void control_fan(short option);

void sig_handler(int signo); // exit function

int main(void)
{
    signal(SIGINT, (void *)sig_handler);

    // GPIO Initialization
	if (wiringPiSetupGpio() == -1) { // BCM
		printf("GPIO Initialization FAILED.\n");
		return -1;
	}

    pinMode(led_r, OUTPUT);
    pinMode(led_y, OUTPUT);
	softPwmCreate(servo, 0, 200);
	pinMode(fan, OUTPUT);

    if (DEBUG_PIN_CHECK) {
		printf("\n\n******************* SENSOR CHECKING ********************\n\n");

		digitalWrite(led_r, LOW);
		digitalWrite(led_y, LOW);
		digitalWrite(fan, LOW);
		delay(2000);
				
        digitalWrite(led_r, HIGH);
        printf("checking led_r ...\n");
        delay(1000);
        digitalWrite(led_r, LOW);
        printf("checking led_r complete!!\n\n\n");
		delay(1000);

        digitalWrite(led_y, HIGH);
        printf("checking led_y ...\n");
        delay(1000);
        digitalWrite(led_y, LOW);
        printf("checking led_y complete!!\n\n\n");
		delay(1000);

		printf("checking servo motor ...\n");
		printf("checking open ...\n");
		control_servo_motor(MOTOR_OPEN);
		printf("checking close ...\n");
		control_servo_motor(MOTOR_CLOSE);
		
		printf("checking servo motor complete!!\n\n");
		delay(1000);

		// digitalWrite(fan, HIGH);
        // printf("checking fan ...\n");
        // delay(3000);
        // digitalWrite(fan, LOW);
        // printf("checking fan complete!!\n\n\n");
		control_fan(TURN_ON);
        printf("checking fan ...\n");
        delay(3000);
        control_fan(TURN_OFF);
        printf("checking fan complete!!\n\n\n");

		printf("checking all of PINs complete!!\n\n");
		delay(1000);
		printf("*******************************************************\n\n\n\n");
	}

	if (DEBUG) {
        printf("\n\n********************* SENSOR INIT *********************\n\n");
		while (out->valid == 0) {
			get_dht22_data(dht22_12, out);
			delay(2000);
		}
		while (in_1->valid == 0) {
			get_dht11_data(dht11_35, in_1);
			delay(2000);
		}
		while (in_2->valid == 0) {
			get_dht11_data(dht11_37, in_2);
			delay(2000);
		}

		set_all_datas();
        
        printf("++----------------------init-------------------------++\n");
        printf("||   DHT #no   ||    Temperature    |    Humidity    ||\n");
        printf("++-------------++-------------------+----------------++\n");
        printf("||  DHT22_out  ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", out->celsius, out->humidity);
        printf("||  DHT11_in_1 ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_1->celsius, in_1->humidity);
        printf("||  DHT11_in_2 ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_2->celsius, in_2->humidity);
        printf("||  DHT_in_avg ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_avg->celsius, in_avg->humidity);
        printf("++-------------++-------------------|----------------++\n");
        printf("|| DHT_in_diff ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_diff->celsius, in_diff->humidity);
        printf("++-------------++-------------------+----------------++\n");
        printf("|| DHT_out_diff||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", out_diff->celsius, out_diff->humidity);
        printf("++-------------++-------------------+----------------++\n\n");
		
		printf("synchronizing indoor sensors...\n\n");
        printf("++-------------++-------------------+----------------++\n");
        printf("|| DHT_in_diff ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_diff->celsius, in_diff->humidity);
        printf("++-------------++-------------------+----------------++\n\n");

		printf("synchronizing outdoor sensors...\n\n");
		printf("++-------------++-------------------+----------------++\n");
        printf("|| DHT_out_diff||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", out_diff->celsius, out_diff->humidity);
        printf("++-------------++-------------------+----------------++\n\n");

		set_cali_val();
		out_error = out->celsius - in_avg->celsius;
		printf("synchronizing innner sensors complete!!\n\n");
		printf("*******************************************************\n");

    } else {
		get_dht22_data(dht22_12, out);
		while (in_1->valid == 0) {
			get_dht11_data(dht11_35, in_1);
			delay(2000);
		}
		while (in_2->valid == 0) {
			get_dht11_data(dht11_37, in_2);
			delay(2000);
		}

		set_all_datas();
		set_cali_val();
	}
	
	delay(3000);
	printf("\n\n\n\n\n\n\n\n\n\n\n\n");
	printf("*******************************************************\n");
	printf("********************* START SYSTEM ********************\n");
	printf("*******************************************************\n\n");

    while (1) {
		printf("\n\n********************* GETTING DATA ********************\n\n");
		get_dht22_data(dht22_12, &dht_data_out);
		get_dht11_data(dht11_35, &dht_datas_in[0]);
		delay(2000);
		get_dht11_data(dht11_37, &dht_datas_in[1]);
		delay(2000);
		calibrate(&dht_datas_in[1]);

		set_all_datas();
		
        printf("++--------------------- GET DATA --------------------++\n");
        printf("||   DHT #no   ||    Temperature    |    Humidity    ||\n");
        printf("++-------------++-------------------+----------------++\n");
        printf("||  DHT22_out  ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", out->celsius, out->humidity);
        printf("||  DHT11_in_1 ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_1->celsius, in_1->humidity);
        printf("||  DHT11_in_2 ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_2->celsius, in_2->humidity);
        printf("||  DHT_in_avg ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_avg->celsius, in_avg->humidity);
        printf("++-------------++-------------------+----------------++\n");
        printf("|| DHT_in_diff ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_diff->celsius, in_diff->humidity);
        printf("++-------------++-------------------+----------------++\n");
        printf("|| DHT_out_diff||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", out_diff->celsius, out_diff->humidity);
        printf("++-------------++-------------------+----------------++\n\n");

        if (in_diff->celsius > IN_DIFF_MAX_T || in_diff->humidity > IN_DIFF_MAX_H) {
			if (DEBUG && in_diff->valid == FAN_OFF) {
				printf("++---------------------------------------------------++\n");
				printf("||            indoor difference is greater           ||\n");
				printf("||                  tmeporary led red                ||\n");
				printf("||                    fan turn on                    ||\n");
        		printf("++-------------++-------------------+----------------++\n");
				printf("|| DHT_in_diff ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_diff->celsius, in_diff->humidity);
        		printf("++-------------++-------------------+----------------++\n");
			}
			control_fan(TURN_ON);
            digitalWrite(led_r, HIGH);
        } else {
			if (DEBUG && in_diff->valid == FAN_ON) {
				printf("++---------------------------------------------------++\n");
				printf("||              indoor difference is less            ||\n");
				printf("||                  tmeporary led red                ||\n");
				printf("||                    fan turn off                   ||\n");
        		printf("++-------------++-------------------+----------------++\n");
				printf("|| DHT_in_diff ||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", in_diff->celsius, in_diff->humidity);
        		printf("++-------------++-------------------+----------------++\n");
			}
			control_fan(TURN_OFF);
            digitalWrite(led_r, LOW);
        }

		if (out_diff->celsius > OUT_DIFF_MAX_T) {
			if (DEBUG && out_diff->valid == CLOSED) {
				printf("++---------------------------------------------------++\n");
				printf("||            outdoor difference is greater          ||\n");
				printf("||                     led yellow                    ||\n");
				printf("||                     open window                   ||\n");
        		printf("++-------------++-------------------+----------------++\n");
        		printf("|| DHT_out_diff||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", out_diff->celsius, out_diff->humidity);
        		printf("++-------------++-------------------+----------------++\n\n");
			}
            digitalWrite(led_y, HIGH);
			control_servo_motor(MOTOR_OPEN);
			//control_servo_motor(MOTOR_CLOSE);
			//softPwmWrite(servo, 5);
			//delay(5000);
        } else {
			if (DEBUG && out_diff->valid == OPENED) {
				printf("++---------------------------------------------------++\n");
				printf("||              outdoor difference is less           ||\n");
				printf("||                     led yellow                    ||\n");
				printf("||                    close window                   ||\n");
        		printf("++-------------++-------------------+----------------++\n");
        		printf("|| DHT_out_diff||  TEMP: %6.2f *C  | HUMI: %6.2f %% ||\n", out_diff->celsius, out_diff->humidity);
        		printf("++-------------++-------------------+----------------++\n\n");
			}
			control_servo_motor(MOTOR_CLOSE);
            digitalWrite(led_y, LOW);
        }

        delay(3000);
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

		data->celsius -= out_error;

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

void set_cali_val()
{
	if (cali->valid == 0) {
		cali->celsius = in_2->celsius - in_1->celsius;
		cali->humidity = in_2->humidity - in_1->humidity;
		cali->valid = 1;
	} else {
		if (DEBUG) {
			printf("\n\n[X_X] Already set calibration value!! ***\n\n");
		}
	}
}

void calibrate(struct dht_data *data)
{
	if (cali->valid == 1) {
		data->celsius -= cali->celsius;
		data->humidity -= cali->humidity;
	} else {
		if (DEBUG) {
			printf("\n\n[X_X] The calibration hasn't progressed yet!! ***\n\n");
		}
	}
}

void set_all_datas()
{
	in_avg->celsius = (in_1->celsius + in_2->celsius) / 2;
	in_avg->humidity = (in_1->humidity + in_2->humidity) / 2;
	in_diff->celsius = abs(in_1->celsius - in_2->celsius);
	in_diff->humidity = abs(in_1->humidity - in_2->humidity);
	out_diff->celsius = abs(in_avg->celsius - out->celsius);
	out_diff->humidity = abs(in_avg->humidity - out->humidity);
}

void control_servo_motor(short option)
{
	int i;
    int dir = 1;
    int pos = 5;

	//softPwmCreate(servo, 0, 200);
	if (option == MOTOR_OPEN) {
		if (out_diff->valid == OPENED) {
			printf("\n[X_X] Already Window has been opened!! ***\n\n");
			return;
		}
		while(!(pos < 5 || pos >25)) {
			pos += dir;
			softPwmWrite(servo, pos);
			delay(200);			
		}
		softPwmWrite(servo, 25);
		out_diff->valid = OPENED;
		return;
	} else if (option == MOTOR_CLOSE) {
		if (out_diff->valid == CLOSED) {
			printf("\n[X_X] Already Window has been closed!! ***\n\n");
			return;
		}
		dir *= -1;
		pos = 25;
		while(!(pos < 5 || pos > 25)) {
			pos += dir;
			softPwmWrite(servo, pos);
			delay(200);
		}
		softPwmWrite(servo, 5);
		out_diff->valid = CLOSED;
		return;
	} else {
		printf("\n[X_X] Invalid option!! ***\n\n");
	}
}

void control_fan(short option)
{
	if (option == TURN_ON) {
		if (in_diff->valid == FAN_ON) {
			printf("\n[X_X] Already FAN is working!! ***\n\n");
			return;
		}
		digitalWrite(fan, HIGH);
		delay(1000);
		in_diff->valid = FAN_ON;
		return;
	} else if (option == TURN_OFF) {
		if (in_diff->valid == FAN_OFF) {
			printf("\n[X_X] Already FAN is NOT working!! ***\n\n");
			return;
		}
		digitalWrite(fan, LOW);
		delay(1000);
		in_diff->valid = FAN_OFF;
	} else {
		printf("\n[X_X] Invalid option!! ***\n\n");
	}
}

void sig_handler(int signo) // 마지막 종료 함수 void sig_handler(int signo)
{
	printf("\n\n\n\n\n\n\n\n");
	printf("*******************************************************\n");
	printf("********************** EXIT SYSTEM ********************\n");
	printf("*******************************************************\n\n");
	
	digitalWrite(led_r, LOW);
	digitalWrite(led_y, LOW);
	control_servo_motor(MOTOR_CLOSE);
	control_fan(TURN_OFF);
	exit(0);
}