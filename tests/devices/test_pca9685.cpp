#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <pigpiod_if2.h>
#include "devices/pca9685.h"

using namespace std;

int getCh(void);
int getkey(void);

int getCh(void){
     int ch;
     struct termios oldt;
     struct termios newt;
     tcgetattr(STDIN_FILENO, &oldt); /*store old settings */
     newt = oldt; /* copy old settings to new settings */
     newt.c_lflag &= ~(ICANON | ECHO); /* make one change to old settings in new settings */
     tcsetattr(STDIN_FILENO, TCSANOW, &newt); /*apply the new settings immediatly */
     ch = getchar(); /* standard getchar call */
     tcsetattr(STDIN_FILENO, TCSANOW, &oldt); /*reapply the old settings */
     return ch; /*return received char */
}

int getKey(void){
     int x = ' ';
     int y = ' ';
     int z = ' ';

     x = getCh();
     if(x == 27){
          y = getCh();
          z = getCh();
          // printf("Key code y is %d\n", y);
          // printf("Key code z is %d\n", z);
     }

     if(x == 27 && y == 91){
          return z;
     }else
          return y;

}

int main(int argc, char *argv[]){
	cout << "[START] Testing PCA-9685 Demo." << endl;
     int err;
     int bus = 1;
     int add = 0x41;
     int width, channel, freq, flag_test;
     float duty;
     char c;
	cout << "[START] PigpioD..." << endl;
     int pi = pigpio_start(NULL,NULL);
	cout << "[START] PCA-9685..." << endl;
     PCA9685 pwm(pi, bus, add);

     cout << "Please enter what frequency you'd like to try (24Hz - 1000Hz)." << endl;
     cin >> freq;
     pwm.setFrequency(freq);

     cout << "What function would you like to test? 'p' for pulsewidth, or 'd' for dutycycle. Otherwise 'default' demo function will run." << endl;
     cin >> c;

     if(c == 'p') flag_test = 1;
     else if(c == 'd') flag_test = 0;
     else flag_test = 2;

     cout << "Which channel would you like to test" << endl;
     cin >> channel;

     float curDuty = 50.0;
     int curPulse = 1500;
     float delta = 5.0;
     while(1){

          if(flag_test == 0){
               cout << "Please enter the desired duty cycle (0 - 100%)" << endl;
               cin >> duty;
               pwm.setDutyCycle(channel,duty);
          }else if(flag_test == 1){
               cout << "Please enter the desired pulsewidth (in microseconds)" << endl;
               cin >> width;
               pwm.setPulsewidth(channel,width);
          }else if(flag_test == 2){
               int z = getKey();

               if(z == 65){
                    curDuty += delta;
                    curPulse += delta;
                    cout << "Current Duty/Pulsewidth: " << curDuty << "/" << curPulse << endl;
                    pwm.setPulsewidth(channel,curPulse);
                    // printf("Stopping...");
                    // pwm.setDutyCycle(channel,0.0);
               }else if(z == 66){
                    curDuty -= delta;
                    curPulse -= delta;
                    cout << "Current Duty/Pulsewidth: " << curDuty << "/" << curPulse << endl;
                    pwm.setPulsewidth(channel,curPulse);
                    // printf("Stopping...");
                    // pwm.setDutyCycle(channel,0.0);
               }else cout << "Recevied: " << z << endl;
          }
          // pwm.setDutyCycle(channel,0);

          // cout << "Would you like to exit? (y) or (n)" << endl;
          // cin >> c;
          // if(c == 'y')
          //      break;

     }

     return 0;
}
