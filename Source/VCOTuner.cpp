/*
  ==============================================================================

    VCOTuner.cpp
    Created: 17 May 2016 8:21:15pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "VCOTuner.h"
#include "AutoCorrelator.h"

VCOTuner::VCOTuner(AudioDeviceManager* d)
{
    state = stopped;
    lowestPitch = 30;
    highestPitch = 120;
    pitchIncrement = 12;
    deviceManager = d;
    midiChannel = 1;
    currentlyPlayingMidiNote = -1;
    startSampling = false;
    stopSampling = false;
    sampleBufferHead = 0;
    
    d->addChangeListener(this);
    d->addAudioCallback(this);
    
    startTimer(10);
}

VCOTuner::~VCOTuner()
{
    stopTimer();
    
    if (currentlyPlayingMidiNote >= 0)
        trySendMidiNoteOff(currentlyPlayingMidiNote);
    
    deviceManager->removeAudioCallback(this);
}


void VCOTuner::setNumMeasurementRange(int lPitch, int pitchInc, int hPitch)
{
    lowestPitch = lPitch;
    pitchIncrement = pitchInc;
    highestPitch = hPitch;
}

void VCOTuner::addListener(Listener* l)
{
    listeners.add(l);
}

void VCOTuner::removeListener(Listener* l)
{
    listeners.remove(l);
}

void VCOTuner::toggleState()
{
    if (!isRunning())
    {
        switchState(prepMeasurement);
    }
    else
    {
        switchState(stopped);
    }
}

void VCOTuner::start()
{
    if (!isRunning())
        switchState(prepMeasurement);
}

void VCOTuner::stop()
{
    if (isRunning())
        switchState(stopped);
}

void VCOTuner::startSingleMeasurement(int pitch)
{
    if (state != stopped && state != finished)
        switchState(stopped);
    
   /* singleMeasurementPitch = pitch;
    singleMeasurementResult = -1;
    
    switchState(prepareSingleMeasurement);*/
}

StringArray VCOTuner::getLastErrors()
{
    StringArray tmp = errors;
    errors.clear();
    return tmp;
}

void VCOTuner::timerCallback()
{
    switch (state)
    {
        case stopped:
            break;
        case prepMeasurement:
            // wait for low level state machine to stop sampling
            if (stopSampling)
                break;
            
            // send midi note and start measuring
            if (cycleCounter == 0)
                trySendMidiNoteOn(69);
            
            // after 100ms start sampling
            if (cycleCounter*getTimerInterval() >= 100)
            {
                startSampling = true;
                switchState(sampleInput);
            }
            cycleCounter++;
            break;
        case sampleInput:
            cycleCounter++;
            if (!startSampling)
            {
                trySendMidiNoteOff(currentlyPlayingMidiNote);
                switchState(processSkim);
            }
            break;
        case processSkim:
        {
            // find gain for normalizing
            Range<float> minMax = sampleBuffer.findMinMax(0, 0, sampleBuffer.getNumSamples());
            float gain = 1 / jmax(-minMax.getStart(), minMax.getEnd());
            sampleBuffer.applyGain(gain);
            
            double absMaximum = AutoCorrelator::calculate(sampleBuffer, 0);
            
            typedef struct {
                double tau_start;
                double tau_end;
                double tau_maximum;
                double maximum;
            } Region;
            Array<Region> regions;
            Region currentRegion;
            currentRegion.maximum = 0;
            bool rangeEntered = false;
            bool firstMaximumPassed = false;
            
            // skim over the data and find regions with an auto correlation > 0.8*absolute maximum value
            double threshold = 0.8 * absMaximum;
            for (int i = 0; i < sampleBuffer.getNumSamples()/2; i++)
            {
                double result = AutoCorrelator::calculate(sampleBuffer, i);
                if (result > threshold && !rangeEntered && firstMaximumPassed)
                {
                    currentRegion.tau_start = i;
                    rangeEntered = true;
                }
                else if (rangeEntered)
                {
                    if (result < threshold)
                    {
                        rangeEntered = false;
                        currentRegion.tau_end = i;
                        regions.add(currentRegion);
                        currentRegion.maximum = 0;
                    }
                    else if (result > currentRegion.maximum)
                    {
                        currentRegion.tau_maximum = i;
                        currentRegion.maximum = result;
                    }
                }
                // to avoid indexing the maximum at tau == 0!
                else if (!firstMaximumPassed && result <= threshold)
                    firstMaximumPassed = true;
                
                if (regions.size() >= 2)
                    break;
            }
            /*
            Image imgS(Image::PixelFormat::RGB, sampleBuffer.getNumSamples()/20, 300, true);
            Graphics gS(imgS);
            gS.fillAll(Colours::white);
            gS.setColour(Colours::black);
            Range<float> waveformMax = sampleBuffer.findMinMax(0, 0, sampleBuffer.getNumSamples());
            for (int i = 0; i < sampleBuffer.getNumSamples()/10; i++)
            {
                double value = sampleBuffer.getSample(0, i);
                double y = (1 - (value - waveformMax.getStart())/(waveformMax.getLength()))*imgS.getHeight();
                gS.setPixel(i, y);
            }
            File("~/waveform.png").deleteFile();
            FileOutputStream streamS(File("~/waveform.png"));
            PNGImageFormat formatS;
            if (!formatS.writeImageToStream(imgS, streamS))
            {
                NativeMessageBox::showMessageBox(AlertWindow::WarningIcon, "Error!", "Error writing the image file!");
            }
            
            Image img(Image::PixelFormat::RGB, sampleBuffer.getNumSamples()/20, 300, true);
            Graphics g(img);
            g.fillAll(Colours::white);
            g.setColour(Colours::black);
            for (int i = 0; i < sampleBuffer.getNumSamples()/2; i++)
            {
                double value = AutoCorrelator::calculate(sampleBuffer, i);
                double y = (1 - value/absMaximum)*img.getHeight();
                g.setPixel(i, y);
            }
            File("~/correlation.png").deleteFile();
            FileOutputStream stream(File("~/correlation.png"));
            PNGImageFormat format;
            if (!format.writeImageToStream(img, stream))
            {
                NativeMessageBox::showMessageBox(AlertWindow::WarningIcon, "Error!", "Error writing the image file!");
            }*/
            
            
            // ==== iteratively scan the data to find its true maximum ====
            // analyse two data points on both sides of the current maximum
            // take the higher one as the next maximum
            // decrease step size whenever direction changes or when both sides are lower
            // than the current maximum.
            // stop when stepsize has reached a lower bound
            // do this for each region that was previously found.
            for (int i = 0; i < regions.size(); i++)
            {
                double h = 0.8;
                double result_tau = AutoCorrelator::calculate(sampleBuffer, regions[i].tau_maximum);
                bool isIncreasing = true;
                
                while (h > 0.0000001)
                {
                    double result_tau_plus =  AutoCorrelator::calculate(sampleBuffer, regions[i].tau_maximum + h);
                    double result_tau_minus = AutoCorrelator::calculate(sampleBuffer, regions[i].tau_maximum - h);
                    
                    if (result_tau_plus > result_tau_minus && result_tau_plus > result_tau)
                    {
                        regions.getReference(i).tau_maximum = regions[i].tau_maximum + h;
                        result_tau = result_tau_plus;
                        if (!isIncreasing)
                        {
                            isIncreasing = true;
                            h = h * 0.8;
                        }
                        continue;
                    }
                    else if (result_tau_minus > result_tau_plus && result_tau_minus > result_tau)
                    {
                        regions.getReference(i).tau_maximum = regions[i].tau_maximum - h;
                        result_tau = result_tau_minus;
                        if (isIncreasing)
                        {
                            isIncreasing = false;
                            h = h * 0.8;
                        }
                        continue;
                    }
                    else
                    {
                        h = h * 0.8;
                    }
                }
                double frequency = sampleRate/regions[i].tau_maximum;
                int p = 1;
                NativeMessageBox::showMessageBoxAsync(AlertWindow::AlertIconType::InfoIcon, "Result", "Frequency = " + String(frequency) + "Hz", nullptr);
            }
            cycleCounter++;
            switchState(stopped);
        } break;
        case processFine:
        {
            
            cycleCounter++;
        } break;
        case finished:
            break;
        /*case prepareContinuousFrequencyMeasurement:
        {
            // wait for low level state machine to stop sampling
            if (stopMeasurement)
                break;
                
            // send midi note and start measuring
            trySendMidiNoteOn(continuousFrequencyMeasurementPitch);
            startMeasurement = true;
            switchState(continuousFrequencyMeasurement);
            cycleCounter++;
        } break;
        case continuousFrequencyMeasurement:
        {
            // if the measurement is done)
            if (!startMeasurement)
            {
                // calculate frequency
                int numMeasurements = periodLengthsHead - indexOfFirstValidPeriodLength;
                int accumulator = 0;
                for (int i = indexOfFirstValidPeriodLength; i < periodLengthsHead; i++)
                    accumulator += periodLengths[i];
                
                double averagePeriod = (double) accumulator / (double) numMeasurements;
                
                double frequency = sampleRate / averagePeriod;
                
                // estimate deviation of frequency
                double fAccumulator = 0;
                for (int i = indexOfFirstValidPeriodLength; i < periodLengthsHead; i++)
                {
                    double f = sampleRate / (double) periodLengths[i];
                    fAccumulator += pow(f - frequency, 2);
                }
                fAccumulator = fAccumulator / (numMeasurements - 1);
                double fDeviation = sqrt(fAccumulator);
                
                continuousFreqMeasurementResult = frequency;
                continuousFreqMeasurementDeviation = fDeviation;
                
                // restart measurement
                startMeasurement = true;
            }
            cycleCounter++;
        } break;
        case prepareSingleMeasurement:
        {
            // wait for low level state machine to stop measuring
            if (stopMeasurement)
                break;
            
            if (cycleCounter == 0)
            {
                // send midi note
                trySendMidiNoteOn(singleMeasurementPitch);
            }
            else
            {
                // after 100ms, we expect the oscillator to be settled at the new pitch
                // (gives some safety margin for audio/midi interface latency)
                if (cycleCounter >= 10)
                {
                    // start a measurement and see if we get a stable pitch here
                    startMeasurement = true;
                    switchState(singleMeasurement);
                    break;
                }
            }
            cycleCounter++;
        } break;
        case singleMeasurement:
        {
            // measurement done
            if (!startMeasurement)
            {
                // send note off
                trySendMidiNoteOff(singleMeasurementPitch);
                
                if (lError == notStable)
                {
                    errors.add(Errors::highJitter);
                    switchState(stopped);
                }
                else
                {
                    // calculate frequency
                    int numMeasurements = periodLengthsHead - indexOfFirstValidPeriodLength;
                    int accumulator = 0;
                    for (int i = indexOfFirstValidPeriodLength; i < periodLengthsHead; i++)
                        accumulator += periodLengths[i];
                    
                    double averagePeriod = (double) accumulator / (double) numMeasurements;
                    
                    double frequency = sampleRate / averagePeriod;
                    
                    // estimate deviation of frequency
                    double fAccumulator = 0;
                    for (int i = indexOfFirstValidPeriodLength; i < periodLengthsHead; i++)
                    {
                        double f = sampleRate / (double) periodLengths[i];
                        fAccumulator += pow(f - frequency, 2);
                    }
                    fAccumulator = fAccumulator / (numMeasurements - 1);
                    double fDeviation = sqrt(fAccumulator);
                    
                    singleMeasurementResult = frequency;
                    singleMeasurementDeviation = fDeviation;
                    
                    switchState(finished);
                    break;
                }
            }
            
            // timeout handling
            if (cycleCounter > 1000)
            {
                if (periodLengthsHead == 0)
                    errors.add(Errors::noZeroCrossings);
                else if (lError == notStable)
                    errors.add(Errors::highJitterTimeOut);
                else
                    errors.add(Errors::stableTimeout);
                stopMeasurement = true;
                switchState(stopped);
                break;
            }
            cycleCounter++;
        } break;*/
        default:
            switchState(stopped);
            break;
    }
}

void VCOTuner::startContinuousMeasurement(int pitch)
{
    /*continuousFrequencyMeasurementPitch = pitch;
    if (state != stopped && state != finished)
        switchState(stopped);
    state = prepareContinuousFrequencyMeasurement;*/
}

void VCOTuner::trySendMidiNoteOn(int pitch)
{
    MidiOutput* midiOut = deviceManager->getDefaultMidiOutput();
    if (midiOut == nullptr)
    {
        errors.add(Errors::noMidiDeviceAvailable);
        switchState(stopped);
        return;
    }
    
    if (currentlyPlayingMidiNote != -1)
        trySendMidiNoteOff(currentlyPlayingMidiNote);
    
    midiOut->sendMessageNow(MidiMessage::noteOn(midiChannel, pitch, (uint8_t) 100));
    currentlyPlayingMidiNote = pitch;
}

void VCOTuner::trySendMidiNoteOff(int pitch)
{
    MidiOutput* midiOut = deviceManager->getDefaultMidiOutput();
    if (midiOut == nullptr)
    {
        errors.add(Errors::noMidiDeviceAvailable);
        switchState(stopped);
        return;
    }
    
    midiOut->sendMessageNow(MidiMessage::noteOff(midiChannel, pitch));
    currentlyPlayingMidiNote = -1;
}

/** inherited from AudioIODeviceCallback */
void VCOTuner::audioDeviceIOCallback (const float** inputChannelData,
                                    int numInputChannels,
                                    float** outputChannelData,
                                    int numOutputChannels,
                                    int numSamples)
{
    if (inputChannelData == nullptr)
        return;
    const AudioBuffer<const float> inputBuffer(inputChannelData, numInputChannels, numSamples);

    if (stopSampling)
    {
        startSampling = false;
        stopSampling = false;
        sampleBufferHead = 0;
    }
    
    if (startSampling)
    {
        int numToWrite = numSamples;
        bool stop = false;
        
        if (sampleBufferHead + numSamples > sampleBuffer.getNumSamples())
        {
            numToWrite = sampleBuffer.getNumSamples() - sampleBufferHead;
            stop = true;
        }
        
        sampleBuffer.copyFrom(0, sampleBufferHead, inputChannelData[0], numToWrite);
        sampleBufferHead += numToWrite;

        if (stop)
        {
            startSampling = false;
            sampleBufferHead = 0;
        }
    }

    if (outputChannelData != nullptr)
    { 
    	AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
    	outputBuffer.clear();
    }
}

void VCOTuner::switchState(VCOTuner::State newState)
{
    cycleCounter = 0;
    state = newState;
    if (state == stopped)
    {
        if (currentlyPlayingMidiNote >= 0 && currentlyPlayingMidiNote < 128)
            trySendMidiNoteOff(currentlyPlayingMidiNote);
        stopSampling = true;
        listeners.call(&Listener::tunerStopped);
    }
    else if (newState == prepMeasurement)
        listeners.call(&Listener::tunerStarted);
    else if (newState == finished)
        listeners.call(&Listener::tunerFinished);
    
    listeners.call(&Listener::tunerStatusChanged, getStatusString());
}

/** inherited from AudioIODeviceCallback */
void VCOTuner::audioDeviceAboutToStart (AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    sampleBuffer.setSize(1, sampleRate*2.0); //2s sample data
    correlationSkimResults.ensureStorageAllocated(sampleBuffer.getNumSamples());
}

/** inherited from AudioIODeviceCallback */
void VCOTuner::audioDeviceStopped()
{
	if (isRunning())
        errors.add(Errors::audioDeviceStoppedDuringMeasurement);

    switchState(stopped);
}

void VCOTuner::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == deviceManager)
    {
        switchState(stopped);
    }
}

String VCOTuner::getStatusString() const 
{
  /*  switch(state)
    {
        case stopped:
            return "Stopped.";
            break; // these breaks are only here to prevent IDE warnings...
        case prepRefMeasurement:
        case refMeasurement:
            return "Measuring reference frequency ...";
            break;
        case prepMeasurement:
        case measurement:
            return "Measuring frequency for MIDI note " + String(currentPitch) + " ...";
            break;
        case finished:
            return "Finished.";
            break;
        case prepareContinuousFrequencyMeasurement:
        case continuousFrequencyMeasurement:
            return "Continuously measuring frequency...";
            break;
        case prepareSingleMeasurement:
        case singleMeasurement:
            return "Measuring frequency for MIDI note " + String(singleMeasurementPitch) + " ...";
        default:
            return "";
            break;
    }*/
    return "nothing here.";
}

const String VCOTuner::Errors::highJitter = "There are zero crossings in the incoming signal but they don't seem to be coming in at a constant rate. Are you sure you're recording on the correct channel? Please use only primitive waveforms (saw, square, triangle, sine, ...) without any other processing such as delays, reverbs, etc. This error typically appears when you are accidentally recording the signal from a microphone or another sound source. Or when you have dropouts (aka clicks and pops) in your audio.";

const String VCOTuner::Errors::noZeroCrossings = "The incoming audio signal does not seem to contain any zero-crossings. Are you sure the oscillator signal is getting through to us? Check your audio device settings.";

const String VCOTuner::Errors::highJitterTimeOut = "Timeout. " + highJitter;

const String VCOTuner::Errors::stableTimeout = "There are some zero crossings in the incoming signal and they seem to come in at a constant rate - but they are coming in much slower than they should be. Are you recording from the right oscillator?";

const String VCOTuner::Errors::noFrequencyChangeBetweenMeasurements = "Apparently the frequency of the oscillator is not changing between measurements. Please check if your MIDI-to-CV interface is set to the correct MIDI channel and make sure that it is selected as the default midi output device in the audio and midi settings.";

const String VCOTuner::Errors::noMidiDeviceAvailable = "You don't have a MIDI output device selected or the selected device is not available.";

const String VCOTuner::Errors::audioDeviceStoppedDuringMeasurement = "The audio device was stopped while the measurement was still running. Please check that the device is still powered, all cables are connected and the driver is working correctly.";
