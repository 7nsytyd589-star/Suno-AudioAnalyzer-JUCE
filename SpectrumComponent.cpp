#include "SpectrumComponent.h"
#include <cmath>

SpectrumComponent::SpectrumComponent()
{
    currentSpectrum.fill (0.0f);
    targetSpectrum.fill (0.0f);
}

void SpectrumComponent::setSpectrumData (const std::array<float, 512>& data)
{
    currentSpectrum = data;
    repaint();
}

void SpectrumComponent::setTargetSpectrumData (const std::array<float, 512>& data)
{
    targetSpectrum = data;
    hasTarget = true;
    repaint();
}

void SpectrumComponent::clearTarget()
{
    targetSpectrum.fill (0.0f);
    hasTarget = false;
    repaint();
}

void SpectrumComponent::setShowTarget (bool show)
{
    showTarget = show;
    repaint();
}

void SpectrumComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // 背景
    g.fillAll (juce::Colour (0xff1e1e1e));
    
    // 留出标签空间
    auto graphBounds = bounds.reduced (50.0f, 30.0f);
    graphBounds.removeFromBottom (25.0f);
    graphBounds.removeFromLeft (10.0f);
    
    // 绘制网格
    drawGrid (g, graphBounds);
    
    // 绘制 Target 频谱 (如果有)
    if (hasTarget && showTarget)
    {
        drawSpectrum (g, graphBounds, targetSpectrum, targetColour, true);
    }
    
    // 绘制 Current 频谱
    drawSpectrum (g, graphBounds, currentSpectrum, currentColour, false);
    
    // 绘制频率标签
    drawFrequencyLabels (g, graphBounds);
    
    // 绘制分贝标签
    drawDecibelLabels (g, graphBounds);
    
    // 图例
    float legendY = bounds.getBottom() - 20.0f;
    float legendX = graphBounds.getX();
    
    g.setColour (currentColour);
    g.fillRect (legendX, legendY, 12.0f, 12.0f);
    g.setColour (juce::Colours::white);
    g.setFont (11.0f);
    g.drawText ("Current", (int)(legendX + 16), (int)(legendY - 1), 50, 14, juce::Justification::left);
    
    if (hasTarget)
    {
        g.setColour (targetColour);
        g.fillRect (legendX + 80, legendY, 12.0f, 12.0f);
        g.setColour (juce::Colours::white);
        g.drawText ("Target", (int)(legendX + 96), (int)(legendY - 1), 50, 14, juce::Justification::left);
    }
    
    // 标题
    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText ("Spectrum Analyzer", bounds.removeFromTop (25.0f), juce::Justification::centred);
}

void SpectrumComponent::resized()
{
}

void SpectrumComponent::drawGrid (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour (gridColour);
    
    // 水平线 (分贝)
    for (int db = 0; db >= -60; db -= 10)
    {
        float y = juce::jmap ((float) db, 0.0f, -60.0f, bounds.getY(), bounds.getBottom());
        g.drawHorizontalLine ((int) y, bounds.getX(), bounds.getRight());
    }
    
    // 垂直线 (频率: 100Hz, 1kHz, 10kHz)
    std::array<float, 5> freqs = { 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f };
    for (float freq : freqs)
    {
        float x = bounds.getX() + frequencyToX (freq, bounds.getWidth());
        g.drawVerticalLine ((int) x, bounds.getY(), bounds.getBottom());
    }
    
    // 边框
    g.setColour (juce::Colours::grey);
    g.drawRect (bounds, 1.0f);
}

void SpectrumComponent::drawSpectrum (juce::Graphics& g, juce::Rectangle<float> bounds,
                                       const std::array<float, 512>& data, juce::Colour colour, bool filled)
{
    juce::Path path;
    
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float sampleRate = 44100.0f;
    const int numBins = 512;
    
    bool pathStarted = false;
    
    for (int i = 1; i < numBins; ++i)
    {
        // 计算这个 bin 对应的频率
        float freq = (float) i * sampleRate / (float) (numBins * 2);
        
        if (freq < minFreq || freq > maxFreq)
            continue;
        
        // X 坐标 (对数刻度)
        float x = bounds.getX() + frequencyToX (freq, bounds.getWidth());
        
        // Y 坐标 (分贝刻度)
        float magnitude = data[(size_t) i];
        float db = magnitude > 0.0f ? 20.0f * std::log10 (magnitude) : -60.0f;
        db = juce::jlimit (-60.0f, 0.0f, db);
        float y = juce::jmap (db, 0.0f, -60.0f, bounds.getY(), bounds.getBottom());
        
        if (!pathStarted)
        {
            path.startNewSubPath (x, y);
            pathStarted = true;
        }
        else
        {
            path.lineTo (x, y);
        }
    }
    
    if (filled && pathStarted)
    {
        // 闭合路径填充
        juce::Path fillPath (path);
        fillPath.lineTo (bounds.getRight(), bounds.getBottom());
        fillPath.lineTo (bounds.getX(), bounds.getBottom());
        fillPath.closeSubPath();
        
        g.setColour (colour.withAlpha (0.2f));
        g.fillPath (fillPath);
    }
    
    // 绘制线条
    g.setColour (colour.withAlpha (filled ? 0.6f : 1.0f));
    g.strokePath (path, juce::PathStrokeType (filled ? 1.5f : 2.0f));
}

void SpectrumComponent::drawFrequencyLabels (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour (juce::Colours::grey);
    g.setFont (10.0f);
    
    std::array<std::pair<float, const char*>, 6> labels = {{
        { 50.0f, "50" },
        { 100.0f, "100" },
        { 500.0f, "500" },
        { 1000.0f, "1k" },
        { 5000.0f, "5k" },
        { 10000.0f, "10k" }
    }};
    
    for (auto& [freq, text] : labels)
    {
        float x = bounds.getX() + frequencyToX (freq, bounds.getWidth());
        g.drawText (text, (int)(x - 15), (int)(bounds.getBottom() + 5), 30, 15, juce::Justification::centred);
    }
    
    // Hz 标签
    g.drawText ("Hz", (int)(bounds.getRight() - 20), (int)(bounds.getBottom() + 5), 30, 15, juce::Justification::left);
}

void SpectrumComponent::drawDecibelLabels (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setColour (juce::Colours::grey);
    g.setFont (10.0f);
    
    for (int db = 0; db >= -60; db -= 20)
    {
        float y = juce::jmap ((float) db, 0.0f, -60.0f, bounds.getY(), bounds.getBottom());
        juce::String text = juce::String (db) + " dB";
        g.drawText (text, (int)(bounds.getX() - 45), (int)(y - 7), 40, 14, juce::Justification::right);
    }
}

float SpectrumComponent::frequencyToX (float freq, float width) const
{
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    
    // 对数刻度
    float logMin = std::log10 (minFreq);
    float logMax = std::log10 (maxFreq);
    float logFreq = std::log10 (juce::jlimit (minFreq, maxFreq, freq));
    
    return width * (logFreq - logMin) / (logMax - logMin);
}
