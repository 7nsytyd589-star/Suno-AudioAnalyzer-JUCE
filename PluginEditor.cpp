#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p)
{
    setSize (900, 620);  // 加宽窗口以容纳雷达图
    setTopLeftPosition (100, 100);
    

    
    // ====== 级别选择按钮 ======
    addAndMakeVisible (introButton);
    addAndMakeVisible (intermediateButton);
    addAndMakeVisible (advancedButton);
    
    // 初始状态：Introductory 选中
    introButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkgreen);
        
    introButton.onClick = [this]
    {
            // 已经是当前模式，不做任何事
        introButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkgreen);
        intermediateButton.setColour (juce::TextButton::buttonColourId,
            getLookAndFeel().findColour (juce::TextButton::buttonColourId));
        advancedButton.setColour (juce::TextButton::buttonColourId,
            getLookAndFeel().findColour (juce::TextButton::buttonColourId));
    };
        
    intermediateButton.onClick = [this]
    {
        openIntermediateWindow();
        intermediateButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkblue);
        introButton.setColour (juce::TextButton::buttonColourId,
            getLookAndFeel().findColour (juce::TextButton::buttonColourId));
        advancedButton.setColour (juce::TextButton::buttonColourId,
            getLookAndFeel().findColour (juce::TextButton::buttonColourId));
    };
        
    advancedButton.onClick = [this]
    {
        openAdvancedWindow();
        advancedButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkred);
        introButton.setColour (juce::TextButton::buttonColourId,
            getLookAndFeel().findColour (juce::TextButton::buttonColourId));
        intermediateButton.setColour (juce::TextButton::buttonColourId,
            getLookAndFeel().findColour (juce::TextButton::buttonColourId));
    };
    
    // ====== 雷达图 ======
    addAndMakeVisible (radarChart);
    
    
    // ====== Title ======
    titleLabel.setText ("Suno Timbre Lab - MVP", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (20.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    // ====== Column headers ======
    targetHeaderLabel.setText ("Target", juce::dontSendNotification);
    targetHeaderLabel.setJustificationType (juce::Justification::centred);
    targetHeaderLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    addAndMakeVisible (targetHeaderLabel);

    currentHeaderLabel.setText ("Current", juce::dontSendNotification);
    currentHeaderLabel.setJustificationType (juce::Justification::centred);
    currentHeaderLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    addAndMakeVisible (currentHeaderLabel);
    
    diffHeaderLabel.setText ("Diff", juce::dontSendNotification);
    diffHeaderLabel.setJustificationType (juce::Justification::centred);
    diffHeaderLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    addAndMakeVisible (diffHeaderLabel);

    // ====== Row names & values ======
    static const char* kNames[8] =
    {
        "Bright", "Body", "Bite", "Air", "Noise", "Width", "Motion", "Space"
    };

    for (size_t i = 0; i < 8; ++i)
    {
        nameLabels[i].setText (kNames[i], juce::dontSendNotification);
        nameLabels[i].setJustificationType (juce::Justification::centredLeft);
        nameLabels[i].setFont (juce::Font (14.0f));
        addAndMakeVisible (nameLabels[i]);

        targetValueLabels[i].setText ("-", juce::dontSendNotification);
        targetValueLabels[i].setJustificationType (juce::Justification::centred);
        addAndMakeVisible (targetValueLabels[i]);

        currentValueLabels[i].setText ("-", juce::dontSendNotification);
        currentValueLabels[i].setJustificationType (juce::Justification::centred);
        addAndMakeVisible (currentValueLabels[i]);
        
        diffValueLabels[i].setText ("-", juce::dontSendNotification);
        diffValueLabels[i].setJustificationType (juce::Justification::centred);
        addAndMakeVisible (diffValueLabels[i]);
    }

    // ====== Buttons ======
    addAndMakeVisible (captureButton);
    addAndMakeVisible (compareButton);
    
    captureButton.onClick = [this]
    {
        processorRef.beginCaptureSeconds (2.0);
        statusLabel.setText (processorRef.getStatusText(), juce::dontSendNotification);
        
        // 清空 Diff 列
        for (size_t i = 0; i < 8; ++i)
            diffValueLabels[i].setText ("-", juce::dontSendNotification);
    };

    compareButton.onClick = [this]
    {
        processorRef.performCompare();
        statusLabel.setText (processorRef.getCompareResultText(), juce::dontSendNotification);
        
        // 更新 Diff 列显示
        auto diffs = processorRef.getDiffArray();
        for (size_t i = 0; i < 8; ++i)
        {
            float d = diffs[i];
            juce::String text;
            juce::Colour colour = juce::Colours::white;
            
            if (d > 0.01f)
            {
                text = juce::String::charToString (0x25B2) + " +" + juce::String (d, 2);  // ▲
                colour = juce::Colours::lightgreen;
            }
            else if (d < -0.01f)
            {
                text = juce::String::charToString (0x25BC) + " " + juce::String (d, 2);   // ▼
                colour = juce::Colours::salmon;
            }
            else
            {
                text = "=";
                colour = juce::Colours::grey;
            }
            
            diffValueLabels[i].setText (text, juce::dontSendNotification);
            diffValueLabels[i].setColour (juce::Label::textColourId, colour);
        }
    };

    // ====== Status Label ======
    addAndMakeVisible (statusLabel);
    statusLabel.setText (processorRef.getStatusText(), juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);
    statusLabel.setFont (juce::Font (13.0f));

    startTimerHz (10);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

//窗口打开函数
void AudioPluginAudioProcessorEditor::openIntermediateWindow()
{
    if (intermediateWindow == nullptr || !intermediateWindow->isVisible())
    {
        intermediateWindow = std::make_unique<SpectrumWindow> (
            "Intermediate - Spectrum Analyzer", processorRef, false);
    }
    intermediateWindow->toFront (true);
}

void AudioPluginAudioProcessorEditor::openAdvancedWindow()
{
    if (advancedWindow == nullptr || !advancedWindow->isVisible())
    {
        advancedWindow = std::make_unique<SpectrumWindow> (
            "Advanced - Spectrum Analyzer", processorRef, true);
    }
    advancedWindow->toFront (true);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    const int pad = 18;
    area.reduce (pad, pad);
    
    // ===== 顶部：级别选择按钮 =====
    auto topBar = area.removeFromTop (30);
    int buttonWidth = 110;
    int buttonGap = 8;
    
    introButton.setBounds (topBar.removeFromLeft (buttonWidth));
    topBar.removeFromLeft (buttonGap);
    intermediateButton.setBounds (topBar.removeFromLeft (buttonWidth));
    topBar.removeFromLeft (buttonGap);
    advancedButton.setBounds (topBar.removeFromLeft (buttonWidth));
    
    area.removeFromTop (15);
    
    // ===== 左侧：雷达图 =====
    auto leftPanel = area.removeFromLeft (320);
    radarChart.setBounds (leftPanel);
    
    // ===== 右侧：原有控件 =====
    area.removeFromLeft (20);
    
    // ===== 状态文字 =====
    const int statusH = 50;
    statusLabel.setBounds (area.removeFromTop (statusH));
    area.removeFromTop (12);

    // ===== 按钮 =====
    const int buttonH = 44;
    auto buttonArea = area.removeFromTop (buttonH);
    const int capButtonGap = 10;
    int capButtonWidth = (buttonArea.getWidth() - capButtonGap) / 2;
    
    captureButton.setBounds (buttonArea.removeFromLeft (capButtonWidth));
    buttonArea.removeFromLeft (capButtonGap);
    compareButton.setBounds (buttonArea);
    
    area.removeFromTop (18);

    // ===== 标题 =====
    const int titleH = 30;
    titleLabel.setBounds (area.removeFromTop (titleH));
    area.removeFromTop (14);

    // ===== 表格 =====
    auto table = area;
    const int headerH = 24;
    const int rowH = 28;
    
    int tableWidth = table.getWidth();
    int targetCol  = (int) (tableWidth * 0.18);
    int nameCol    = (int) (tableWidth * 0.28);
    int currentCol = (int) (tableWidth * 0.22);
    int diffCol    = tableWidth - targetCol - nameCol - currentCol;

    auto colTarget  = table.removeFromLeft (targetCol);
    auto colName    = table.removeFromLeft (nameCol);
    auto colCurrent = table.removeFromLeft (currentCol);
    auto colDiff    = table;

    targetHeaderLabel.setBounds (colTarget.removeFromTop (headerH));
    colName.removeFromTop (headerH);
    currentHeaderLabel.setBounds (colCurrent.removeFromTop (headerH));
    diffHeaderLabel.setBounds (colDiff.removeFromTop (headerH));

    const int headerGap = 8;
    colTarget.removeFromTop (headerGap);
    colName.removeFromTop (headerGap);
    colCurrent.removeFromTop (headerGap);
    colDiff.removeFromTop (headerGap);

    for (size_t i = 0; i < nameLabels.size(); ++i)
    {
        targetValueLabels[i].setBounds (colTarget.removeFromTop (rowH));
        nameLabels[i].setBounds (colName.removeFromTop (rowH));
        currentValueLabels[i].setBounds (colCurrent.removeFromTop (rowH));
        diffValueLabels[i].setBounds (colDiff.removeFromTop (rowH));
    }
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    // 更新 Current 列
    const auto current = processorRef.getCurrentProfileArray();
    for (size_t i = 0; i < 8; ++i)
    {
        currentValueLabels[i].setText (juce::String (current[i], 2), juce::dontSendNotification);
    }
    
    // 更新 Target 列
    std::array<float, 8> target {};
    if (processorRef.hasTarget())
    {
        target = processorRef.getTargetProfileArray();
        for (size_t i = 0; i < 8; ++i)
        {
            targetValueLabels[i].setText (juce::String (target[i], 2), juce::dontSendNotification);
        }
    }
    
    // 更新雷达图
    radarChart.setCurrentData (current);
    radarChart.setTargetData (target);
    
    // 更新状态文字
    if (!processorRef.hasTarget())
    {
        statusLabel.setText (processorRef.getStatusText(), juce::dontSendNotification);
    }
}



