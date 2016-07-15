/*
  ==============================================================================

    AutoCorrelator.h
    Created: 7 Jul 2016 4:19:59pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef AUTOCORRELATOR_H_INCLUDED
#define AUTOCORRELATOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class AutoCorrelator
{
public:
    
    static double calculate(const AudioSampleBuffer& input, double tau);
    static double interpolate(double a, double b, double balance);
    static double interpolate(double a, double b, double c, double balance);
    static double interpolate(double y0, double y1, double y2, double y3, double balance_x1_x2);
    
private:
    
};



#endif  // AUTOCORRELATOR_H_INCLUDED
