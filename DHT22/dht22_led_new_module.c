/*
 * DHT22 for Raspberry Pi with WiringPi
 * Original Author: Hyun Wook Choi
 * Original Version: 0.1.0
 * Forked from: https://github.com/ccoong7/DHT22
 */


#include <stdio.h>
#include <wiringPi.h>


static const unsigned short signal = 18; // BCM (physical 12)
unsigned short data[5] = {0, 0, 0, 0, 0};
static const unsigned short led_r = 20; // BCM (physical 38)

static const unsigned short led_on_temp = 26;

struct dht_data
{
	float humidity;
	float celsius;
	short checksum;
};

struct dht_data get_dht_data(const unsigned short pin_num);

short readData(const unsigned short pin_num)
{
	unsigned short val = 0x00;
	unsigned short signal_length = 0;
	unsigned short val_counter = 0;
	unsigned short loop_counter = 0;

	while (1)
	{
		// Count only HIGH signal
		while (digitalRead(pin_num) == HIGH)
		{
			signal_length++;

			// When sending data ends, high signal occur infinite.
			// So we have to end this infinite loop.
			if (signal_length >= 200)
			{
				return -1;
			}

			delayMicroseconds(1);
		}

		// If signal is HIGH
		if (signal_length > 0)
		{
			loop_counter++;	// HIGH signal counting

			// The DHT22 sends a lot of unstable signals.
			// So extended the counting range.
			if (signal_length < 10)
			{
				// Unstable signal
				val <<= 1;		// 0 bit. Just shift left
			}

			else if (signal_length < 30)
			{
				// 26~28us means 0 bit
				val <<= 1;		// 0 bit. Just shift left
			}

			else if (signal_length < 85)
			{
				// 70us means 1 bit
				// Shift left and input 0x01 using OR operator
				val <<= 1;
				val |= 1;
			}

			else
			{
				// Unstable signal
				return -1;
			}

			signal_length = 0;	// Initialize signal length for next signal
			val_counter++;		// Count for 8 bit data
		}

		// The first and second signal is DHT22's start signal.
		// So ignore these data.
		if (loop_counter < 3)
		{
			val = 0x00;
			val_counter = 0;
		}

		// If 8 bit data input complete
		if (val_counter >= 8)
		{
			// 8 bit data input to the data array
			data[(loop_counter / 8) - 1] = val;

			val = 0x00;
			val_counter = 0;
		}
	}
}


int main(void)
{
	struct dht_data dht_01;

	// GPIO Initialization
	if (wiringPiSetupGpio() == -1) // BCM
	{
		printf("[T-T] GPIO Initialization FAILED.\n");
		return -1;
	}

	pinMode(led_r, OUTPUT);

	for (unsigned char i = 0; i < 100; i++)
	{
        dht_01 = get_dht_data(signal);
		if (dht_01.celsius > 25) {
			digitalWrite(led_r, HIGH);
		} else {
			digitalWrite(led_r, LOW);
		}
	}

	return 0;
}



struct dht_data get_dht_data(const unsigned short pin_num)
{
	struct dht_data dht;
	pinMode(pin_num, OUTPUT);

	// Send out start signal
	digitalWrite(pin_num, LOW);
	delay(20);					// Stay LOW for 5~30 milliseconds
	pinMode(pin_num, INPUT);		// 'INPUT' equals 'HIGH' level. And signal read mode

	readData(pin_num);		// Read DHT22 signal

	// The sum is maybe over 8 bit like this: '0001 0101 1010'.
	// Remove the '9 bit' data using AND operator.
	dht.checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;

	// If Check-sum data is correct (NOT 0x00), display humidity and temperature
	if (data[4] == dht.checksum && dht.checksum != 0x00)
	{
		// * 256 is the same thing '<< 8' (shift).
		dht.humidity = ((data[0] * 256) + data[1]) / 10.0;
		
		// found that with the original code at temperatures > 25.4 degrees celsius
		// the temperature would print 0.0 and increase further from there.
		// Eventually when the actual temperature drops below 25.4 again
		// it would print the temperature as expected.
		// Some research and comparisin with other C implementation suggest a
		// different calculation of celsius.
		//celsius = data[3] / 10.0; //original
		dht.celsius = (((data[2] & 0x7F)*256) + data[3]) / 10.0; //Juergen Wolf-Hofer

		// If 'data[2]' data like 1000 0000, It means minus temperature
		if (data[2] == 0x80)
		{
			dht.celsius *= -1;
		}

		// fahrenheit = ((celsius * 9) / 5) + 32;

		// Display all data
		printf("TEMP: %6.2f *C (%6.2f *F) | HUMI: %6.2f %%\n\n", dht.celsius, dht.humidity);
		// if (dht.celsius > led_on_temp) {
		// 	digitalWrite(led_r, HIGH);
		// } else {
		// 	digitalWrite(led_r, LOW);
		// }
	}

	else
	{
		printf("[x_x] Invalid Data. Try again.\n\n");
	}

	// Initialize data array for next loop
	for (unsigned char i = 0; i < 5; i++)
	{
		data[i] = 0;
	}

	delay(2000);	// DHT22 average sensing period is 2 seconds

	return dht;
}