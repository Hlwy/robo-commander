#include <unistd.h>

#include <pigpiod_if2.h>
#include "vehicle_profiles/dual_roboclaw.h"

using namespace std;

int main(int argc, char *argv[]){

     vector<int32_t> cmds;
     vector<int32_t> cmds1;
     vector<int32_t> cmds2;

     int pi = pigpio_start(NULL, NULL); /* Connect to Pi. */

     if (pi >= 0){

          DualClaw claws(pi);
          claws.reset_encoders();

          // while(1){
          //      claws.update_status();
          //      claws.update_encoders();
          //      // cmds = claws.set_speeds(-0.1, 0.0);
          //      // claws.drive(cmds);
          //      usleep(0.1 * 1000000);
          // }

          claws.update_encoders();
          cmds1 = claws.set_speeds(1.0, 0.0);
          cmds2 = claws.set_speeds(0.0, 0.0);

          claws.drive(cmds1);
          usleep(1.0 * 1000000);
          claws.drive(cmds2);

          claws.update_encoders();

          pigpio_stop(pi);
     }

     return 0;
}