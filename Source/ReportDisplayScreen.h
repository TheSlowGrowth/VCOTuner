/*
  ==============================================================================

    ReportDisplayScreen.h
    Created: 2 Jul 2016 4:12:22pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef REPORTDISPLAYSCREEN_H_INCLUDED
#define REPORTDISPLAYSCREEN_H_INCLUDED

#include "ReportCreatorWindow.h"

class ReportDisplayScreen: public Component,
                           public Button::Listener
{
public:
    ReportDisplayScreen(VCOTuner* t, Visualizer* v, ReportCreatorWindow* parent);
    virtual ~ReportDisplayScreen() override;
    
    void buttonClicked (Button* bttn) override;
    void resized() override;
    void paint(Graphics& g) override;
    
private:
    void drawReport();
    
    VCOTuner* tuner;
    Visualizer* visualizer;
    ReportCreatorWindow* parent;
  
    Image img;
    TextButton save;
    TextButton close;
};


#endif  // REPORTDISPLAYSCREEN_H_INCLUDED
