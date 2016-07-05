# VCOTuner
A JUCE based tuner application for VCOs, VCFs and other analog gear. It runs on Windows, Mac and Linux.

## Overview

**How tuning usually works** - Tuning is usually a tedious ping-pong game between adjusting a fine tune pot and adjusting one or multiple tuning trimmers. Whenever a trimmer has been adjusted, the fine tune pot has to be adjusted as well to bring the pitch back to a specific note. 

**How tuning works with the app** - The app spits out midi notes and measures the frequency. This is done for multiple notes in a user selectable note range. At first the note in the center of the range is selected as the reference pitch. All other measurements will be compared to this reference. This removes the need to adjust the fine tune pot. Tuning the oscillator is just a matter of tweaking the trimmers and looking at the screen. Takes no longer than a few minutes. See the video below. 

**The application can also produce a report** that features measurements in the highest accuracy and over a very wide pitch range. Reports are saved as a *.png file including information on the device under test and the CV interface that was used. 

This video shows how to use it:

<a href="http://www.youtube.com/watch?feature=player_embedded&v=JpMFTOBXuv8
" target="_blank"><img src="http://img.youtube.com/vi/JpMFTOBXuv8/0.jpg" 
alt="Youtube tutorial video" width="400" border="0" /></a>

## Download

[Head over to the "release" section of this repository to download the latest release.](https://github.com/TheSlowGrowth/VCOTuner/releases/latest)

## Help to improve it

[If you find bugs, please raise an issue here!](https://github.com/TheSlowGrowth/VCOTuner/issues)

## Are you on Muff's?

[Here's a thread on MuffWiggler. Post your tuning reports here, if you like](https://www.muffwiggler.com/forum/viewtopic.php?p=2276045)
