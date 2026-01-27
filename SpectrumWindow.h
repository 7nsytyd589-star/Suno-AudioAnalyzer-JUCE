#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SpectrumComponent.h"

// 前向声明
class AudioPluginAudioProcessor;

class SpectrumWindow : public juce::DocumentWindow
{
public:
    SpectrumWindow (const juce::String& name, AudioPluginAudioProcessor& processor, bool isAdvanced);
    ~SpectrumWindow() override;

    void closeButtonPressed() override;
    
    // 更新频谱数据
    void updateSpectrum (const std::array<float, 512>& current, const std::array<float, 512>& target);

private:
    class ContentComponent;
    std::unique_ptr<ContentComponent> content;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumWindow)
};
