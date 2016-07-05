/*
  ==============================================================================

    ReportDetailsEditor.h
    Created: 24 May 2016 10:05:02pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef REPORTDETAILSEDITOR_H_INCLUDED
#define REPORTDETAILSEDITOR_H_INCLUDED

#include "VCOTuner.h"
#include "Visualizer.h"
#include "ReportCreatorWindow.h"

//==============================================================================
ApplicationProperties& getAppProperties();

//==============================================================================
/**
 */
class ReportDetailsEditorScreen: public Component,
                           public Button::Listener,
                           public VCOTuner::Listener
{
public:
    ReportDetailsEditorScreen(VCOTuner* t, Visualizer* v, ReportCreatorWindow* p);
    ~ReportDetailsEditorScreen();
    
    void buttonClicked (Button* bttn) override;
    void resized() override;
    void paint(Graphics& g) override;
    
    
    virtual void tunerStopped() override;
    virtual void tunerFinished() override;
    virtual void tunerStatusChanged(String statusString) override;
    
private:
    
    Label brandLabel;
    TextEditor brandEdit;
    Label deviceLabel;
    TextEditor deviceEdit;
    Label interfaceBrandLabel;
    TextEditor interfaceBrandEdit;
    Label interfaceLabel;
    TextEditor interfaceEdit;
    Label notesLabel;
    TextEditor notesEdit;
    
    TextButton submit;
    bool submitted;
    bool tunerHasFinished;
    
    Label instructions;
    Label status;
    
    VCOTuner* tuner;
    Visualizer* visualizer;
    ReportCreatorWindow* parent;
    
    enum State
    {
        measuring,
        reMeasuringReference
    } state;
    double initalReferenceFreq;
    
    JUCE_DECLARE_NON_COPYABLE(ReportDetailsEditorScreen)
};


#endif  // REPORTDETAILSEDITOR_H_INCLUDED
