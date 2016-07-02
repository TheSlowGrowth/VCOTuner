/*
  ==============================================================================

    Visualizer.cpp
    Created: 22 May 2016 2:45:34pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Visualizer.h"


Visualizer::Visualizer()
{
    
}

Visualizer::~Visualizer()
{
    
}

void Visualizer::paint(Graphics& g, int width, int height)
{
    if (measurements.size() == 0)
    {
        g.drawText("No Data", 0, 0, width, height, juce::Justification::centred);
        return;
    }
    
    const float sidebarWidth = 75;
    double columnWidth = (double) (width - sidebarWidth) / (double) measurements.size();
    const double allowedPitchOffset = 0.02; // 5 cents allowed
    //bool displayDeviation = columnWidth > 5; // larger than 5 pixels
    
    // calculate display range
    double max = allowedPitchOffset;
    double min = -allowedPitchOffset;
    for (int i = 0; i < measurements.size(); i++)
    {
        double value = measurements[i].pitchOffset;
        double deviation = measurements[i].pitchDeviation;
        if (value - deviation < min)
            min = value - deviation;
        if (value + deviation > max)
            max = value + deviation;
    }
    double expandAmount = (max - min) * 0.2;
    min -= expandAmount;
    max += expandAmount;
    
    double vertScaling = (double) height / (max - min);
    
    // draw maximum "in-tune" pitch offset and center line
    g.setColour(Colours::grey);
    const float dashLengths[] = {4, 4};
    double position = (allowedPitchOffset - min) * vertScaling;
    g.drawDashedLine(Line<float>(sidebarWidth, (float) position, (float) width, (float) position), dashLengths, 2);
    position = (-allowedPitchOffset - min) * vertScaling;
    g.drawDashedLine(Line<float>(sidebarWidth, (float) position, (float) width, (float) position), dashLengths, 2);
    position = (- min) * vertScaling;
    g.drawLine(sidebarWidth, (float) position, (float) width, (float) position);
    
    // draw scales
    g.setColour(Colours::grey);
    const int numIntervals = 13;
    const double allowedIntervals[numIntervals] = {0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50 };
    double lineInterval = allowedIntervals[0];
    int currentIntervalIndex = 0;
    
    int numLinesAllowed = height / 30;
    while (((max - min) / lineInterval) > numLinesAllowed)
    {
        lineInterval = allowedIntervals[++currentIntervalIndex];
        if (currentIntervalIndex >= numIntervals)
            break;
    }
    bool useSemitoneTexts = lineInterval >= 1.0;
    
    int numPosLines = (int) trunc(max/lineInterval);
    int numNegLines = (int) trunc(-min/lineInterval);
    for (double y = numPosLines; y > -numNegLines; y--)
    {
        double position = (y*lineInterval - min) * vertScaling;
        double left = sidebarWidth - 2;
        double number = y*lineInterval*((useSemitoneTexts)?1.0:100.0);
        String numberString = (std::abs(number - round(number)) > 0.1)?String(number, 1):String((int)round(number));
        String lineText = numberString;
        if (!useSemitoneTexts)
            lineText += " cents";
        g.drawText(lineText, 0, (int) position - 8, (int) left-4, 16, Justification::centredRight);
        
        // don't overwrite maximum "in-tune" lines
        if (y*lineInterval == allowedPitchOffset || y*lineInterval == -allowedPitchOffset)
            continue;
        
        const float dashLengths[] = {4, 20};
        g.drawDashedLine(Line<float>((float) left, (float) position, (float) width, (float) position), dashLengths, 2);
        
    }
    
    // draw pitch measurement points
    for (int i = 0; i < measurements.size(); i++)
    {
        float left = sidebarWidth + i*(float)columnWidth;
        
        // draw deviation
        float maxPosition = (float) ((measurements[i].pitchOffset + measurements[i].pitchDeviation - min) * vertScaling);
        float minPosition = (float) ((measurements[i].pitchOffset - measurements[i].pitchDeviation - min) * vertScaling);
        
        g.setColour(Colours::springgreen.withAlpha(0.4f));
        g.fillRect(left, minPosition, (float) columnWidth, maxPosition - minPosition);
        
        // draw average value
        float position = (float) ((measurements[i].pitchOffset - min) * vertScaling);

        
        g.setColour(Colours::green);
        g.drawLine(left, position, left + (float) columnWidth, position);
    }
}

void Visualizer::paint(Graphics& g)
{
    paint(g, getWidth(), getHeight());
}

void Visualizer::newMeasurementReady(const VCOTuner::measurement_t& m)
{
    bool found = false;
    for (int i = 0; i < measurements.size(); i++)
    {
        if (measurements[i].midiPitch == m.midiPitch)
        {
            measurements.set(i, m);
            found = true;
            repaint();
        }
    }
    
    if (!found)
        measurements.add(m);
    
    repaint();
}