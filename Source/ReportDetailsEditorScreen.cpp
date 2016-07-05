/*
  ==============================================================================

    ReportDetailsEditorScreen.cpp
    Created: 24 May 2016 10:05:02pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ReportDetailsEditorScreen.h"
#include "ReportProperties.h"

ReportDetailsEditorScreen::ReportDetailsEditorScreen(VCOTuner* t, Visualizer* v, ReportCreatorWindow* p)
{
    tuner = t;
    visualizer = v;
    parent = p;
    
    submitted = false;
    tunerHasFinished = false;
    
    if (tuner->isRunning())
        tuner->toggleState();
    
    instructions.setText(String("The measurements are running. This can take a while. You can watch the progress")  + newLine + "at the bottom of this window. Meanwhile, please fill in some information about your setup.", dontSendNotification);
    instructions.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(&instructions);
    
    brandLabel.setName("BrandLabel");
    brandLabel.setJustificationType(Justification::centredRight);
    brandLabel.setText("Device Brand:", dontSendNotification);
    addAndMakeVisible(&brandLabel);
    
    brandEdit.setName("BrandEdit");
    if (getAppProperties().getUserSettings()->containsKey("DUT-Brand"))
        brandEdit.setText(getAppProperties().getUserSettings()->getValue("DUT-Brand"), dontSendNotification);
    else
        brandEdit.setText(" - ", dontSendNotification);
    addAndMakeVisible(&brandEdit);
    
    deviceLabel.setName("DeviceLabel");
    deviceLabel.setJustificationType(Justification::centredRight);
    deviceLabel.setText("Device Type:", dontSendNotification);
    addAndMakeVisible(&deviceLabel);
    
    deviceEdit.setName("deviceEdit");
    if (getAppProperties().getUserSettings()->containsKey("DUT-Device"))
        deviceEdit.setText(getAppProperties().getUserSettings()->getValue("DUT-Device"), dontSendNotification);
    else
        deviceEdit.setText(" - ", dontSendNotification);
    addAndMakeVisible(&deviceEdit);
    
    interfaceBrandLabel.setName("InterfaceBrandLabel");
    interfaceBrandLabel.setJustificationType(Justification::centredRight);
    interfaceBrandLabel.setText("CV Interface Brand:", dontSendNotification);
    addAndMakeVisible(&interfaceBrandLabel);
    
    interfaceBrandEdit.setName("InterfaceBrandEdit");
    if (getAppProperties().getUserSettings()->containsKey("Interface-Brand"))
        interfaceBrandEdit.setText(getAppProperties().getUserSettings()->getValue("Interface-Brand"), dontSendNotification);
    else
        interfaceBrandEdit.setText(" - ", dontSendNotification);
    addAndMakeVisible(&interfaceBrandEdit);
    
    interfaceLabel.setName("InterfaceLabel");
    interfaceLabel.setJustificationType(Justification::centredRight);
    interfaceLabel.setText("CV Interface Type:", dontSendNotification);
    addAndMakeVisible(&interfaceLabel);
    
    interfaceEdit.setName("InterfaceEdit");
    if (getAppProperties().getUserSettings()->containsKey("Interface-Device"))
        interfaceEdit.setText(getAppProperties().getUserSettings()->getValue("Interface-Device"), dontSendNotification);
    else
        interfaceEdit.setText(" - ", dontSendNotification);
    addAndMakeVisible(&interfaceEdit);
    
    notesLabel.setName("NotesLabel");
    notesLabel.setJustificationType(Justification::centredRight);
    notesLabel.setText("Notes:", dontSendNotification);
    addAndMakeVisible(&notesLabel);
    
    notesEdit.setName("NotesEdit");
    notesEdit.setMultiLine(true);
    notesEdit.setReturnKeyStartsNewLine(true);
    if (getAppProperties().getUserSettings()->containsKey("Notes"))
        notesEdit.setText(getAppProperties().getUserSettings()->getValue("Notes"), dontSendNotification);
    else
        notesEdit.setText(" - ", dontSendNotification);
    addAndMakeVisible(&notesEdit);
    
    submit.setName("SubmitButton");
    submit.setButtonText("I'm done.");
    submit.addListener(this);
    addAndMakeVisible(&submit);
    
    status.setText("Measuring...", dontSendNotification);
    status.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(&status);
    
    visualizer->clearCache();
    
    state = measuring;
    tuner->addListener(this);
    tuner->setNumMeasurementRange(ReportProperties::lowestPitch,
                                  ReportProperties::pitchIncrement,
                                  ReportProperties::highestPitch);
    tuner->setResolution(ReportProperties::numPeriods);
    tuner->start();
    
}

ReportDetailsEditorScreen::~ReportDetailsEditorScreen()
{
    tuner->removeListener(this);
    
    getAppProperties().getUserSettings()->setValue("DUT-Brand", brandEdit.getText());
    getAppProperties().getUserSettings()->setValue("DUT-Device", deviceEdit.getText());
    getAppProperties().getUserSettings()->setValue("Notes", notesEdit.getText());
    getAppProperties().getUserSettings()->setValue("Interface-Brand", interfaceBrandEdit.getText());
    getAppProperties().getUserSettings()->setValue("Interface-Device", interfaceEdit.getText());
}

void ReportDetailsEditorScreen::buttonClicked (Button* bttn)
{
    if (bttn == &submit)
    {
        submitted = true;
        
        submit.setEnabled(false);
        deviceEdit.setEnabled(false);
        deviceEdit.setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
        brandEdit.setEnabled(false);
        brandEdit.setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
        notesEdit.setEnabled(false);
        notesEdit.setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
        interfaceEdit.setEnabled(false);
        interfaceEdit.setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
        interfaceBrandEdit.setEnabled(false);
        interfaceBrandEdit.setColour(TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
        
        if (tunerHasFinished)
            parent->next();
    }
}

void ReportDetailsEditorScreen::resized()
{
    int left = proportionOfWidth(0.35f);
    int width = proportionOfWidth(0.6f);
    int height = 24;
    int spacing = height/4;
    
    int y = 10;
    
    instructions.setBounds(10, y, getWidth() - 20, 40);
    y += instructions.getHeight() + 3*spacing;
    
    brandEdit.setBounds(left, y, width, height);
    brandLabel.setBounds(0, y, left, height);
    y += height + spacing;
    
    deviceEdit.setBounds(left, y, width, height);
    deviceLabel.setBounds(0, y, left, height);
    y += height + spacing;
    
    interfaceBrandEdit.setBounds(left, y, width, height);
    interfaceBrandLabel.setBounds(0, y, left, height);
    y += height + spacing;
    
    interfaceEdit.setBounds(left, y, width, height);
    interfaceLabel.setBounds(0, y, left, height);
    y += height + spacing;
    
    int editHeight = getHeight() - y - height - 10 - spacing - 40 - 2*spacing;
    notesEdit.setBounds(left, y, width, editHeight);
    notesLabel.setBounds(0, y, left, height);
    y += editHeight + spacing;
    
    submit.setBounds(proportionOfWidth(0.35f), y, proportionOfWidth(0.6f), height);
    
    status.setBounds(10, getHeight() - spacing - 40, getWidth() - 20, 40);
}

void ReportDetailsEditorScreen::paint(Graphics& g)
{
    g.setColour(Colours::black);
    g.drawHorizontalLine(instructions.getBottom() + 5, 10, getWidth() - 10);
    g.drawHorizontalLine(status.getY() - 5, 10, getWidth() - 10);
}

void ReportDetailsEditorScreen::tunerStopped()
{
    // this is only called on an error
    if (DialogWindow* dw = findParentComponentOfClass<DialogWindow>())
        dw->exitModalState (1);
}

void ReportDetailsEditorScreen::tunerFinished()
{
    switch (state)
    {
        case measuring:
            // measure the reference frequency once more
            state = reMeasuringReference;
            tuner->startSingleMeasurement(tuner->getReferencePitch());
            return;
            break;
        case reMeasuringReference:
            double initalReferenceFreq = tuner->getReferenceFrequency();
            double reMeasuredFreq = tuner->getSingleMeasurementResult();
            double pitchDrift = 12.0 * log(reMeasuredFreq / initalReferenceFreq) / log(2.0);
            
            if (std::abs(pitchDrift) > ReportProperties::desiredDriftMargin)
            {
                String driftString;
                if (std::abs(pitchDrift) >= 1)
                    driftString = String(pitchDrift, 2) + " semitones";
                else
                    driftString = String(pitchDrift * 100, 1) + " cents";
                int result = AlertWindow::showYesNoCancelBox(AlertWindow::AlertIconType::WarningIcon, "Warning: High drift!", String("Apparently the reference pitch has drifted by ") + driftString + " during the measurement. This can happen when the oscillator is not properly heated up yet, or when someone touched the tuning controls by accident. " + newLine + newLine + "Do you want to repeat the measurement?", "Repeat", "Keep the poor results", "Cancel", nullptr, nullptr);
                
                switch(result)
                {
                    // abort
                    case 0:
                        if (DialogWindow* dw = findParentComponentOfClass<DialogWindow>())
                            dw->exitModalState (1);
                        break;
                    // Repeat
                    case 1:
                        state = measuring;
                        tuner->setNumMeasurementRange(ReportProperties::lowestPitch,
                                                      ReportProperties::pitchIncrement,
                                                      ReportProperties::highestPitch);
                        tuner->setResolution(ReportProperties::numPeriods);
                        tuner->start();
                        break;
                    // keep existing
                    case 2:
                        tuner->removeListener(this);
                        
                        tunerHasFinished = true;
                        if (submitted)
                            parent->next();
                        break;
                }
            }
            // re-measured reference pitch is within margins
            else
            {
                tuner->removeListener(this);
                
                tunerHasFinished = true;
                if (submitted)
                    parent->next();
            }
            break;
    }
}

void ReportDetailsEditorScreen::tunerStatusChanged(String statusString)
{

    status.setText(statusString, dontSendNotification);
}
