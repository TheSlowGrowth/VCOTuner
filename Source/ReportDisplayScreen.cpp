/*
  ==============================================================================

    ReportDisplayScreen.cpp
    Created: 2 Jul 2016 4:12:22pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "ReportDisplayScreen.h"


ReportDisplayScreen::ReportDisplayScreen(VCOTuner* t, Visualizer* v, ReportCreatorWindow* p)
: img(Image::RGB, 800, 600, true)
{
    tuner = t;
    visualizer = v;
    parent = p;

    save.setButtonText("Save Report");
    save.addListener(this);
    addAndMakeVisible(&save);
    
    close.setButtonText("Close");
    close.addListener(this);
    addAndMakeVisible(&close);
    
    drawReport();
}

ReportDisplayScreen::~ReportDisplayScreen()
{

}

void ReportDisplayScreen::resized()
{
    save.setBounds(10, 10, 60, 20);
    close.setBounds(getWidth() - 10 - 60, 10, 60, 20);
}

void ReportDisplayScreen::buttonClicked (Button* bttn)
{
    if (bttn == &save)
    {
        // store as an image file
        FileChooser fileChooser("Save report ... ", File::nonexistent, "*.png");
        if (fileChooser.browseForFileToSave(true))
        {
            FileOutputStream stream(fileChooser.getResult());
            PNGImageFormat format;
            if (!format.writeImageToStream(img, stream))
            {
                NativeMessageBox::showMessageBox(AlertWindow::WarningIcon, "Error!", "Error writing the report image file!");
            }
        }
    }
    else if (bttn == &close)
    {
        if (DialogWindow* dw = findParentComponentOfClass<DialogWindow>())
            dw->exitModalState (1);
    }
}

void ReportDisplayScreen::paint(juce::Graphics &g)
{
    g.drawImageAt(img, 0, 40);
}

void ReportDisplayScreen::drawReport()
{
    Graphics g(img);
    
    g.fillAll(Colours::white);
    
    g.setColour(Colours::black);
    
    const int contentHeight = 16;
    const int lineHeight = contentHeight + contentHeight/4;
    Rectangle<int> leftColumnLabels(10, 10, 160, contentHeight);
    Rectangle<int> leftColumnContent(160, 10, 200, contentHeight);
    Rectangle<int> rightColumnLabels(380, 10, 120, contentHeight);
    Rectangle<int> rightColumnContent(500, 10, 800 - 10 - 500, contentHeight);
    
    
    String dutBrand = getAppProperties().getUserSettings()->getValue("DUT-Brand");
    String dutType = getAppProperties().getUserSettings()->getValue("DUT-Device");
    String interfaceBrand = getAppProperties().getUserSettings()->getValue("Interface-Brand");
    String interfaceType = getAppProperties().getUserSettings()->getValue("Interface-Device");
    String notes = getAppProperties().getUserSettings()->getValue("Notes");
    
    g.drawText("Device under test:", leftColumnLabels, Justification::topLeft);
    g.drawText("'" + dutType + "' (" + dutBrand + ")", leftColumnContent, Justification::topLeft);
    leftColumnLabels.translate(0, lineHeight);
    leftColumnContent.translate(0, lineHeight);
    
    g.drawText("CV Interface:", leftColumnLabels, Justification::topLeft);
    g.drawText("'" + interfaceType + "' (" + interfaceBrand + ")", leftColumnContent, Justification::topLeft);
    leftColumnLabels.translate(0, lineHeight);
    leftColumnContent.translate(0, lineHeight);
    
    g.drawText("Samplerate:", leftColumnLabels, Justification::topLeft);
    g.drawText(String(tuner->getCurrentSampleRate() / 1000) + " kHz", leftColumnContent, Justification::topLeft);
    leftColumnLabels.translate(0, lineHeight);
    leftColumnContent.translate(0, lineHeight);
    
    g.drawText("Reference frequency:", leftColumnLabels, Justification::topLeft);
    g.drawText(String(tuner->getReferenceFrequency()) + " Hz", leftColumnContent, Justification::topLeft);
    leftColumnLabels.translate(0, lineHeight);
    leftColumnContent.translate(0, lineHeight);
    
    g.drawText("Notes:", rightColumnLabels, Justification::topLeft);
    g.drawText(notes, rightColumnContent.withHeight(lineHeight + contentHeight), Justification::topLeft);
    
    g.saveState();
    int bottom = jmax(leftColumnLabels.getBottom(), leftColumnContent.getBottom(),
                      rightColumnLabels.getBottom(), rightColumnContent.getBottom());
    Rectangle<int> graphArea(10, bottom + 10, img.getWidth() - 20, img.getHeight() - 10 - bottom - 10);
    g.reduceClipRegion(graphArea);
    g.setOrigin(graphArea.getTopLeft());
    visualizer->paintWithFixedScaling(g, graphArea.getWidth(), graphArea.getHeight(), -0.15, 0.15);
    g.restoreState();
}