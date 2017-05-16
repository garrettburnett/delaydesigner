/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <string>
using namespace std;


//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    addParameter(wetMixParameter = new AudioParameterFloat("Wet Mix", //ID
                                                           "Wet Mix", //NAME
                                                           0.0f,//min
                                                           1.0f,//max
                                                           0.5f));//default
    
    addParameter(dryMixParameter = new AudioParameterFloat("Dry Mix", //ID
                                                           "Dry Mix", //NAME
                                                           0.0f,//min
                                                           1.0f,//max
                                                           0.5f));//default
    
    for(int i =0; i<numDelays;i++){
        addParameter(delayParameters[i] =
            new AudioParameterFloat("Dry Level " + to_string(i+1), //ID
                                    "Delay Level " + to_string(i+1), //NAME
                                    0.0f,//min
                                    1.0f,//max
                                    0.5f));//default
    }
    
    
    
}

NewProjectAudioProcessor::~NewProjectAudioProcessor()
{
}

//==============================================================================
const String NewProjectAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NewProjectAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

double NewProjectAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NewProjectAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NewProjectAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NewProjectAudioProcessor::setCurrentProgram (int index)
{
}

const String NewProjectAudioProcessor::getProgramName (int index)
{
    return {};
}

void NewProjectAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for(int i = 0; i<numDelays; i++){ //initialize delays and set offsets.
        echos[i].initialize(sampleRate);
        echos[i].offset = (i+1)*2; //quarter notes
    }
}

void NewProjectAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NewProjectAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
 
    syncBPM();
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);
        rwIn(channel);
        for(int i =0;i< numSamples; ++i){
            const float in = channelData[i];
            rwOut(in);
            channelData[i] = effectOut(in); //the final output data
        }
        // ..do something to the data...
    }
    rwClean(); //prepare for the next data loop
    
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    MemoryOutputStream (destData, true).writeFloat(*wetMixParameter);
    
    MemoryOutputStream (destData, true).writeFloat(*dryMixParameter);
    
     for(int i = 0; i<numDelays; i++){
         MemoryOutputStream (destData, true).writeFloat(*delayParameters[i]);
     }
    
    
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    *wetMixParameter = MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat();
     *dryMixParameter = MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat();
    
    for(int i = 0; i<numDelays; i++){
        *delayParameters[i] = MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat();

        
    }
}

float NewProjectAudioProcessor::getBPM(){
    
    playHead = getPlayHead();
    playHead->getCurrentPosition(currentPositionInfo);
    float hostBPM = currentPositionInfo.bpm;
    return hostBPM;
    
}

void NewProjectAudioProcessor::syncBPM(){ //maintains sychronicity of delay buffers based on host bpm.
    
     for(int i = 0; i<numDelays; i++){
         echos[i].setDelayLength(getBPM(), getSampleRate());
         echos[i].delayReadPosition = int(echos[i].delayWritePosition - echos[i].delayInSamples + echos[i].delayBufferLength) %echos[i].delayBufferLength;
     }
}

void NewProjectAudioProcessor::rwIn(int _channel){ //sets upthe temp read writes for each process block
    for(int i = 0; i<numDelays; i++){
        delayData[i] =  echos[i].delayData(_channel);
        echos[i].dpr=echos[i].delayReadPosition;
        echos[i].dpw=echos[i].delayWritePosition;
    }
}

void NewProjectAudioProcessor::rwOut(float _in){
    for(int i = 0; i<numDelays; i++){
        delayData[i][echos[i].dpw] = _in + (delayData[i][echos[i].dpr] * 0); //actually writes delayed data to the delay data buffer.
        
        if (++echos[i].dpr >= echos[i].delayBufferLength)
            echos[i].dpr = 0;
        
        if (++echos[i].dpw >= echos[i].delayBufferLength)
            echos[i].dpw = 0;
    }
}

void NewProjectAudioProcessor::rwClean(){ //reset after each process block
    for(int i = 0; i<numDelays; i++){
        echos[i].delayReadPosition = echos[i].dpr;
        echos[i].delayWritePosition = echos[i].dpw;
    }
}

float NewProjectAudioProcessor::effectOut(float _in){
    
    float effectOut = (*dryMixParameter * _in); //first adds the dry input
    
    for(int i = 0; i<numDelays; i++){
        effectOut+=(*wetMixParameter*delayData[i][echos[i].dpr] * *delayParameters[i] ); //then comes the wet outputs from each delay, multiplied by the global wetMixParameter.
        delayGain[i] = *delayParameters[i];
    }
    
    wetMix = *wetMixParameter;
    dryMix = *dryMixParameter;
    return effectOut;
}


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}