#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>

/**
 * @class Arpeggiator
 * @brief 专为 8-bit Chiptune 风格设计的极速琶音器
 * * 采用私有缓冲隔离架构，完美解决 VST3 事件越界崩溃问题与 MIDI 挂音问题。
 */
class Arpeggiator
{
public:
    Arpeggiator() {}
    ~Arpeggiator() {}

    // 初始化采样率并重置所有状态
    void prepareToPlay(double newSampleRate)
    {
        sampleRate = newSampleRate;
        timeInSamples = 0;
        currentNoteIndex = 0;
        currentPlayingNote = -1;
        heldNotes.clear();

        // 默认设置一个琶音速度（例如每秒弹奏 15 个音符，产生极速机枪感）
        setSpeed(15.0f);
    }

    // 设置琶音速度（频率：Hz）。你可以稍后将其连接到 APVTS 的参数上
    void setSpeed(float speedInHz)
    {
        if (speedInHz > 0.0f && sampleRate > 0.0) {
            // 计算每个音符应该持续的采样点数
            noteDuration = static_cast<int>(sampleRate / speedInHz);
        }
    }

    /**
     * @brief 核心 DSP 逻辑
     * @param inputMidi 只读的宿主输入（用于收集用户按键）
     * @param outputMidi 私有输出缓冲（专门喂给 Synth 的机枪音符）
     * @param numSamples 当前音频块的采样总数
     */
    void processBlock(const juce::MidiBuffer& inputMidi, juce::MidiBuffer& outputMidi, int numSamples, bool isOn)
    {

        if (!isOn)
        {
            // 1. 如果正在琶音时关闭开关，立即发送 Note Off 防止挂音
            if (currentPlayingNote != -1)
            {
                outputMidi.addEvent(juce::MidiMessage::noteOff(1, currentPlayingNote), 0);
                currentPlayingNote = -1;
            }
            // 2. 旁路模式：直接将输入的 MIDI 信号原封不动拷贝到输出
            outputMidi.addEvents(inputMidi, 0, numSamples, 0);
            heldNotes.clear();
            return;
        }

        // ====================================================================
        // 第一层：输入收集（Input Collection） - 只读不写，只更新内部数组
        // ====================================================================
        for (const auto meta : inputMidi)
        {
            auto msg = meta.getMessage();

            // 如果是按下音符
            if (msg.isNoteOn())
            {
                int note = msg.getNoteNumber();
                // 确保数组里没有重复的音，再加进去
                if (std::find(heldNotes.begin(), heldNotes.end(), note) == heldNotes.end())
                {
                    heldNotes.push_back(note);
                    // 【可选】按音高从小到大排序，产生向上琶音效果
                    std::sort(heldNotes.begin(), heldNotes.end());
                }
            }
            // 如果是松开音符
            else if (msg.isNoteOff())
            {
                int note = msg.getNoteNumber();
                // 从数组中剔除这个音
                heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
            }
        }

        // ====================================================================
        // 第二层：终极挂音兜底（Emergency Stop）
        // ====================================================================
        if (heldNotes.empty())
        {
            // 用户手全松开了。检查琶音器是不是还让合成器响着某个音？
            if (currentPlayingNote != -1)
            {
                // 紧急刹车：向私有缓冲的第 0 个采样点发送这个音的 Note Off
                outputMidi.addEvent(juce::MidiMessage::noteOff(1, currentPlayingNote), 0);

                // 重置所有状态哨兵
                currentPlayingNote = -1;
            }

            // 时间和步进索引归零，直接结束这一个 Block 的处理
            timeInSamples = 0;
            currentNoteIndex = 0;
            return;
        }

        // ====================================================================
        // 第三层：采样级时钟与发声逻辑（Arp Logic & Output）
        // ====================================================================
        // 遍历这一个 Block 里的每一个采样点，确保 MIDI 事件卡在绝对精确的时间点上
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // 触发条件：如果是刚按下第一个键（哨兵为 -1），或者内部计时器走到了该切音的时间
            if (currentPlayingNote == -1 || timeInSamples >= noteDuration)
            {
                // 1. 先“擦屁股”：关掉上一个正在响的音
                if (currentPlayingNote != -1)
                {
                    outputMidi.addEvent(juce::MidiMessage::noteOff(1, currentPlayingNote), sample);
                }

                // 2. 挑选下一个要响的音符
                if (currentPlayingNote == -1)
                {
                    // 刚刚弹下，从头开始
                    currentNoteIndex = 0;
                }
                else
                {
                    // 索引向后移动一位
                    currentNoteIndex++;
                    // 如果超出了数组长度，就绕回第一个音，形成循环
                    if (currentNoteIndex >= heldNotes.size()) {
                        currentNoteIndex = 0;
                    }
                }

                // 3. 生成新音符并发射
                int nextNote = heldNotes[currentNoteIndex];
                // 发送新音符的 Note On（使用默认力度 1.0f 或者 127），精确附着在当前的 sample 位置
                outputMidi.addEvent(juce::MidiMessage::noteOn(1, nextNote, 1.0f), sample);

                // 4. 更新状态哨兵和计时器
                currentPlayingNote = nextNote;
                timeInSamples = 0;
            }
            else
            {
                // 如果还没到切音的时间，计时器继续往前走
                timeInSamples++;
            }
        }
    }

private:
    // --- 音符状态追踪区 ---
    std::vector<int> heldNotes;     // 存储当前被按下的所有 MIDI 音符
    int currentPlayingNote = -1;    // 挂音哨兵：当前正在被琶音器“按着”的那个具体音符
    size_t currentNoteIndex = 0;    // 当前轮到了数组里的哪一个音

    // --- 时钟与计步区 ---
    double sampleRate = 44100.0;    // 宿主采样率
    int timeInSamples = 0;          // 内部采样点计数器
    int noteDuration = 0;           // 一个琶音音符持续的采样点总数阈值
};