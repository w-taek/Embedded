#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <softPwm.h>


#define FAN 24

int main(void)
{
    if(wiringPiSetupGpio() == -1){
        printf("error\n");
        return 1;
    }

    pinMode(FAN, OUTPUT);

    digitalWrite (FAN, 1) ; // On
		
    delay (5000) ; // ms
    
    digitalWrite (FAN, 0) ; // Off
    
    delay (2000) ;
}
