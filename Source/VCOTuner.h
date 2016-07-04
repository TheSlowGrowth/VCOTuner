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
    bool isRunning() const { return state != stopped && state != finished; }
    
    void setNumMeasurementRange(int lowestPitch, int pitchIncrement, int highestPitch);
    int getLowestPitch() const { return lowestPitch; }
    int getPitchIncrement() const { return pitchIncrement; }
    int getHighestPitch() const { return highestPitch; }
    
    void setMidiChannel(int channel) { midiChannel = channel; }
    int  getMidiChannel() const { return midiChannel; }
    
    void setResolution(int numCyclesPerNote) { numPeriodSamples = numCyclesPerNote; }
    int getResolution() { return numPeriodSamples; }
    
    double getCurrentSampleRate() { return sampleRate; }
    double getReferenceFrequency() { return referenceFrequency; }
    int getReferencePitch() const { return referencePitch; }
    
    String getStatusString()const;
    
    void startContinuousMeasurement(int pitch);
    double getContinuousMesurementResult() const { return continuousFreqMeasurementResult; }
    
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
        prepRefMeasurement,
        refMeasurement,
        prepMeasurement,
        measurement,
        finished,
        prepareContinuousFrequencyMeasurement,
        continuousFrequencyMeasurement
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

    
    /** error message from the audio thread */
    enum LowLevelError
    {
        noError = 0,
        notStable // frequency not stable (= too much jitter)
    };
    
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
    MidiOutput* midiOut;
    int midiChannel;
    
    /** state of the state machine */
    State state;
    
    
    /** the following must only be accessed from the message thread, when startMeasurement == false and
     be accessed from the audio thread, when startMeasurement == true */
    bool startMeasurement; // set by message thread, reset by audio thread.
    bool stopMeasurement;  // set by message thread, reset by audio thread.
    static const int maxNumPeriodLengths = 600;
    int periodLengths[maxNumPeriodLengths]; // all measured period lengths of this measurement
    int numPeriodSamples; // number of periods to measure before averaging
    int indexOfFirstValidPeriodLength; // the index in periodLengths[] at which the system has reached a stable frequency
                                       // this is also the first valid period length measurement that is included in the result
    int periodLengthsHead;
    LowLevelError lError; // holds error message from the audio thread
    
    /** the following are only to be accessed from the audio thread */
    int sampleCounter; // counts samples since the start of a measurement
    int lastZeroCrossing; // holds the sample counters value of the last zero corssing (- => +)
    float lastSample;
    double sampleRate;
    bool initialized;
    
    int continuousFrequencyMeasurementPitch;
    double continuousFreqMeasurementResult;
    double continuousFreqMeasurementDeviation;
};


#endif  // VCOTUNER_H_INCLUDED
