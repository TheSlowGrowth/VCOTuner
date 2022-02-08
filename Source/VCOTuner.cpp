/*
  ==============================================================================

    VCOTuner.cpp
    Created: 17 May 2016 8:21:15pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "VCOTuner.h"

VCOTuner::VCOTuner(AudioDeviceManager* d)
{
    state = stopped;
    numPeriodSamples = 10;
    lowestPitch = 30;
    highestPitch = 120;
    pitchIncrement = 12;
    deviceManager = d;
    midiChannel = 1;
    currentlyPlayingMidiNote = -1;
    
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
        switchState(prepRefMeasurement);
    }
    else
    {
        switchState(stopped);
    }
}

void VCOTuner::start()
{
    if (!isRunning())
        switchState(prepRefMeasurement);
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
    
    singleMeasurementPitch = pitch;
    singleMeasurementResult = -1;
    
    switchState(prepareSingleMeasurement);
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
        case prepRefMeasurement:
            if (cycleCounter == 0)
            {
                // send reference midi note
                referencePitch = (highestPitch + lowestPitch) / 2;
                currentPitch = referencePitch;
                trySendMidiNoteOn(currentPitch);
            }
            else
            {
                // after 100ms, we expect the oscillator to be settled at the new pitch
                // (gives some safety margin for audio/midi interface latency)
                if (cycleCounter >= 10)
                {
                    // start a measurement and see if we get a stable pitch here
                    startMeasurement = true;
                    switchState(refMeasurement);
                    break;
                }
            }
            cycleCounter++;
            break;
        case refMeasurement:
        {
            // measurement done
            if (!startMeasurement)
            {
                // send note off
                trySendMidiNoteOff(currentPitch);
                
                if (lError == notStable)
                {
                    errors.add(Errors::highJitter);
                    switchState(stopped);
                }
                else
                {
                    // calculate frequency
                    int numMeasurements = periodLengthsHead - indexOfFirstValidPeriodLength;
                    double accumulator = 0;
                    for (int i = indexOfFirstValidPeriodLength; i < periodLengthsHead; i++)
                        accumulator += periodLengths[i];
                    
                    double averagePeriod = accumulator / (double) numMeasurements;
                    
                    referenceFrequency = float(sampleRate / averagePeriod);
                    
                    // prepare next measurement
                    currentPitch = lowestPitch;
                    currentIndex = 0;
                    switchState(prepMeasurement);
                    break;
                }
            }

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
            break;
        }
        case prepMeasurement:
            if (cycleCounter == 0)
            {
                // send midi note
                trySendMidiNoteOn(currentPitch);
            }
            else
            {
                // after 100ms, we expect the oscillator to be settled at the new pitch
                // (gives some safety margin for audio/midi interface latency)
                if (cycleCounter >= 10)
                {
                    // start a measurement and see if we get a stable pitch here
                    startMeasurement = true;
                    switchState(measurement);
                    break;
                }
            }
            cycleCounter++;
            break;
        case measurement:
        {
            // measurement done
            if (!startMeasurement)
            {
                // send note off
                trySendMidiNoteOff(currentPitch);

                if (lError == notStable)
                {
                    errors.add(Errors::highJitter);
                    switchState(stopped);
                }
                else
                {
                    // calculate frequency
                    int numMeasurements = periodLengthsHead - indexOfFirstValidPeriodLength;
                    double accumulator = 0;
                    for (int i = indexOfFirstValidPeriodLength; i < periodLengthsHead; i++)
                        accumulator += periodLengths[i];
                    
                    double averagePeriod = (double) accumulator / (double) numMeasurements;
                    
                    double frequency = sampleRate / averagePeriod;
                    double pitch = 12.0 * log(frequency / referenceFrequency) / log(2.0) + referencePitch;
                    
                    // check if the frequency has changed compared to the reference frequency
                    // if not, it is likely that the MIDI output is not working. Do this only for the very first measurement
                    if (currentIndex == 0)
                    {
                        if (std::abs(frequency - referenceFrequency)/referenceFrequency < 0.1)
                        {
                            errors.add(Errors::noFrequencyChangeBetweenMeasurements);
                            switchState(stopped);
                        }
                    }
                    
                    // estimate deviation of frequency and pitch
                    double fAccumulator = 0;
                    double pAccumulator = 0;
                    for (int i = indexOfFirstValidPeriodLength; i < periodLengthsHead; i++)
                    {
                        double f = sampleRate / (double) periodLengths[i];
                        fAccumulator += pow(f - frequency, 2);
                        pAccumulator += pow(12.0 * log(f / referenceFrequency) / log(2.0) + referencePitch - pitch, 2);
                    }
                    fAccumulator = fAccumulator / (numMeasurements - 1);
                    pAccumulator = pAccumulator / (numMeasurements - 1);
                    double fDeviation = sqrt(fAccumulator);
                    double pDeviation = sqrt(pAccumulator);
                    
                    measurement_t m;
                    m.timestamp = Time::getCurrentTime();
                    m.frequency = frequency;
                    m.pitch = pitch;
                    m.midiPitch = currentPitch;
                    m.pitchOffset = pitch - currentPitch;
                    m.freqDeviation = fDeviation;
                    m.pitchDeviation = pDeviation;
                    m.numMeasurements = numMeasurements;
                    listeners.call(&Listener::newMeasurementReady, m);
                    
                    // prepare next measurement
                    currentPitch += pitchIncrement;
                    currentIndex++;
                    
                    if (currentPitch <= highestPitch)
                        switchState(prepMeasurement);
                    else
                        switchState(finished);
                    break;
                }
            }
            
            float expectedFrequency = referenceFrequency * powf(2,((float) currentPitch - (float) referencePitch)/12.0f);
            float expectedTime = 1.0f / (float) expectedFrequency * numPeriodSamples;
            expectedTime *= 2;
            int expectedCycles = juce::roundToInt(expectedTime * 100);
            if (cycleCounter > expectedCycles)
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
            break;
        }
        case finished:
            break;
        case prepareContinuousFrequencyMeasurement:
        {
            // wait for low level state machine to stop measuring
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
                double accumulator = 0;
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
                    double accumulator = 0;
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
        } break;
        default:
            state = stopped;
            break;
    }
}

void VCOTuner::startContinuousMeasurement(int pitch)
{
    continuousFrequencyMeasurementPitch = pitch;
    if (state != stopped && state != finished)
        switchState(stopped);
    state = prepareContinuousFrequencyMeasurement;
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

    if (stopMeasurement)
    {
        startMeasurement = false;
        stopMeasurement = false;
        initialized = false;
    }
    
    if (startMeasurement)
    {
        // check if measurement was initialized
        if (!initialized)
        {
            lError = noError;
            sampleCounter = 0;
            lastZeroCrossing = -1;
            indexOfFirstValidPeriodLength = -1;
            periodLengthsHead = 0;
            initialized = true;
        }
        
        // try to find a zero crossing (- => +)
        for (int i = 0; i < numSamples; i++)
        {
            float currentSample = inputBuffer.getSample(0, i);
            if (lastSample < 0 && currentSample >= 0)
            {
                if (periodLengthsHead >= maxNumPeriodLengths)
                    break;
                
                // interpolate line between the sample before and after the crossing
                // y = mx + n
                double m = (lastSample - currentSample);
                double n = lastSample - m*(sampleCounter);
                
                // zero crossing of interpolated line: y = 0 => x0 = -n/m
                double zeroCrossingPos = -n / m;
                
                periodLengths[periodLengthsHead++] = zeroCrossingPos - lastZeroCrossing;
                lastZeroCrossing = zeroCrossingPos;                
            }
            lastSample = currentSample;
            sampleCounter++;
        }
        
        // see if the period length is stable
        if (periodLengthsHead > 5 && indexOfFirstValidPeriodLength < 0)
        {
            double sum = 0;
            for (int i = periodLengthsHead - 5; i < periodLengthsHead; i++)
            {
                sum += periodLengths[i];
            }
            double average = sum / 5.0;
            
            bool okay = true;
            double boundary = average * 0.1; // max 10% error allowed
            for (int i = periodLengthsHead - 5; i < periodLengthsHead; i++)
            {
                if (std::abs(periodLengths[i] - average) >= boundary)
                    okay = false;
            }
            
            if (okay)
            {
                indexOfFirstValidPeriodLength = periodLengthsHead;
            }
        }
        
        // finish measurement when the required number of valid measurements are made
        int numMeasurements = periodLengthsHead - indexOfFirstValidPeriodLength;
        if ((indexOfFirstValidPeriodLength > 0) && (numMeasurements > numPeriodSamples))
        {
            lError = noError;
            initialized = false;
            startMeasurement = false;
        }
        // the pitch hasn't stabilized yet.
        // assign the notStable error prematurely, just in case the top level statemachine runs into
        // a timeout and wants to know whats going on.
        else if (indexOfFirstValidPeriodLength < 0)
        {
            lError = notStable;
            
            // ran out of recording space => period length too jittery or does change constantly - stop here.
            if ((periodLengthsHead >= maxNumPeriodLengths))
            {
                initialized = false;
                startMeasurement = false;
            }
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
        stopMeasurement = true;
        listeners.call(&Listener::tunerStopped);
    }
    else if (newState == prepRefMeasurement)
        listeners.call(&Listener::tunerStarted);
    else if (newState == finished)
        listeners.call(&Listener::tunerFinished);
    
    listeners.call(&Listener::tunerStatusChanged, getStatusString());
}

/** inherited from AudioIODeviceCallback */
void VCOTuner::audioDeviceAboutToStart (AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
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
    switch(state)
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
    }
}

const String VCOTuner::Errors::highJitter = "There are zero crossings in the incoming signal but they don't seem to be coming in at a constant rate. Are you sure you're recording on the correct channel? Please use only primitive waveforms (saw, square, triangle, sine, ...) without any other processing such as delays, reverbs, etc. This error typically appears when you are accidentally recording the signal from a microphone or another sound source. Or when you have dropouts (aka clicks and pops) in your audio.";

const String VCOTuner::Errors::noZeroCrossings = "The incoming audio signal does not seem to contain any zero-crossings. Are you sure the oscillator signal is getting through to us? Check your audio device settings.";

const String VCOTuner::Errors::highJitterTimeOut = "Timeout. " + highJitter;

const String VCOTuner::Errors::stableTimeout = "There are some zero crossings in the incoming signal and they seem to come in at a constant rate - but they are coming in much slower than they should be. Are you recording from the right oscillator?";

const String VCOTuner::Errors::noFrequencyChangeBetweenMeasurements = "Apparently the frequency of the oscillator is not changing between measurements. Please check if your MIDI-to-CV interface is set to the correct MIDI channel and make sure that it is selected as the default midi output device in the audio and midi settings.";

const String VCOTuner::Errors::noMidiDeviceAvailable = "You don't have a MIDI output device selected or the selected device is not available.";

const String VCOTuner::Errors::audioDeviceStoppedDuringMeasurement = "The audio device was stopped while the measurement was still running. Please check that the device is still powered, all cables are connected and the driver is working correctly.";
