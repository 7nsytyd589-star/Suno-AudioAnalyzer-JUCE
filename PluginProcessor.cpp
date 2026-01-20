#include "PluginProcessor.h"
#include "PluginEditor.h"
AudioPluginAudioProcessor::TimbreProfile
AudioPluginAudioProcessor::analyseBufferToProfile (const juce::AudioBuffer<float>& monoBuffer,
                                                   double sampleRate)
{
    juce::ignoreUnused (sampleRate);

    TimbreProfile p;

    if (monoBuffer.getNumSamples() <= 0)
        return p;

    // Reuse the simple RMS-based mock analysis so the target column updates
    const float rms = monoBuffer.getRMSLevel (0, 0, monoBuffer.getNumSamples());
    const float mockValue = juce::jlimit (0.0f, 1.0f, rms * 5.0f);

    p.bright = mockValue;
    p.body   = mockValue * 0.8f;
    p.bite   = mockValue * 1.2f;
    p.air    = mockValue * 0.5f;
    p.noise  = mockValue * 0.5f;
    p.width  = mockValue * 0.5f;
    p.motion = mockValue * 0.5f;
    p.space  = mockValue * 0.5f;

    return p;
}


AudioPluginAudioProcessor::TimbreProfile
AudioPluginAudioProcessor::analyseCurrentBlockToProfile (const juce::AudioBuffer<float>& buffer,
                                                         double sampleRate)
{
    juce::ignoreUnused (sampleRate);

    TimbreProfile p;

    if (buffer.getNumSamples() <= 0)
        return p;

    // Use channel 0 for a quick mock analysis, similar to analyseBufferToProfile
    const float rms = buffer.getRMSLevel (0, 0, buffer.getNumSamples());
    const float mockValue = juce::jlimit (0.0f, 1.0f, rms * 5.0f);

    p.bright = mockValue;
    p.body   = mockValue * 0.8f;
    p.bite   = mockValue * 1.2f;
    p.air    = mockValue * 0.5f;
    p.noise  = mockValue * 0.5f;
    p.width  = mockValue * 0.5f;
    p.motion = mockValue * 0.5f;
    p.space  = mockValue * 0.5f;

    return p;
}



//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                       )
#endif
{
    for (auto& a : currentEnvAtomic) a.store (0.0f, std::memory_order_relaxed);
    for (auto& a : current01)        a.store (0.0f, std::memory_order_relaxed);
    targetReady.store (false);
}





//
void AudioPluginAudioProcessor::beginCaptureSeconds (double seconds)
{
    // 1. 计算要抓多少 samples
        captureLengthSamples = (int) (seconds * lastSampleRate);

        // 2. 重置写指针 & 状态
        captureWritePos = 0;
        isCapturing = true;

        // 3. 准备 buffer（单声道）
        captureBuffer.setSize (1, captureLengthSamples);
        captureBuffer.clear();

        // 4. UI / 状态
        statusText = "Capturing target...";
        targetReady.store (false);
    statusText = "Capturing target... " + juce::String (seconds, 1) + "s";
    targetReady.store (false);
}


bool AudioPluginAudioProcessor::hasTarget() const
{
    return targetReady.load();
}


    juce::String AudioPluginAudioProcessor::getStatusText() const
    {
        return statusText;
   

    }

std::array<float, 8> AudioPluginAudioProcessor::getCurrentProfileArray() const
{
    std::array<float, 8> out{};
    for (int i = 0; i < 8; ++i)
        out[i] = current01[i].load(std::memory_order_relaxed);   // 关键：load atomic
    return out;
}

std::array<float, 8> AudioPluginAudioProcessor::getTargetProfileArray() const
{
    // 你 targetProfile 是普通 struct，但它只在 capture 完成时改一次，
    // UI 读它风险不大；更严谨也可以像 current 一样搞个 target01 atomic。
    return targetProfile.toArray();
}



//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    lastSampleRate = sampleRate;

    buildBandBinMapping();

    // FFT buffers init
    fifoIndex = 0;
    fifo.fill (0.0f);
    fftBuffer.fill (0.0f);

    // Capture buffer init (2 seconds default)
    captureLengthSamples = (int) (2.0 * sampleRate);
    captureWritePos = 0;
    isCapturing = false;

    captureBuffer.setSize (1, captureLengthSamples);
    captureBuffer.clear();
}

void AudioPluginAudioProcessor::buildBandBinMapping()
{
    const float fMin = 20.0f;
    const float fMax = (float) juce::jmin (20000.0, lastSampleRate * 0.5);

    const int nyquistBin = kFFTSize / 2;
    auto hzToBin = [this](float hz)
    {
        const float clamped = juce::jlimit(0.0f, (float)(lastSampleRate * 0.5), hz);
        return (int) std::floor(clamped * (float)kFFTSize / (float)lastSampleRate);
    };

    for (int b = 0; b < kBands; ++b)
    {
        const float t0 = (float)b / (float)kBands;
        const float t1 = (float)(b + 1) / (float)kBands;

        // log spacing
        const float f0 = fMin * std::pow(fMax / fMin, t0);
        const float f1 = fMin * std::pow(fMax / fMin, t1);

        int start = juce::jlimit(1, nyquistBin, hzToBin(f0));
        int end   = juce::jlimit(1, nyquistBin, hzToBin(f1));

        if (end <= start) end = juce::jmin(start + 1, nyquistBin);

        bandBins[(size_t)b] = { start, end };
    }

    bandBinsReady = true;
}



void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
 #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
 #else
    // 只允许 mono 或 stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // 如果不是 synth，输入输出要一致
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
 #endif
}



void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    
    // 1. 获取输入输出信息
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // 清理多余输出通道
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 2. --- 关键：捕获逻辑 ---
    // 这里使用 atomic 的 load 确保线程安全
    if (isCapturing)
    {
        int numSamples = buffer.getNumSamples();
        int remaining = captureLengthSamples - captureWritePos;
        int toCopy = std::min(numSamples, remaining);

        if (toCopy > 0)
        {
            // 写入 captureBuffer (单声道)
            captureBuffer.copyFrom(0, captureWritePos, buffer, 0, 0, toCopy);
            captureWritePos += toCopy;
        }

        // 检查是否录制结束
        if (captureWritePos >= captureLengthSamples)
        {
            isCapturing = false; // 停止录制
            
            // 执行捕获后的分析
            targetProfile = analyseBufferToProfile(captureBuffer, lastSampleRate);
            targetReady.store(true);
            statusText = "Target Captured";
        }
        else
        {
            // 更新 UI 进度提示
            statusText = "Capturing: " + juce::String((int)((float)captureWritePos / captureLengthSamples * 100)) + "%";
        }
    }

    // 3. --- 实时分析逻辑 (Current 列) ---
    // 即使不在录音，我们也需要实时更新 UI 的 Current 数值
    if (totalNumInputChannels > 0)
    {
        auto* channelData = buffer.getReadPointer(0);
        
        // 推入 FFT 队列
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            pushSampleForEnvelope(channelData[i]);
        
        // 更新实时 Profile
        currentProfile = analyseCurrentBlockToProfile(buffer, lastSampleRate);
        auto arr = currentProfile.toArray();
        // 在 processBlock 里的最后一行循环改为：
        for (int i = 0; i < kBands; ++i)
        {
            // 直接读你在 computeCurrentEnvelopeFromFFT 里存进去的 FFT 能量
            float val = currentEnvAtomic[i].load();
            current01[i].store(juce::jlimit(0.0f, 1.0f, val * 10.0f), std::memory_order_relaxed);
        }
    }
}

//
void AudioPluginAudioProcessor::pushSampleForEnvelope(float s)
{
    fifo[(size_t)fifoIndex++] = s;

    if (fifoIndex >= kFFTSize)
    {
        // 复制到 fftBuffer 并加窗
        for (int i = 0; i < kFFTSize; ++i)
            fftBuffer[(size_t)i] = fifo[(size_t)i];
        
        window.multiplyWithWindowingTable(fftBuffer.data(), kFFTSize);
        
        // JUCE: real-only forward
        fft.performRealOnlyForwardTransform(fftBuffer.data());
        
        // 计算 bands（power -> 0..1）
        if (bandBinsReady)
            computeCurrentEnvelopeFromFFT();
        
        // hop: 把 fifo 左移 kHop
        const int remain = kFFTSize - kHop;
        for (int i = 0; i < remain; ++i)
            fifo[(size_t)i] = fifo[(size_t)(i + kHop)];
        
        fifoIndex = remain;
    }
}
//
void AudioPluginAudioProcessor::computeCurrentEnvelopeFromFFT()
{
    if (! bandBinsReady)
        buildBandBinMapping();

    std::array<float, kBands> env {};
    env.fill (0.0f);

    for (int b = 0; b < kBands; ++b)
    {
        float sum = 0.0f;
        int count = 0;

        for (int bin = bandBins[b].startBin;
             bin <= bandBins[b].endBin;
             ++bin)
        {
            float mag = std::abs (fftBuffer[(size_t) bin]);
            sum += mag;
            ++count;
        }

        env[b] = (count > 0 ? sum / (float) count : 0.0f);
    }

    // 写入 atomic（给 UI 用）
    for (int i = 0; i < kBands; ++i)
        currentEnvAtomic[i].store (env[i], std::memory_order_relaxed);
}
    


//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
//函数签名
std::array<float, AudioPluginAudioProcessor::kBands>
AudioPluginAudioProcessor::analyseBufferToTargetEnvelope (const juce::AudioBuffer<float>& mono,
                                                          double /*sampleRate*/)
{
    std::array<float, kBands> out {};
    out.fill (0.0f);

    // TODO: 这里以后你再写真正的分析逻辑（FFT 分 band / 能量包络等）
    // 先返回全 0，只为了解决 Undefined symbol 让工程先能跑起来。

    return out;
}


//

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
