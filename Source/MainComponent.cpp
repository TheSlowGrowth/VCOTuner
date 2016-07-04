/*
  ==============================================================================

    MainComponent.cpp
    Created: 17 May 2016 8:21:04pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"
#include "ReportCreatorWindow.h"

MainComponent::MainComponent() : tuner(&deviceManager), display(&tuner)
{
    ScopedPointer<XmlElement> savedAudioState (getAppProperties().getUserSettings()
                                               ->getXmlValue ("audioDeviceState"));
    
    deviceManager.initialise (1, 0, savedAudioState, true);
    
    setVisible (true);
    
    Process::setPriority (Process::HighPriority);
    
    audioSettings.setName("AudioSettingsBttn");
    audioSettings.setButtonText("Audio Settings");
    audioSettings.addListener(this);
    addAndMakeVisible(&audioSettings);
    
    startStop.setName("StartStopBttn");
    startStop.setButtonText("Start");
    startStop.addListener(this);
    addAndMakeVisible(&startStop);
    
    report.setName("ReportBttn");
    report.setButtonText("Create Report");
    report.addListener(this);
    addAndMakeVisible(&report);
    
    statusLabel.setName("Status Label");
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(&statusLabel);
    
    regimeLabel.setName("Regime Label");
    regimeLabel.setText("Pitch range: ", dontSendNotification);
    regimeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(&regimeLabel);
    
    regime.setName("RegimeSelector");
    regime.addItemList(StringArray(regimeTexts, numRegimes), 1);
    regime.addListener(this);
    if (getAppProperties().getUserSettings()->containsKey("RegimeID"))
        regime.setSelectedId(getAppProperties().getUserSettings()->getIntValue("RegimeID"));
    else
        regime.setSelectedId(1);
    addAndMakeVisible(&regime);
    
    resolutionLabel.setName("Resolution Label");
    resolutionLabel.setText("Resolution: ", dontSendNotification);
    resolutionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(&resolutionLabel);
    
    resolution.setName("ResolutionSelector");
    resolution.addItemList(StringArray(resolutionsTexts, numResolutions), 1);
    resolution.addListener(this);
    if (getAppProperties().getUserSettings()->containsKey("ResolutionID"))
        resolution.setSelectedId(getAppProperties().getUserSettings()->getIntValue("ResolutionID"));
    else
        resolution.setSelectedId(1);
    addAndMakeVisible(&resolution);
    
    display.setName("ResultsDisplay");
    addAndMakeVisible(&display);
    
    tuner.addListener(this);
    tuner.addListener(&display);
    if (getAppProperties().getUserSettings()->containsKey("MIDIChannel"))
        tuner.setMidiChannel(getAppProperties().getUserSettings()->getIntValue("MIDIChannel"));
    else
        tuner.setMidiChannel(1);
    
    cycle = false;
    creatingReport = false;
    
    // for first-time starters, display a help message and the audio settings
    if ((!getAppProperties().getUserSettings()->containsKey("hideWelcomeScreen"))
        || (getAppProperties().getUserSettings()->getIntValue("hideWelcomeScreen") != 1))
    {
        NativeMessageBox::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "Welcome!", welcomeText);
        showAudioSettings();
        getAppProperties().getUserSettings()->setValue("hideWelcomeScreen", 1);
    }
}

MainComponent::~MainComponent()
{
    tuner.removeListener(this);
    getAppProperties().getUserSettings()->setValue("RegimeID", regime.getSelectedId());
    getAppProperties().getUserSettings()->setValue("ResolutionID", resolution.getSelectedId());
}


void MainComponent::resized()
{
    const int borderWidth = 10;
    const int buttonWidth = 100;
    const int buttonHeight = 20;
    
    audioSettings.setBounds(borderWidth, borderWidth, buttonWidth, buttonHeight);
    report.setBounds(getWidth() - buttonWidth - borderWidth, borderWidth, buttonWidth, buttonHeight);
    startStop.setBounds(report.getX() - buttonWidth - borderWidth, borderWidth, buttonWidth, buttonHeight);
    statusLabel.setBounds(audioSettings.getRight() + borderWidth,
                          borderWidth,
                          startStop.getX() - borderWidth - borderWidth - audioSettings.getRight(),
                          buttonHeight);
    
    regime.setBounds(getWidth() - 120 - borderWidth, audioSettings.getBottom() + borderWidth, 120, buttonHeight);
    regimeLabel.setBounds(regime.getX() - 80 - borderWidth, audioSettings.getBottom() + borderWidth, 80, buttonHeight);
    resolution.setBounds(regimeLabel.getX() - 120 - borderWidth, audioSettings.getBottom() + borderWidth, 120, buttonHeight);
    resolutionLabel.setBounds(resolution.getX() - 80 - borderWidth, audioSettings.getBottom() + borderWidth, 80, buttonHeight);
    
    display.setBounds(borderWidth,
                      regimeLabel.getBottom() + borderWidth,
                      getWidth() - 2 * borderWidth,
                      getHeight() - 2* borderWidth - regimeLabel.getBottom());

}

void MainComponent::paint(Graphics& g)
{
    g.setColour(Colours::lightgrey);
    g.fillAll();
}


void MainComponent::buttonClicked (Button* bttn)
{
    if (bttn == &audioSettings)
        showAudioSettings();
    else if (bttn == &startStop)
    {
        // re-apply the currently selected settings on a start.
        // this prevents the VCOTuner and the comboboxes being out of
        // sync after a report (when report is finished, the settings from the
        // report remain active but the comboboxes still show the old value)
        if (!tuner.isRunning())
        {
            comboBoxChanged(&regime);
            comboBoxChanged(&resolution);
        }
        tuner.toggleState();
        if (tuner.isRunning())
        {
            display.clearCache();
            cycle = true;
        }
    }
    else if (bttn == &report)
    {
        ReportCreatorWindow* reportWindow = new ReportCreatorWindow(&tuner, &display);
        
        DialogWindow::LaunchOptions o;
        o.content.setOwned (reportWindow);
        o.dialogTitle                   = "Create Report";
        o.componentToCentreAround       = this;
        o.dialogBackgroundColour        = Colours::lightgrey;
        o.escapeKeyTriggersCloseButton  = false;
        o.resizable                     = true;
        o.useNativeTitleBar             = true;
        
        o.launchAsync();
    }
}

void MainComponent::startCreatingReport()
{
    tuner.setNumMeasurementRange(reportRange.startNote, reportRange.interval, reportRange.endNote);
    display.clearCache();
    creatingReport = true;
}

void MainComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &regime)
    {
        bool wasRunning = false;
        bool wasCycling = cycle;
        if (tuner.isRunning())
        {
            wasRunning = true;
            tuner.toggleState();
        }
        
        int selected = comboBoxThatHasChanged->getSelectedId() - 1;
        tuner.setNumMeasurementRange(regimes[selected].startNote, regimes[selected].interval, regimes[selected].endNote);
        display.clearCache();
        
        
        if (wasRunning)
        {
            tuner.toggleState();
            cycle = wasCycling;
        }
    }
    else if (comboBoxThatHasChanged == &resolution)
    {
        bool wasRunning = false;
        bool wasCycling = cycle;
        if (tuner.isRunning())
        {
            wasRunning = true;
            tuner.toggleState();
        }
        
        int selected = comboBoxThatHasChanged->getSelectedId() - 1;
        tuner.setResolution(resolutions[selected]);        
        
        if (wasRunning)
        {
            tuner.toggleState();
            cycle = wasCycling;
        }
    }
}

void MainComponent::showAudioSettings()
{
    class SettingsWrapperComponent: public Component,
                                    public ComboBox::Listener
    {
    public:
        SettingsWrapperComponent(VCOTuner* tuner, AudioDeviceManager& m)
        : selectorComponent(m, 1, 1, 0, 0, false, true, false, false)
        {
            t = tuner;
            
            channelLabel.setName("MidiChannel Label");
            channelLabel.setText("MIDI Channel: ", dontSendNotification);
            channelLabel.setJustificationType(juce::Justification::centredRight);
            addAndMakeVisible(&channelLabel);
            
            channelEdit.setName("MidiChannel Edit");
            const char* items[16] = {"1", "2", "3", "4", "5", "6", "7", "8",
                                     "9", "10", "11", "12", "13", "14", "15", "16"};
            channelEdit.addItemList(StringArray(items, 16), 1);
            channelEdit.addListener(this);
            if (getAppProperties().getUserSettings()->containsKey("MIDIChannel"))
                channelEdit.setSelectedId(getAppProperties().getUserSettings()->getIntValue("MIDIChannel"));
            else
                channelEdit.setSelectedId(1);
            addAndMakeVisible(&channelEdit);
            
            addAndMakeVisible(&selectorComponent);
        }
        
        void comboBoxChanged (ComboBox* comboBoxThatHasChanged)
        {
            int channel = comboBoxThatHasChanged->getSelectedId();
            t->setMidiChannel(channel);
            getAppProperties().getUserSettings()->setValue("MIDIChannel", channel);
        }
        
        void resized()
        {
            const int height = selectorComponent.getItemHeight();
            const int border = 10;
            
            selectorComponent.setBounds(0, 0, getWidth(), getHeight() - 2*border - height);
            // selectorComponent overwrites its height in its resized() function. But it doesnt seem to work
            channelEdit.setBounds(proportionOfWidth (0.35f), selectorComponent.getBottom() + border, proportionOfWidth (0.6f), height);
            channelLabel.setBounds(0, selectorComponent.getBottom() + border, proportionOfWidth (0.35f), height);
        }
        
    private:
        AudioDeviceSelectorComponent selectorComponent;
        Label channelLabel;
        ComboBox channelEdit;
        VCOTuner* t;
    };
    
    SettingsWrapperComponent content(&tuner, deviceManager);
    content.setSize(400, 340);
    
    
    DialogWindow::LaunchOptions o;
    o.content.setNonOwned (&content);
    o.dialogTitle                   = "Audio & Midi Settings";
    o.componentToCentreAround       = this;
    o.dialogBackgroundColour        = Colours::lightgrey;
    o.escapeKeyTriggersCloseButton  = true;
    o.resizable                     = true;
    o.useNativeTitleBar             = true;
    
    o.runModal();
    
    ScopedPointer<XmlElement> audioState (deviceManager.createStateXml());
    
    getAppProperties().getUserSettings()->setValue ("audioDeviceState", audioState);
    getAppProperties().getUserSettings()->saveIfNeeded();
}


void MainComponent::tunerStarted()
{
    startStop.setButtonText("Stop");
}

void MainComponent::tunerStatusChanged(String statusString)
{
    if (creatingReport)
        statusLabel.setText("Creating Report: " + statusString, juce::dontSendNotification);
    else
        statusLabel.setText(statusString, juce::dontSendNotification);
}

void MainComponent::tunerStopped()
{
    StringArray errors = tuner.getLastErrors();
    for (int i = 0; i < errors.size(); i++)
        NativeMessageBox::showMessageBox(AlertWindow::WarningIcon, "Error!", errors[i]);
    
    startStop.setButtonText("Start");
    cycle = false;
    creatingReport = false;
}

void MainComponent::tunerFinished()
{
    startStop.setButtonText("Start");
    
    if (creatingReport)
    {
        creatingReport = false;
    }
    
    if (cycle)
        tuner.toggleState();
}

const MainComponent::regime_t MainComponent::regimes[numRegimes] = {
    {54, 66, 6},
    {54, 66, 3},
    {54, 66, 1},
    {48, 72, 12},
    {48, 72, 6},
    {48, 72, 1},
    {36, 84, 12},
    {36, 84, 6},
    {36, 84, 1},
    {24, 96, 12},
    {24, 96, 6},
    {24, 96, 1},
};
const char* MainComponent::regimeTexts[numRegimes] = {
    "narrow > coarse (54-66, +6)",
    "narrow > normal (54-66, +3)",
    "narrow > fine (54-66, +1)",
    "medium > coarse (48-72, +12)",
    "medium > normal (48-72, +6)",
    "medium > fine (48-72, +1)",
    "large > coarse (36-84, +12)",
    "large > normal (36-84, +6)",
    "large > fine (36-84, +1)",
    "huge > coarse (24-96, +12)",
    "huge > normal (24-96, +6)",
    "huge > fine (24-96, +1)",
};

const int MainComponent::resolutions[numResolutions] = {20, 50, 100, 200, 400};
const char* MainComponent::resolutionsTexts[numResolutions] = {
    "20 - quick & dirty",
    "50 - not quite enough",
    "100 - okay",
    "200 - neat and tidy",
    "400 - never accurate enough"
};

const MainComponent::regime_t MainComponent::reportRange = {24, 96, 1};

const String MainComponent::welcomeText = String("Welcome to the VCO Tuner!") + newLine + newLine + "Please follow these steps to get running:" + newLine + "1) connect a MIDI-CV interface to your Computer" + newLine + "2) connect the CV output of the interface to your oscillators frequency input" + newLine + "3) Connect one of the oscillators basic wavaforms (sine, saw, triangle, pulse, etc.) directly to your soundcard." + newLine + newLine + "When you close this dialog, the audio settings panel will open. Please select your audio and midi device there." + newLine + newLine + "Have fun!" + newLine + newLine + "PS: If you find any bugs, please raise an issue on the github repository under https://github.com/TheSlowGrowth/VCOTuner. Thanks!";

