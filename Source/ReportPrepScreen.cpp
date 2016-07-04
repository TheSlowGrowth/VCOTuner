/*
  ==============================================================================

    ReportPrepScreen.cpp
    Created: 2 Jul 2016 4:12:50pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ReportPrepScreen.h"
#include "ReportProperties.h"

ReportPrepScreen::ReportPrepScreen(VCOTuner* t, Visualizer* v, ReportCreatorWindow* p)
{
    tuner = t;
    visualizer = v;
    parent = p;
    
    t->startContinuousMeasurement(ReportProperties::adjustmentPitch);
    startTimer(100);
    millisecCounter = 0;
}

ReportPrepScreen::~ReportPrepScreen()
{
	stopTimer();
	Timer::callPendingTimersSynchronously();

    if (tuner->isRunning())
        tuner->toggleState();
}

void ReportPrepScreen::timerCallback()
{
    currentFreq = tuner->getContinuousMesurementResult();
    
    if (currentFreq < ReportProperties::desiredAdjustmentFrequency + ReportProperties::allowedDeviation
        && currentFreq > ReportProperties::desiredAdjustmentFrequency - ReportProperties::allowedDeviation)
    {
        millisecCounter += getTimerInterval();
        if (millisecCounter >= ReportProperties::requiredHoldTimeInMs)
            parent->next();
    }
    else
    {
        millisecCounter = 0;
    }
    
    repaint();
}

void ReportPrepScreen::paint(Graphics& g)
{
    Rectangle<int> box(0, 0, 300, 50);
    box.setCentre(getBounds().getCentre());
    
    g.setColour(Colours::white);
    g.fillRect(box);
    
    
    double lineSpacing = box.getWidth() / 12;
    g.setColour(Colours::lightgreen.withAlpha(0.5f));
    g.fillRect(Rectangle<int>(box.getCentreX() - (int)lineSpacing, box.getY(), (int)(2.0*lineSpacing), box.getHeight()));

    for (int i = -6; i < 7; i++)
    {
        int spacing;
        if (i == -1 || i == 1)
        {
            g.setColour(Colours::black.withAlpha(0.5f));
            spacing = 0;
        }
        else
        {
            g.setColour(Colours::black.withAlpha(0.25f));
            spacing = 6;
        }
        g.drawLine(box.getCentreX() + (int)(i*lineSpacing), box.getY() + spacing, box.getCentreX() + (int)(i*lineSpacing), box.getBottom() - spacing);
    }
    
    double frequencyOffset = currentFreq - ReportProperties::desiredAdjustmentFrequency;
    double halfBoxRange = ReportProperties::allowedDeviation * 6;
    float positionInBox = (float) (frequencyOffset / halfBoxRange);
    if (positionInBox > 1.0)
    {
        g.setColour(Colours::red);
        g.drawText(">>", box.getRight() + 5, box.getY(), 40, box.getHeight(), juce::Justification::centredLeft);
    }
    else if (positionInBox < -1.0)
    {
        g.setColour(Colours::red);
        g.drawText("<<", box.getX() - 45, box.getY(), 40, box.getHeight(), juce::Justification::centredRight);
    }
    else
    {
        g.setColour(Colours::red);
        g.drawLine(box.getCentreX() + (int) (positionInBox * box.getWidth()/2), box.getY(), box.getCentreX() + (int) (positionInBox * box.getWidth()/2), box.getBottom());
    }
    
    g.setColour(Colours::black);
    g.drawText("Use coarse and fine tune controls to adjust the", box.translated(0, -120), juce::Justification::centred);
    g.drawText("frequency to " + String(ReportProperties::desiredAdjustmentFrequency) + " Hz. This is done to make reports", box.translated(0, -105), juce::Justification::centred);
    g.drawText("more comparable by using the same pitch ranges.", box.translated(0, -90), juce::Justification::centred);
    g.drawText(String(currentFreq) + " Hz", box.translated(0, -box.getHeight()), juce::Justification::centred);
    
    g.drawText("Measurements will start when the frequency error is below", 0, getHeight() - 80, getWidth(), 15, juce::Justification::centred);
    g.drawText("+-" + String(ReportProperties::allowedDeviation) + " Hz for at least " + String((float) ReportProperties::requiredHoldTimeInMs / 1000.0f) + " seconds", 0, getHeight() - 65, getWidth(), 15, juce::Justification::centred);
}