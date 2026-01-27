#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>

#include "RadarChartComponent.h"
#include "SpectrumWindow.h"

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

    // ====== 级别选择按钮 ======
    juce::TextButton introButton { "Introductory" };
    juce::TextButton intermediateButton { "Intermediate" };
    juce::TextButton advancedButton { "Advanced" };
    
    // ====== 弹出窗口 ======
    std::unique_ptr<SpectrumWindow> intermediateWindow;
    std::unique_ptr<SpectrumWindow> advancedWindow;

    // ====== 雷达图 ======
    RadarChartComponent radarChart;

    // ====== 其他控件 ======
    juce::Label titleLabel;
    juce::Label diffHeaderLabel;
    std::array<juce::Label, 8> diffValueLabels;
    juce::Label targetHeaderLabel;
    juce::Label currentHeaderLabel;
    std::array<juce::Label, 8> nameLabels;
    std::array<juce::Label, 8> targetValueLabels;
    std::array<juce::Label, 8> currentValueLabels;
    juce::Label statusLabel;
    juce::TextButton captureButton { "Capture" };
    juce::TextButton compareButton { "Compare" };

    void timerCallback() override;
    void openIntermediateWindow();
    void openAdvancedWindow();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
