#include <stdio.h>
#include <wiringPi.h>

static const unsigned short led_y = 29;
static const unsigned short led_r = 28;

int main() {
    if (wiringPiSetup() < 0) {
        printf("Unable to setup wiring Pi\n");
        return 1;
    }

    pinMode(led_r, OUTPUT);
    pinMode(led_y, OUTPUT);

    while (1) {
        // digitalWrite(led_r, HIGH);
        // delay(500);
        // digitalWrite(led_y, HIGH);
        // delay(500);

        digitalWrite(led_y, LOW);
        delay(500);
        digitalWrite(led_r, LOW);
        delay(500);

        // digitalWrite(led_r, HIGH);
        // delay(500);
        // digitalWrite(led_r, LOW);
        // delay(500);

        // digitalWrite(led_y, HIGH);
        // delay(500);
        // digitalWrite(led_y, LOW);
        // delay(500);
    }

    return 0;
}

