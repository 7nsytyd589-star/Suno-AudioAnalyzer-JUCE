#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p)
{
    setSize (520, 520);
    

    
    setTopLeftPosition (100, 100);
    // ====== Title ======
    titleLabel.setText ("Suno Timbre Lab - MVP", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);
    
    
    // ===== 表格对齐方式（只需要设置一次） =====
    titleLabel.setJustificationType (juce::Justification::centred);
    
    targetHeaderLabel.setJustificationType (juce::Justification::centred);
    currentHeaderLabel.setJustificationType (juce::Justification::centred);


    for (size_t i = 0; i < nameLabels.size(); ++i)
    
    {
        // 中间维度名：左对齐（更像表格）
        nameLabels[i].setJustificationType (
            juce::Justification::centredLeft);

        // 数值：右对齐（所有小数点看起来会非常整齐）
        targetValueLabels[i].setJustificationType (
            juce::Justification::centredRight);

        currentValueLabels[i].setJustificationType (
            juce::Justification::centredRight);
    }


    // ====== Column headers ======
    targetHeaderLabel.setText ("Target", juce::dontSendNotification);
    targetHeaderLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (targetHeaderLabel);

    currentHeaderLabel.setText ("Current", juce::dontSendNotification);
    currentHeaderLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (currentHeaderLabel);

    // ====== Row names ======
    static const char* kNames[8] =
    {
        "Bright", "Body", "Bite", "Air", "Noise", "Width", "Motion", "Space"
    };

    for (size_t i = 0; i < 8; ++i)
    {
        nameLabels[i].setText (kNames[i], juce::dontSendNotification);
        nameLabels[i].setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (nameLabels[i]);

        targetValueLabels[i].setText ("-", juce::dontSendNotification);
        targetValueLabels[i].setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (targetValueLabels[i]);

        currentValueLabels[i].setText ("-", juce::dontSendNotification);
        currentValueLabels[i].setJustificationType (juce::Justification::centred);
        addAndMakeVisible (currentValueLabels[i]);
    }

    
    addAndMakeVisible (captureButton);
    addAndMakeVisible (compareButton);
    addAndMakeVisible (statusLabel);


    statusLabel.setText (processorRef.getStatusText(), juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);

    captureButton.onClick = [this]
    {
        processorRef.beginCaptureSeconds (2.0);
        statusLabel.setText (processorRef.getStatusText(), juce::dontSendNotification);
    };

    compareButton.onClick = [this]
    {
        // MVP：先只是刷新一下文字，下一步我们会做真正 compare
        statusLabel.setText ("Compare: (next step)", juce::dontSendNotification);
    };
    
  
    startTimerHz(10);


}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    
}


void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // ===== 全局 padding =====
    const int pad = 18;
    area.reduce(pad, pad);

    // ===== 顶部状态文字 =====
    const int statusH = 28;
    statusLabel.setBounds(area.removeFromTop(statusH));
    area.removeFromTop(12);

    // ===== 两个大按钮 =====
    const int buttonH = 56;
    const int buttonGap = 14;

    captureButton.setBounds(area.removeFromTop(buttonH));
    area.removeFromTop(buttonGap);
    compareButton.setBounds(area.removeFromTop(buttonH));
    area.removeFromTop(18);

    // ===== 标题 =====
    const int titleH = 26;
    titleLabel.setBounds(area.removeFromTop(titleH));
    area.removeFromTop(14);

    // ===== 表格区域 =====
    // 你现在有：Target | Name | Current
    // 做成：左右两列数值固定比例，中间 name 自适应
    auto table = area;

    // 行高：表头 + 8行
    const int headerH = 22;
    const int rowH    = 26;
    
    // 列宽策略（随窗口变）
    const int minValueCol = 90;   // Target/Current 最小宽度
    const int maxValueCol = 140;  // Target/Current 最大宽度
    const int nameColMin  = 110;

    // valueCol = clamp(table宽度 * 0.25)
    int valueCol = (int) std::round(table.getWidth() * 0.25);
    valueCol = juce::jlimit(minValueCol, maxValueCol, valueCol);

    // 确保中间 name 不会小到看不清
    int remainingForName = table.getWidth() - valueCol * 2;
    if (remainingForName < nameColMin)
    {
        // 如果太窄，就从 valueCol 里让一点出来
        int need = nameColMin - remainingForName;
        valueCol = juce::jmax(minValueCol, valueCol - (need + 1) / 2);
    }

    auto leftCol  = table.removeFromLeft(valueCol);
    auto rightCol = table.removeFromRight(valueCol);
    auto midCol   = table; // 剩下的就是 name 列

    // ===== 表头 =====
    targetHeaderLabel.setBounds(leftCol.removeFromTop(headerH));
    midCol.removeFromTop(headerH); // 中间表头留空（或你以后放 "Name"）
    currentHeaderLabel.setBounds(rightCol.removeFromTop(headerH));

    // 表头下方留一点空
    const int headerGap = 8;
    leftCol.removeFromTop(headerGap);
    midCol.removeFromTop(headerGap);
    rightCol.removeFromTop(headerGap);

    // ===== 8 行 =====
    for (size_t i = 0; i < nameLabels.size(); ++i)
    {
        targetValueLabels[i].setBounds(leftCol.removeFromTop(rowH));
        nameLabels[i].setBounds       (midCol .removeFromTop(rowH));
        currentValueLabels[i].setBounds(rightCol.removeFromTop(rowH));
    }
}


void AudioPluginAudioProcessorEditor::timerCallback()
{
    const auto current = processorRef.getCurrentProfileArray();
    for (size_t i = 0; i < 8; ++i)
    {
        currentValueLabels[i].setText (juce::String(current[i], 2), juce::dontSendNotification);
    }
}



