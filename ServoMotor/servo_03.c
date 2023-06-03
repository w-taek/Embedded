#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <softPwm.h>


#define PIN 6

int main(void)
{
    if(wiringPiSetupGpio() == -1){
        printf("error\n");
        return 1;
    }

    pinMode(PIN, OUTPUT);
    softPwmCreate(PIN, 0, 200);
    int i = 0;
    for (int j = 0; j < 5; ++j) {
        softPwmWrite(PIN, i);
        delay(2000);
        i += 5;
        if (i == 60) {
            i = 0;
        }
    }
    i = 60;
    for (int j = 0; j < 5; ++j) {
        softPwmWrite(PIN, i);
        delay(2000);
        i -= 5;
        if (i == 0) {
            i = 60;
        }
    }
}