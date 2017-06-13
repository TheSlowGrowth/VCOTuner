/*
  ==============================================================================

    MainComponent.h
    Created: 17 May 2016 8:21:04pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "VCOTuner.h"
#include "Visualizer.h"

//==============================================================================
ApplicationProperties& getAppProperties();

//==============================================================================
/**
 */
class MainComponent: public Component,
                     public VCOTuner::Listener,
                     public Button::Listener,
                     public ComboBox::Listener
{
public:
    MainComponent();
    ~MainComponent();
    
    void resized() override;
    void paint(Graphics& g) override;
    
    virtual void buttonClicked (Button* bttn) override;
    virtual void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    
    virtual void tunerStarted() override;
    virtual void tunerStopped() override;
    virtual void tunerFinished() override;
    virtual void tunerStatusChanged(String statusString) override;
    
    void startCreatingReport();
    
private:
    //==============================================================================
    AudioDeviceManager deviceManager;
    VCOTuner tuner;
    
    void showAudioSettings();
    
    TextButton audioSettings;
    TextButton startStop;
    TextButton report;
    Visualizer display;
    Label statusLabel;
    Label regimeLabel;
    ComboBox regime;
    Label resolutionLabel;
    ComboBox resolution;
    
    typedef struct
    {
        int startNote;
        int endNote;
        int interval;
    } regime_t;
    static const int numRegimes = 12;
    static const regime_t regimes[numRegimes];
    static const char* regimeTexts[numRegimes];
    static const regime_t reportRange;
    static const int numResolutions = 5;
    static const int resolutions[numResolutions];
    static const char* resolutionsTexts[numResolutions];
    
    bool cycle;
    bool creatingReport;
    
    static const String welcomeText;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};



#endif  // MAINCOMPONENT_H_INCLUDED
