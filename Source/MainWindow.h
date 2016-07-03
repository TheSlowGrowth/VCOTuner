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

#ifndef __MAINWINDOW_JUCEHEADER__
#define __MAINWINDOW_JUCEHEADER__


//==============================================================================
ApplicationProperties& getAppProperties();

//==============================================================================
/**
*/
class MainWindow    : public DocumentWindow
{
public:
    //==============================================================================
    MainWindow();
    ~MainWindow();

    //==============================================================================
    void closeButtonPressed();

    bool tryToQuitApplication();

private:
    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};


#endif   // __MAINWINDOW_JUCEHEADER__
