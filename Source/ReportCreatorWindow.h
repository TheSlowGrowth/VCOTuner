/*
  ==============================================================================

    ReportCreaterWindow.h
    Created: 2 Jul 2016 4:12:09pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef REPORTCREATERWINDOW_H_INCLUDED
#define REPORTCREATERWINDOW_H_INCLUDED


#include "VCOTuner.h"
#include "Visualizer.h"

//==============================================================================
ApplicationProperties& getAppProperties();

//==============================================================================
/**
 */
class ReportCreatorWindow: public Component
{
public:
    ReportCreatorWindow(VCOTuner* t, Visualizer* v);
    virtual ~ReportCreatorWindow();
    
    // switches to the next dialog
    void next();
    
    void resized() override;
private:
    enum State
    {
        prepareReport,
        editTextsAndDoMeasurements,
        displayResults
    } state;
    
    VCOTuner* tuner;
    Visualizer* visualizer;
    
    Component* currentContentComponent;
    
    JUCE_DECLARE_NON_COPYABLE(ReportCreatorWindow)
};


#endif  // REPORTCREATERWINDOW_H_INCLUDED
