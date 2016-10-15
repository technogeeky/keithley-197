#include "Arduino.h"
#include "Plotter.h"
#include "k197.h"

void setup_plotter(Plotter& p,  double& x_0,  double& x,  double& y, double& x2, double mv[]) {

  p.AddXYGraph("x_y graph (ms vs uA)", 1000, "ms on x_y", x, "uV on x_y", y);
  
  x_0 = millis();
  
  p.AddTimeGraph("x_t graph (time vs uA)", 1000, "uV on x_t", x2);

  p.AddTimeGraph("multivariate", 1000, "a (green)", mv[0], "b (red)", mv[1], "c (blue)", mv[2], "d (yellow)", mv[3], "e (orange)", mv[4]);
  
}

void loop_plotter(Plotter& p, byte DMMReading[],  double& x_0,  double& x,  double& y, double& x2, double mv[]) {
  String measure = get_digits(DMMReading,false);
  x = millis() - x_0;
  y = measure.substring(1).toDouble() * 1000; 

  x2 = y;

  mv[0] = 3*cos( 2.0*PI*( millis()/2500.0 ) );
  mv[1] = 4.0;
  mv[2] = 10*sin( 2.0*PI*( millis() / 5000.0 ) );
  mv[3] = 7*cos( 2.0*PI*( millis() / 5000.0 ) );
  mv[4] = 5*sin( 2.0*PI*( millis() / 5000.0 ) );


  p.Plot();
   
}
