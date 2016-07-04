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
    midiOut = nullptr;
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
    
    if (midiOut != nullptr)
    {
        delete midiOut;
        midiOut = nullptr;
    }
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
                    errors.add("The pitch on the audio input is not stable. This can be due to excessive jitter or a frequency modulation on the oscillator. Please note that the recognition only works for 'simple' waveforms with two zero-crossings per cycle. Please choose Saw, Triangle, Sine, Pulse, etc.");
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
                    
                    referenceFrequency = sampleRate / averagePeriod;
                    
                    // prepare next measurement
                    currentPitch = lowestPitch;
                    currentIndex = 0;
                    switchState(prepMeasurement);
                    break;
                }
            }

            if (cycleCounter > 1000)
            {
                errors.add("The incoming audio signal does not seem to contain any zero-crossings. Are you sure the oscillator signal is getting through to us? Check your audio device settings.");
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
                    errors.add("The pitch on the audio input is not stable. This can be due to excessive jitter or a frequency modulation on the oscillator. Please note that the recognition only works for 'simple' waveforms with two zero-crossings per cycle. Please choose Saw, Triangle, Sine, Pulse, etc.");
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
                    double pitch = 12.0 * log(frequency / referenceFrequency) / log(2.0) + referencePitch;
                    
                    // check if the frequency has changed compared to the reference frequency
                    // if not, it is likely that the MIDI output is not working. Do this only for the very first measurement
                    if (currentIndex == 0)
                    {
                        if (std::abs(frequency - referenceFrequency)/referenceFrequency < 0.1)
                        {
                            errors.add("Apparently the frequency of the oscillator is not changing between measurements. Please check if your MIDI-to-CV interface is set to MIDI channel " + String(midiChannel) + " and make sure that it is selected as the default midi output device in the audio and midi settings.");
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
            
            float expectedFrequency = referenceFrequency * pow(2,((float) currentPitch - (float) referencePitch)/12.0);
            float expectedTime = 1.0 / (float) expectedFrequency * numPeriodSamples;
            expectedTime *= 2;
            int expectedCycles = expectedTime * 100;
            if (cycleCounter > expectedCycles)
            {
                errors.add("The incoming audio signal does not seem to contain any zero-crossings. Are you sure the oscillator signal is getting through? Check your audio device settings!");
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
        } break;
        default:
            state = stopped;
            break;
    }
}

void VCOTuner::startContinuousMeasurement(int pitch)
{
    continuousFrequencyMeasurementPitch = pitch;
    if (state != stopped)
        switchState(stopped);
    state = prepareContinuousFrequencyMeasurement;
}

void VCOTuner::trySendMidiNoteOn(int pitch)
{
    if (midiOut == nullptr)
        return;
    
    if (currentlyPlayingMidiNote != -1)
        trySendMidiNoteOff(currentlyPlayingMidiNote);
    
    midiOut->sendMessageNow(MidiMessage::noteOn(midiChannel, pitch, (uint8_t) 100));
    currentlyPlayingMidiNote = pitch;
}

void VCOTuner::trySendMidiNoteOff(int pitch)
{
    if (midiOut == nullptr)
        return;
    
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
                
                periodLengths[periodLengthsHead++] = sampleCounter - lastZeroCrossing;
                lastZeroCrossing = sampleCounter;                
            }
            lastSample = currentSample;
            sampleCounter++;
        }
        
        // see if the period length is stable
        if (periodLengthsHead > 5 && indexOfFirstValidPeriodLength < 0)
        {
            int sum = 0;
            for (int i = periodLengthsHead - 5; i < periodLengthsHead; i++)
            {
                sum += periodLengths[i];
            }
            float average = (float) sum / 5.0;
            
            bool okay = true;
            float boundary = average * 0.1; // max 10% error allowed
            for (int i = periodLengthsHead - 5; i < periodLengthsHead; i++)
            {
                if (std::abs((float) periodLengths[i] - average) >= boundary)
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
            initialized = false;
            startMeasurement = false;
        }
        // ran out of recording space => period length too jittery or does change constantly
        else if (periodLengthsHead >= maxNumPeriodLengths)
        {
            lError = notStable;
            
            initialized = false;
            startMeasurement = false;
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
    switchState(stopped);
}

void VCOTuner::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == deviceManager)
    {
        if (midiOut != nullptr)
        {
            delete midiOut;
            midiOut = nullptr;
        }
        
        midiOut = MidiOutput::openDevice(MidiOutput::getDefaultDeviceIndex());
        if (midiOut == nullptr)
        {
            errors.add("Could not open any midi device");
            switchState(stopped);
        }
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
        default:
            return "";
            break;
    }
}
