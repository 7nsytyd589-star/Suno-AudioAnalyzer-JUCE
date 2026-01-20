#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class AudioPluginAudioProcessor;

class AudioPluginAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AudioPluginAudioProcessor& processorRef;

    juce::Label titleLabel;
    juce::Label targetHeaderLabel;
    juce::Label currentHeaderLabel;

    std::array<juce::Label, 8> nameLabels;
    std::array<juce::Label, 8> targetValueLabels;
    std::array<juce::Label, 8> currentValueLabels;

    juce::Label statusLabel;
    
    void timerCallback() override;
    juce::TextButton captureButton { "Capture" };
    juce::TextButton compareButton { "Compare" };



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
