#include <stdio.h>
#include <wiringPi.h>
#include <softPwm.h>

#define PIN 6

int main(void)
{
    if(wiringPiSetupGpio() == -1){
        printf("error\n");
        return 1;
    }

    for(int i=0; i<5; ++i){
        softPwmCreate(PIN, 0, 200);
        softPwmWrite(PIN, 5);
        delay(2000);
        softPwmWrite(PIN, 25);
        delay(2000);
    }
    
    return 0;
}