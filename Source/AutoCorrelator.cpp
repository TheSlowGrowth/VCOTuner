/*
  ==============================================================================

    AutoCorrelator.cpp
    Created: 7 Jul 2016 4:19:59pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "AutoCorrelator.h"

double AutoCorrelator::calculate(const AudioSampleBuffer& input, double tau)
{  
    double balance = tau - floor(tau);
    
    if (tau >= input.getNumSamples()/2)
        return -1;

    // needed to be able to do the interpolation
    int start = 1;
    int end = input.getNumSamples()/2 - 2;
    
    double accumulator = 0;
    for (int i = start; i < end; i++)
    {
        // only interpolate if necessary
        if (balance == 0)
        {
            accumulator += (input.getSample(0, i)) * (input.getSample(0, i + tau));
        }
        else
        {
            double y1 = input.getSample(0, i);
            double y2 = interpolate(input.getSample(0, floor(i + tau - 1)),
                                    input.getSample(0, floor(i + tau)),
                                    input.getSample(0, floor(i + tau + 1)),
                                    input.getSample(0, floor(i + tau + 2)),
                                    balance);
            
            accumulator += (y1 * y2);
        }
    }
    
    return accumulator;
}

double AutoCorrelator::interpolate(double y0, double y1, double balance)
{
    return y0 + ( y1 - y0 ) * balance;
}

double AutoCorrelator::interpolate(double y0, double y1, double y2, double x)
{
    // using newtons algorithm to fit a 2rd order polynomial
    double a0, a1, a2;
    
    a0 = y0;
    a1 = ( y1 - a0 );
    a2 = ( y2 - a0 ) * 0.5 - a1;
    
    // evaluate polynomial
    double result = a0 + a1 * x + a2 * x * ( x - 1 );
    return result;
}

double AutoCorrelator::interpolate(double y0, double y1, double y2, double y3, double balance_x1_x2)
{
    // using newtons algorithm to fit a 3rd order polynomial
    double a0, a1, a2, a3;
    
    a0 = y0;
    a1 = ( y1 - a0 );
    a2 = ( y2 - a0 ) * 0.5 - a1;
    a3 = 1.0 / 6.0 * ( y3 - a0 ) - 0.5 * a1 - a2;
    
    // evaluate polynomial
    double b0 = 1 + balance_x1_x2;
    double b1 = balance_x1_x2;
    double b2 = balance_x1_x2 - 1;
    
    double result = a0 + a1 * b0 + a2 * b0 * b1 + a3 * b0 * b1 * b2;
    return result;
}