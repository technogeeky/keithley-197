#ifndef _plotter_h
#define _plotter_h

void setup_plotter(Plotter& p,  double& x_0,  double& x,  double& y, double& x2, double mv[]);
void loop_plotter(Plotter& p, byte DMMReading[],  double& x_0,  double& x,  double& y, double& x2, double mv[]);
#endif
