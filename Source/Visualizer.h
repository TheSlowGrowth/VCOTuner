/*
  ==============================================================================

    Visualizer.h
    Created: 22 May 2016 2:45:34pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef VISUALIZER_H_INCLUDED
#define VISUALIZER_H_INCLUDED

#include "VCOTuner.h"

class Visualizer: public Component,
                  public VCOTuner::Listener
{
public:
    Visualizer();
    ~Visualizer();
    
    void paintWithFixedScaling(Graphics& g, int width, int height, double min, double max);
    void paint(Graphics& g, int width, int height);
    virtual void paint(Graphics& g);
    
    virtual void newMeasurementReady(const VCOTuner::measurement_t& m);
    
    void clearCache() { measurements.clear(); }
private:
    /** holds the list of completed measurements */
    Array<VCOTuner::measurement_t> measurements;
};


#endif  // VISUALIZER_H_INCLUDED
