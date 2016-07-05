/*
  ==============================================================================

    ReportPrepScreen.h
    Created: 2 Jul 2016 4:12:50pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef REPORTPREPSCREEN_H_INCLUDED
#define REPORTPREPSCREEN_H_INCLUDED


#include "../JuceLibraryCode/JuceHeader.h"
#include "ReportCreatorWindow.h"

class ReportPrepScreen: public Component,
                        public Timer,
                        public VCOTuner::Listener
{
public:
    ReportPrepScreen(VCOTuner* t, Visualizer* v, ReportCreatorWindow* p);
    ~ReportPrepScreen();
    
    void timerCallback() override;
    void paint(Graphics& g) override;
    
    virtual void tunerStopped() override;
    
private:
    VCOTuner* tuner;
    Visualizer* visualizer;
    ReportCreatorWindow* parent;
    
    double currentFreq;
    int millisecCounter;
};



#endif  // REPORTPREPSCREEN_H_INCLUDED
