/*
  ==============================================================================

    Visualizer.cpp
    Created: 22 May 2016 2:45:34pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Visualizer.h"


Visualizer::Visualizer(VCOTuner* t)
{
    tuner = t;
}

Visualizer::~Visualizer()
{
    
}


void Visualizer::paint(juce::Graphics &g, int width, int height)
{
    if (measurements.size() == 0)
    {
        g.drawText("No Data", 0, 0, width, height, juce::Justification::centred);
        return;
    }
    
    // calculate display range
    double max = 0;
    double min = 0;
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
    
    paintWithFixedScaling(g, width, height, min, max);
}

void Visualizer::paintWithFixedScaling(Graphics& g, int width, int height, double min, double max)
{
    if (measurements.size() == 0)
    {
        g.drawText("No Data", 0, 0, width, height, juce::Justification::centred);
        return;
    }
    
    // fill background
    //g.setColour(Colours::lightgrey);
    //g.fillAll();
    
    const int bottomBarHeight = 20;
    
    int imageHeight = height - bottomBarHeight;
    
    // prepare coordinate transformation (flipping the y axis)
    heightForFlipping = (float) imageHeight;
    
    const float sidebarWidth = 75;
    double columnWidth = (double) (width - sidebarWidth) / (double) measurements.size();
    const double allowedPitchOffset = 0.02; // 5 cents allowed
    //bool displayDeviation = columnWidth > 5; // larger than 5 pixels
    
    if (min > -allowedPitchOffset)
        min = -allowedPitchOffset;
    if (max < allowedPitchOffset)
        max = allowedPitchOffset;
    if (max < min)
        return;
    
    double vertScaling = (double) imageHeight / (max - min);
    
    // draw maximum "in-tune" pitch offset and center line
    g.setColour(Colours::grey);
    const float dashLengths[] = {4, 4};
    double position = (allowedPitchOffset - min) * vertScaling;
    g.drawDashedLine(Line<float>(sidebarWidth, yFlip((float) position), (float) width, yFlip((float) position)), dashLengths, 2);
    position = (-allowedPitchOffset - min) * vertScaling;
    g.drawDashedLine(Line<float>(sidebarWidth, yFlip((float) position), (float) width, yFlip((float) position)), dashLengths, 2);
    position = (- min) * vertScaling;
    g.drawLine(sidebarWidth, yFlip((float) position), (float) width, yFlip((float) position));
    
    // draw scales
    g.setColour(Colours::grey);
    const int numIntervals = 13;
    const double allowedIntervals[numIntervals] = {0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50 };
    double lineInterval = allowedIntervals[0];
    int currentIntervalIndex = 0;
    
    int numLinesAllowed = imageHeight / 30;
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
        g.drawText(lineText, 0, (int) yFlip(position + g.getCurrentFont().getHeight()/2), (int) left-4, g.getCurrentFont().getHeight(), Justification::centredRight);
        
        // don't overwrite maximum "in-tune" lines
        if (y*lineInterval == allowedPitchOffset || y*lineInterval == -allowedPitchOffset)
            continue;
        
        const float dashLengths[] = {4, 20};
        g.drawDashedLine(Line<float>((float) left, yFlip((float) position), (float) width, yFlip((float) position)), dashLengths, 2);
        
    }
    
    // draw pitch measurement points
    for (int i = 0; i < measurements.size(); i++)
    {
        float left = sidebarWidth + i*(float)columnWidth;
        
        // draw deviation
        float maxPosition = (float) ((measurements[i].pitchOffset + measurements[i].pitchDeviation - min) * vertScaling);
        float minPosition = (float) ((measurements[i].pitchOffset - measurements[i].pitchDeviation - min) * vertScaling);
        
        g.setColour(Colours::springgreen.withAlpha(0.4f));
        g.fillRect(left, yFlip(maxPosition), (float) columnWidth, maxPosition - minPosition);
        
        // draw average value
        float position = (float) ((measurements[i].pitchOffset - min) * vertScaling);
        g.setColour(Colours::green);
        g.drawLine(left, yFlip(position), left + (float) columnWidth, yFlip(position));
    }
    
    // draw the X-Axis label
    g.setColour(Colours::black);
    g.drawText("MIDI note", 0, imageHeight, sidebarWidth - 10, bottomBarHeight, Justification::centredRight);
    
    // draw the corresponding note values to the X axis
    const int numPitchTextIntervals = 5;
    const int pitchTextIntervals[numPitchTextIntervals] = {1, 2, 5, 10, 20};
    int currentPitchTextIntervalIndex = 0;
    while (g.getCurrentFont().getStringWidth("123.") > pitchTextIntervals[currentPitchTextIntervalIndex] * columnWidth)
    {
        currentPitchTextIntervalIndex++;
        if (currentPitchTextIntervalIndex >= numPitchTextIntervals)
            break;
    }
    int pitchTextInterval = pitchTextIntervals[currentPitchTextIntervalIndex];
    int startLine = 0;
    int endLine = measurements.size() - 1;
    while (measurements[startLine].midiPitch % pitchTextInterval != 0)
    {
        startLine++;
        if (startLine >= measurements.size())
            return;
    }
    while (measurements[endLine].midiPitch % pitchTextInterval != 0)
    {
        endLine--;
        if (endLine < 0 || endLine < startLine)
            return;
    }
    
    for (int i = startLine; i <= endLine; i += pitchTextInterval)
    {
        g.setColour(Colours::black);
        float textWidth = g.getCurrentFont().getStringWidth(String(measurements[i].midiPitch));
        float left = sidebarWidth + i * columnWidth;
        float x = left + columnWidth/2 - textWidth/2;
        float y = imageHeight;
        g.drawText(String(measurements[i].midiPitch), x, y, textWidth, bottomBarHeight, Justification::centred);
        
        // the line for the reference pitch will be drawn later
        if (measurements[i].midiPitch == tuner->getReferencePitch())
            continue;
        
        // also draw dim vertical lines for the larger devisions or if the columns get very narrow
        if (pitchTextInterval >= 2)
        {
            g.setColour(Colours::blue.withAlpha(0.05f));
            g.fillRect(Rectangle<float>(left, 0, columnWidth, imageHeight));
        }
        if (pitchTextInterval == 1 && columnWidth < g.getCurrentFont().getStringWidth("123."))
        {
            if (i % 2 == 0)
            {
                g.setColour(Colours::blue.withAlpha(0.05f));
                g.fillRect(Rectangle<float>(left, 0, columnWidth, imageHeight));
            }
        }
    }
    
    // draw a blue indicator for the reference pitch (if included in the measurements)
    if (measurements[0].midiPitch < tuner->getReferencePitch()
        && measurements.getLast().midiPitch > tuner->getReferencePitch())
    {
        for (int i = 0; i < measurements.size(); i++)
        {
            if (measurements[i].midiPitch == tuner->getReferencePitch())
            {
                float left = sidebarWidth + i * columnWidth;
                g.setColour(Colours::blue.withAlpha(0.15f));
                g.fillRect(Rectangle<float>(left, 0, columnWidth, imageHeight));
            }
        }
    }
}

void Visualizer::paint(Graphics& g)
{
    paint(g, getWidth(), getHeight());
}

float Visualizer::yFlip(float y)
{
    return heightForFlipping - y;
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