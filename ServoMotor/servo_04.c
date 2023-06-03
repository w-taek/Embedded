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

    int i;
    int dir = 1;
    int pos = 5;
    softPwmCreate(PIN, 0, 200);

    while(1) {
        pos += dir;
        if (pos < 5 || pos >25) {
            dir *= -1;
        }
        softPwmWrite(PIN, pos);
        delay(200);
    }
}