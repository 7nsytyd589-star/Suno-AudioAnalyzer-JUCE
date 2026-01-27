#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

class RadarChartComponent : public juce::Component
{
public:
    RadarChartComponent();
    ~RadarChartComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    // 设置 Target 和 Current 数据 (0.0 - 1.0)
    void setTargetData (const std::array<float, 8>& data);
    void setCurrentData (const std::array<float, 8>& data);
    
    // 设置颜色
    void setTargetColour (juce::Colour colour);
    void setCurrentColour (juce::Colour colour);

private:
    std::array<float, 8> targetData {};
    std::array<float, 8> currentData {};
    
    juce::Colour targetColour { juce::Colours::orange };
    juce::Colour currentColour { juce::Colours::cyan };
    
    // 8 个维度的名称
    const std::array<juce::String, 8> dimensionNames = {
        "Bright", "Body", "Bite", "Air", "Noise", "Width", "Motion", "Space"
    };
    
    // 绘制辅助函数
    void drawBackground (juce::Graphics& g, juce::Point<float> center, float radius);
    void drawDataPolygon (juce::Graphics& g, juce::Point<float> center, float radius,
                          const std::array<float, 8>& data, juce::Colour colour, bool filled);
    void drawLabels (juce::Graphics& g, juce::Point<float> center, float radius);
    
    juce::Point<float> getPointOnCircle (juce::Point<float> center, float radius, int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadarChartComponent)
};
