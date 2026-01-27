#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <array>
#include <cstddef>

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    
    // Compare 功能
    void performCompare();
    std::array<float, 8> getDiffArray() const;
    juce::String getCompareResultText() const;
    
    //==============================================================================
    
        bool isTargetReady() const;

        void beginCaptureSeconds (double seconds);
        bool hasTarget() const;
        juce::String getStatusText() const;

        std::array<float, 8> getTargetProfileArray() const;
        std::array<float, 8> getCurrentProfileArray() const;

        float getDiff (int index) const;
        int getSuggestionA() const;
        int getSuggestionB() const;
    
    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override = default;
    

    // 频谱数据获取函数
    std::array<float, 512> getSpectrumData() const;
    std::array<float, 512> getTargetSpectrumData() const;



   
   

private:

    juce::String statusText { "Ready" };
    
    // ====== UI: Two-column table ======

//
    // Compare 结果缓存
    std::array<std::atomic<float>, 8> diffValues;
    juce::String compareResultText { "" };


//
    static constexpr int kBands = 8;
    std::array<std::atomic<float>, kBands> currentEnvAtomic;
    
    //Timbre Compare MVP
    //8个对新手友好的维度 (0..1)
    struct TimbreProfile
    {
        float bright = 0.0f;  // 亮度
        float body   = 0.0f;  // 厚度(低中频)
        float bite   = 0.0f;  // 锐度(存在感)
        float air    = 0.0f;  // 空气感
        float noise  = 0.0f;  // 颗粒/噪声
        float width  = 0.0f;  // 宽度(立体声)
        float motion = 0.0f;  // 起伏/抖动
        float space  = 0.0f;  // 空间感(尾巴)
        
        std::array<float, 8> toArray() const
        {
            return { bright, body, bite, air, noise, width, motion, space};
        }
    };
    
    enum class TimbreDim : int
    {
        Bright = 0, Body, Bite, Air, Noise, Width, Motion, Space
    };
    
    // 给UI读的共享状态 (原子变量: 线程安全)
    std::atomic<bool> targetReady { false };
    std::array<std::atomic<float>, 8> diff01;  //target - current 范围大约 -1..+1)
    std::atomic<int> topSuggestionA { -1 };
    std::atomic<int> topSuggestionB { -1 };
    
    TimbreProfile currentProfile;
    std::array<std::atomic<float>, 8> current01; // 0..1
    
    // 频谱数据获取函数
    std::array<std::atomic<float>, 512> spectrumDataAtomic {};
    std::array<float, 512> targetSpectrumData {};

    
    //Capture相关: 抓2秒音频（先只声明，后面实现)
    std::atomic<bool> isCapturing {false};
    int captureWritePos = 0;
    double lastSampleRate = 44100.0;
    int captureLengthSamples = 0;
    juce::AudioBuffer<float> captureBuffer; //单声道抓取
    
    TimbreProfile targetProfile;
    
    //下面这些函数在PluginProcessor.cpp 里实现
    TimbreProfile analyseBufferToProfile (const juce::AudioBuffer<float>& monoBuffer, double sampleRate);
    TimbreProfile analyseCurrentBlockToProfile (const juce::AudioBuffer<float>& buffer, double sampleRate);
    //
    static constexpr int kFtBands = 96;

    static constexpr int kFFTOrder = 11;
    static constexpr int kFFTSize  = 1 << kFFTOrder;
    static constexpr int kHop      = 512;
    
    //

    std::array<float, kBands> targetEnv {};
    juce::SpinLock targetEnvLock;
    
    // ====== 音色分析相关 ======
    // 用于 Motion 计算（帧间变化）
    std::array<float, kBands> previousFrameEnergy {};
    bool hasPreviousFrame = false;
    
    // 辅助函数
    int frequencyToBin (float freqHz) const;


    //

    juce::dsp::FFT fft { kFFTOrder };
    juce::dsp::WindowingFunction<float> window { (size_t)kFFTSize,
        juce::dsp::WindowingFunction<float>::hann };

    std::array<float, kFFTSize> fifo {};
    int fifoIndex = 0;
    std::array<float, kFFTSize * 2> fftBuffer {};

    struct BandBinRange { int startBin=0, endBin=0; };
    std::array<BandBinRange, kBands> bandBins;
    bool bandBinsReady = false;



    void buildBandBinMapping();
    void pushSampleForEnvelope(float s);
    void computeCurrentEnvelopeFromFFT();

    std::array<float, kBands> analyseBufferToTargetEnvelope (const juce::AudioBuffer<float>& mono,
                                                             double sampleRate);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
