#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

class SpectrumComponent : public juce::Component
{
public:
    SpectrumComponent();
    ~SpectrumComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    // 设置频谱数据 (FFT 结果，通常是 512 或 1024 个 bin)
    void setSpectrumData (const std::array<float, 512>& data);
    void setTargetSpectrumData (const std::array<float, 512>& data);
    
    // 清除 target
    void clearTarget();
    
    // 设置是否显示 target
    void setShowTarget (bool show);

private:
    std::array<float, 512> currentSpectrum {};
    std::array<float, 512> targetSpectrum {};
    bool hasTarget = false;
    bool showTarget = true;
    
    // 颜色
    juce::Colour currentColour { juce::Colours::cyan };
    juce::Colour targetColour { juce::Colours::orange };
    juce::Colour gridColour { juce::Colours::grey.withAlpha (0.3f) };
    
    // 绘制辅助函数
    void drawGrid (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> bounds,
                       const std::array<float, 512>& data, juce::Colour colour, bool filled);
    void drawFrequencyLabels (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawDecibelLabels (juce::Graphics& g, juce::Rectangle<float> bounds);
    
    // 频率转 X 坐标 (对数刻度)
    float frequencyToX (float freq, float width) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumComponent)
};
