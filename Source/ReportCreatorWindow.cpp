/*
  ==============================================================================

    ReportCreaterWindow.cpp
    Created: 2 Jul 2016 4:12:09pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "ReportCreatorWindow.h"
#include "ReportDetailsEditorScreen.h"
#include "ReportDisplayScreen.h"
#include "ReportPrepScreen.h"

ReportCreatorWindow::ReportCreatorWindow(VCOTuner* t, Visualizer* v)
{
    state = prepareReport;
    tuner = t;
    visualizer = v;
    currentContentComponent = new ReportPrepScreen(tuner, visualizer, this);
    setSize(800, 640);
    addAndMakeVisible(currentContentComponent);
}

ReportCreatorWindow::~ReportCreatorWindow()
{
    delete currentContentComponent;
}

void ReportCreatorWindow::next()
{
    switch (state)
    {
        case prepareReport:
            delete currentContentComponent;
            currentContentComponent = new ReportDetailsEditorScreen(tuner, visualizer, this);
            addAndMakeVisible(currentContentComponent);
            resized();
            state = editTextsAndDoMeasurements;
            break;
        case editTextsAndDoMeasurements:
            delete currentContentComponent;
            currentContentComponent = new ReportDisplayScreen(tuner, visualizer, this);
            addAndMakeVisible(currentContentComponent);
            resized();
            state = displayResults;
            break;
        case displayResults:
            break;
        default:
            break;
    }
}

void ReportCreatorWindow::resized()
{
    currentContentComponent->setBounds(0, 0, getWidth(), getHeight());
}