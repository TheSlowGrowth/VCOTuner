/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainWindow.h"
#include "MainComponent.h"
#include "VersionInfo.h"

//==============================================================================
MainWindow::MainWindow()
    : DocumentWindow (JUCEApplication::getInstance()->getApplicationName() + " " + versionString, Colours::lightgrey,
                      DocumentWindow::allButtons)
{
    setResizable (true, false);
    setResizeLimits (600, 300, 10000, 10000);
    centreWithSize (600, 300);
    
    setContentOwned (new MainComponent (), false);
    
    if (getAppProperties().getUserSettings()->containsKey("mainWindowPos"))
        restoreWindowStateFromString (getAppProperties().getUserSettings()->getValue ("mainWindowPos"));
    setVisible(true);
}

MainWindow::~MainWindow()
{
    getAppProperties().getUserSettings()->setValue ("mainWindowPos", getWindowStateAsString());
    clearContentComponent();
}

void MainWindow::closeButtonPressed()
{
    tryToQuitApplication();
}

bool MainWindow::tryToQuitApplication()
{
    JUCEApplication::quit();

    return true;
}
