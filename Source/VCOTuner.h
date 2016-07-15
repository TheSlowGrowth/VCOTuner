/*
  ==============================================================================

    VCOTuner.h
    Created: 17 May 2016 8:21:15pm
    Author:  Johannes Neumann

  ==============================================================================
*/

#ifndef VCOTUNER_H_INCLUDED
#define VCOTUNER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class VCOTuner: public ChangeListener,
                private Timer,
                public AudioIODeviceCallback
{
public:
    VCOTuner(AudioDeviceManager* deviceManager);
    ~VCOTuner();
    
    void toggleState();
    void start();
    void stop();
    bool isRunning() const { return state != stopped && state != finished; }
    
    void setNumMeasurementRange(int lowestPitch, int pitchIncrement, int highestPitch);
    int getLowestPitch() const { return lowestPitch; }
    int getPitchIncrement() const { return pitchIncrement; }
    int getHighestPitch() const { return highestPitch; }
    
    void setMidiChannel(int channel) { midiChannel = channel; }
    int  getMidiChannel() const { return midiChannel; }
    
    void setResolution(int numCyclesPerNote) { /*numPeriodSamples = numCyclesPerNote;*/ }
    int getResolution() { return 0; /* numPeriodSamples; */ }
    
    double getCurrentSampleRate() { return sampleRate; }
    double getReferenceFrequency() { return referenceFrequency; }
    int getReferencePitch() const { return referencePitch; }
    
    String getStatusString()const;
    
    void startContinuousMeasurement(int pitch);
    double getContinuousMesurementResult() const { return 0; /* continuousFreqMeasurementResult;*/ }
    
    void startSingleMeasurement(int pitch);
    double getSingleMeasurementResult() const { return 0; /* singleMeasurementResult; */ }
    
    /** holds all properties of a single measurements */
    typedef struct
    {
        int midiPitch;
        double frequency;
        double pitch; // according to the measured reference pitch
        double pitchOffset; // pitch - midiPitch
        double freqDeviation;
        double pitchDeviation;
        int numMeasurements;
        Time timestamp;
    } measurement_t;
    
    /** returns all error messages and removes them from the internal list */
    StringArray getLastErrors();
    
    /** inherited from AudioIODeviceCallback */
    virtual void audioDeviceIOCallback (const float** inputChannelData,
                                        int numInputChannels,
                                        float** outputChannelData,
                                        int numOutputChannels,
                                        int numSamples);
    
    /** inherited from AudioIODeviceCallback */
    virtual void audioDeviceAboutToStart (AudioIODevice* device);
    
    /** inherited from AudioIODeviceCallback */
    virtual void audioDeviceStopped();
    
    /** inherited from ChangeListener */
    virtual void changeListenerCallback (ChangeBroadcaster* source);
    
    class Listener
    {
    public:
        virtual ~Listener() {}
        
        virtual void newMeasurementReady(const measurement_t& /*m*/) {}
        virtual void tunerStarted() {}
        virtual void tunerStopped() {}
        virtual void tunerFinished() {}
        virtual void tunerStatusChanged(String statusString) {}
    };
    
    void addListener(Listener* l);
    void removeListener(Listener* l);
    
private:
    // states for the state machine
    enum State
    {
        stopped,
        prepMeasurement,
        sampleInput,
        processSkim,
        processFine,
        finished
    };
    
    ListenerList<Listener> listeners;
    
    // processes the state machine
    virtual void timerCallback();
    void switchState(State newState);
    void trySendMidiNoteOn(int pitch);
    void trySendMidiNoteOff(int pitch);
    int currentlyPlayingMidiNote;
    
    // counts cycles since the last state transition
    int cycleCounter;
    
    /** lowest pitch to be measured */
    int lowestPitch;
    /** pitch increment */
    int pitchIncrement;
    /** highest pitch to be measured */
    int highestPitch;
    
    int currentPitch;
    int currentIndex;
    
    /** midi note for which the reference measurement was done. */
    int referencePitch;
    /** frequency returned during the reference measurement */
    float referenceFrequency;
    
    /** a list with recent error messages */
    StringArray errors;
    
    AudioDeviceManager* deviceManager;
    int midiChannel;
    
    /** state of the state machine */
    State state;
    
    /** the following must only be accessed from the message thread, when startMeasurement == false and
     be accessed from the audio thread, when startMeasurement == true */
    bool startSampling; // set by message thread, reset by audio thread.
    bool stopSampling;  // set by message thread, reset by audio thread.
    
    AudioSampleBuffer sampleBuffer;
    Array<double> correlationSkimResults;
    
    /** the following are only to be accessed from the audio thread */
    int sampleBufferHead;
    float lastSample;
    double sampleRate;
    bool initialized;
    
    struct Errors
    {
        static const String highJitter;
        static const String noZeroCrossings;
        static const String highJitterTimeOut;
        static const String stableTimeout;
        static const String noFrequencyChangeBetweenMeasurements;
        static const String noMidiDeviceAvailable;
        static const String audioDeviceStoppedDuringMeasurement;
    };
};


#endif  // VCOTUNER_H_INCLUDED
