#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <softPwm.h>


#define PIN 6

int main ()
{
  int pos = 10 ;
  int dir = 1 ;
  if (wiringPiSetupGpio() == -1) exit(1) ;  //init wiringPi

  pinMode(PIN, OUTPUT) ;  // 0번핀을 OUTPUT으로 설정. WiringPi의 0번핀은 BCM GPIO 17번핀에 해당한다
  digitalWrite(PIN, LOW) ;  // 0 pin output LOW voltage
  softPwmCreate(PIN, 0, 200) ;  // 0번 핀의 초기 pwm 값을 LOW로 하고 PWM주기는 20ms 로 설정한다

  // 10은 1ms, 15는 1.5ms(1500마이크로 초), 20은 2.0ms 이므로, HS-311기종의 경우, 10은 최저각, 15는 중립, 20은 최고각을 가리킨다
  while(1) {
    pos += dir ;
    if (pos < 10 || pos > 20) dir *= -1 ;
    softPwmWrite(PIN, pos) ;
    delay(190) ;
  }
  return 0 ;
}
