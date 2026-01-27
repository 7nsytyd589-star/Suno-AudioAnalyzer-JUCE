#include "RadarChartComponent.h"

RadarChartComponent::RadarChartComponent()
{
    targetData.fill (0.0f);
    currentData.fill (0.0f);
}

void RadarChartComponent::setTargetData (const std::array<float, 8>& data)
{
    targetData = data;
    repaint();
}

void RadarChartComponent::setCurrentData (const std::array<float, 8>& data)
{
    currentData = data;
    repaint();
}

void RadarChartComponent::setTargetColour (juce::Colour colour)
{
    targetColour = colour;
    repaint();
}

void RadarChartComponent::setCurrentColour (juce::Colour colour)
{
    currentColour = colour;
    repaint();
}

void RadarChartComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (10.0f);
    
    // 计算中心点和半径
    float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    float radius = size * 0.35f;  // 留空间给标签
    juce::Point<float> center (bounds.getCentreX(), bounds.getCentreY());
    
    // 1. 绘制背景网格
    drawBackground (g, center, radius);
    
    // 2. 绘制 Target 多边形 (填充 + 边框)
    drawDataPolygon (g, center, radius, targetData, targetColour, true);
    
    // 3. 绘制 Current 多边形 (只有边框，更粗)
    drawDataPolygon (g, center, radius, currentData, currentColour, false);
    
    // 4. 绘制维度标签
    drawLabels (g, center, radius);
    
    // 5. 绘制图例
    float legendY = bounds.getBottom() - 20.0f;
    float legendX = bounds.getX() + 10.0f;
    
    g.setColour (targetColour);
    g.fillRect (legendX, legendY, 12.0f, 12.0f);
    g.setColour (juce::Colours::white);
    g.setFont (11.0f);
    g.drawText ("Target", (int)(legendX + 16), (int)(legendY - 1), 50, 14, juce::Justification::left);
    
    g.setColour (currentColour);
    g.fillRect (legendX + 70, legendY, 12.0f, 12.0f);
    g.setColour (juce::Colours::white);
    g.drawText ("Current", (int)(legendX + 86), (int)(legendY - 1), 50, 14, juce::Justification::left);
}

void RadarChartComponent::resized()
{
    // 不需要特殊处理
}

void RadarChartComponent::drawBackground (juce::Graphics& g, juce::Point<float> center, float radius)
{
    // 绘制同心圆 (5 层)
    g.setColour (juce::Colours::grey.withAlpha (0.3f));
    for (int i = 1; i <= 5; ++i)
    {
        float r = radius * (float) i / 5.0f;
        g.drawEllipse (center.x - r, center.y - r, r * 2.0f, r * 2.0f, 1.0f);
    }
    
    // 绘制 8 条轴线
    g.setColour (juce::Colours::grey.withAlpha (0.5f));
    for (int i = 0; i < 8; ++i)
    {
        auto endPoint = getPointOnCircle (center, radius, i);
        g.drawLine (center.x, center.y, endPoint.x, endPoint.y, 1.0f);
    }
}

void RadarChartComponent::drawDataPolygon (juce::Graphics& g, juce::Point<float> center, float radius,
                                            const std::array<float, 8>& data, juce::Colour colour, bool filled)
{
    juce::Path path;
    
    for (int i = 0; i < 8; ++i)
    {
        float value = juce::jlimit (0.0f, 1.0f, data[(size_t) i]);
        float r = radius * value;
        auto point = getPointOnCircle (center, r, i);
        
        if (i == 0)
            path.startNewSubPath (point);
        else
            path.lineTo (point);
    }
    path.closeSubPath();
    
    if (filled)
    {
        // 填充半透明
        g.setColour (colour.withAlpha (0.2f));
        g.fillPath (path);
        
        // 边框
        g.setColour (colour.withAlpha (0.8f));
        g.strokePath (path, juce::PathStrokeType (2.0f));
    }
    else
    {
        // 只画边框，更粗
        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (3.0f));
    }
    
    // 绘制顶点圆点
    g.setColour (colour);
    for (int i = 0; i < 8; ++i)
    {
        float value = juce::jlimit (0.0f, 1.0f, data[(size_t) i]);
        float r = radius * value;
        auto point = getPointOnCircle (center, r, i);
        g.fillEllipse (point.x - 4.0f, point.y - 4.0f, 8.0f, 8.0f);
    }
}

void RadarChartComponent::drawLabels (juce::Graphics& g, juce::Point<float> center, float radius)
{
    g.setColour (juce::Colours::white);
    g.setFont (12.0f);
    
    float labelRadius = radius + 20.0f;
    
    for (int i = 0; i < 8; ++i)
    {
        auto point = getPointOnCircle (center, labelRadius, i);
        
        int textWidth = 60;
        int textHeight = 16;
        
        // 根据位置调整文字对齐
        juce::Justification just = juce::Justification::centred;
        int textX = (int) point.x - textWidth / 2;
        int textY = (int) point.y - textHeight / 2;
        
        // 顶部和底部的标签
        if (i == 0) // 顶部
        {
            textY -= 8;
        }
        else if (i == 4) // 底部
        {
            textY += 8;
        }
        // 左侧标签
        else if (i == 5 || i == 6 || i == 7)
        {
            textX -= 15;
            just = juce::Justification::right;
        }
        // 右侧标签
        else if (i == 1 || i == 2 || i == 3)
        {
            textX += 15;
            just = juce::Justification::left;
        }
        
        g.drawText (dimensionNames[(size_t) i], textX, textY, textWidth, textHeight, just);
    }
}

juce::Point<float> RadarChartComponent::getPointOnCircle (juce::Point<float> center, float radius, int index)
{
    // 从顶部开始，顺时针方向
    // index 0 = 顶部 (Bright)
    float angle = juce::MathConstants<float>::twoPi * (float) index / 8.0f - juce::MathConstants<float>::halfPi;
    
    float x = center.x + radius * std::cos (angle);
    float y = center.y + radius * std::sin (angle);
    
    return { x, y };
}
