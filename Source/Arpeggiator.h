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
                // 新增：同时获取音高和通道
                ArpNote newNote{ msg.getNoteNumber(), msg.getChannel() };
                // 确保数组里没有重复的音，再加进去
                if (std::find(heldNotes.begin(), heldNotes.end(), newNote) == heldNotes.end())
                {
                    heldNotes.push_back(newNote);
                    std::sort(heldNotes.begin(), heldNotes.end()); // 依然支持向上琶音
                }
            }
            // 如果是松开音符
            else if (msg.isNoteOff())
            {
                ArpNote oldNote{ msg.getNoteNumber(), msg.getChannel() };
                heldNotes.erase(std::remove(heldNotes.begin(), heldNotes.end(), oldNote), heldNotes.end());
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
                // 修改：使用记录下来的通道号来关闭音符
                outputMidi.addEvent(juce::MidiMessage::noteOff(currentPlayingChannel, currentPlayingNote), 0);
                currentPlayingNote = -1;
                currentPlayingChannel = -1; // 重置通道哨兵
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
                    outputMidi.addEvent(juce::MidiMessage::noteOff(currentPlayingChannel, currentPlayingNote), sample);
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
                ArpNote nextNote = heldNotes[currentNoteIndex]; // 修改：取出来的是个结构体
                // 修改：精准附着对应的通道号
                outputMidi.addEvent(juce::MidiMessage::noteOn(nextNote.channel, nextNote.note, 1.0f), sample);

                // 4. 更新状态哨兵和计时器
                currentPlayingNote = nextNote.note;
                currentPlayingChannel = nextNote.channel; // 更新通道哨兵
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

    // 新增：定义一个小结构体，把音高和通道号死死绑定在一起
    struct ArpNote {
        int note;
        int channel;
        // 重载运算符，让 std::find 和 std::sort 能直接看懂这个结构体
        bool operator==(const ArpNote& other) const { return note == other.note && channel == other.channel; }
        bool operator<(const ArpNote& other) const { return note < other.note; }
    };

    // --- 音符状态追踪区 ---
    std::vector <ArpNote> heldNotes;     // 存储当前被按下的所有 MIDI 音符
    int currentPlayingNote = -1;    // 挂音哨兵：当前正在被琶音器“按着”的那个具体音符
    int currentPlayingChannel = -1; // 新增：挂音哨兵也要记住当前发声的通道
    size_t currentNoteIndex = 0;    // 当前轮到了数组里的哪一个音

    // --- 时钟与计步区 ---
    double sampleRate = 44100.0;    // 宿主采样率
    int timeInSamples = 0;          // 内部采样点计数器
    int noteDuration = 0;           // 一个琶音音符持续的采样点总数阈值


};