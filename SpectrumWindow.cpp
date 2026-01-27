#include "SpectrumWindow.h"
#include "PluginProcessor.h"

// 内部内容组件
class SpectrumWindow::ContentComponent : public juce::Component,
                                          private juce::Timer
{
public:
    ContentComponent (AudioPluginAudioProcessor& p, bool advanced)
        : processor (p), isAdvanced (advanced)
    {
        addAndMakeVisible (spectrum);
        
        if (isAdvanced)
        {
            // 高级模式：添加更多控件
            addAndMakeVisible (showTargetButton);
            showTargetButton.setButtonText ("Show Target");
            showTargetButton.setToggleState (true, juce::dontSendNotification);
            showTargetButton.onClick = [this]
            {
                spectrum.setShowTarget (showTargetButton.getToggleState());
            };
            
            addAndMakeVisible (captureButton);
            captureButton.setButtonText ("Capture Target");
            captureButton.onClick = [this]
            {
                processor.beginCaptureSeconds (2.0);
            };
        }
        
        startTimerHz (30);  // 30 FPS 更新
    }
    
    ~ContentComponent() override
    {
        stopTimer();
    }
    
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff2d2d2d));
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        
        if (isAdvanced)
        {
            auto topBar = area.removeFromTop (35);
            captureButton.setBounds (topBar.removeFromLeft (120));
            topBar.removeFromLeft (10);
            showTargetButton.setBounds (topBar.removeFromLeft (120));
            area.removeFromTop (10);
        }
        
        spectrum.setBounds (area);
    }
    
    void timerCallback() override
    {
        // 获取频谱数据并更新
        auto spectrumData = processor.getSpectrumData();
        spectrum.setSpectrumData (spectrumData);
        
        if (processor.hasTarget())
        {
            auto targetData = processor.getTargetSpectrumData();
            spectrum.setTargetSpectrumData (targetData);
        }
    }
    
    SpectrumComponent spectrum;
    
private:
    AudioPluginAudioProcessor& processor;
    bool isAdvanced;
    
    juce::ToggleButton showTargetButton;
    juce::TextButton captureButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentComponent)
};

SpectrumWindow::SpectrumWindow (const juce::String& name, AudioPluginAudioProcessor& processor, bool isAdvanced)
    : DocumentWindow (name,
                      juce::Colour (0xff2d2d2d),
                      DocumentWindow::closeButton | DocumentWindow::minimiseButton)
{
    content = std::make_unique<ContentComponent> (processor, isAdvanced);
    setContentOwned (content.release(), true);
    
    setSize (800, isAdvanced ? 500 : 450);
    setResizable (true, true);
    setUsingNativeTitleBar (true);
    
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

SpectrumWindow::~SpectrumWindow()
{
}

void SpectrumWindow::closeButtonPressed()
{
    setVisible (false);
}

void SpectrumWindow::updateSpectrum (const std::array<float, 512>& current, const std::array<float, 512>& target)
{
    // 如果需要外部更新，可以用这个方法
}
