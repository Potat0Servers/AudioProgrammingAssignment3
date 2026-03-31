
#pragma once
#include <JuceHeader.h>

class Arpeggiator
{
public:
    Arpeggiator() {}
    ~Arpeggiator() {}


    /**
     * @brief Updates the playback speed based on a target frequency.
     * 
     * @param newFreq The target frequency in Hz.
     */
    void setSpeedInSamples(float newFreq)
    {
        speedInSamples = round(sampleRate / newFreq);
	}



    // 核心处理函数，已经在你的 processBlock 中被调用
    void processMidi(juce::MidiBuffer& midiMessages, int numSamples)
    {
        // =====================================================================
        // 第一步：遍历 MIDI 消息，更新 currentNotes 集合
        // =====================================================================

        // 1. 遍历当前传入的 MidiBuffer 中的所有消息
        for (const auto metadata : midiMessages)
        {
            // 提取出实际的 MIDI 消息对象
            auto message = metadata.getMessage();

            // 2. 判断是不是“按下琴键”
            if (message.isNoteOn())
            {
                // 获取 MIDI 音符编号（0-127）并加入到我们的有序集合中
                currentNotes.add(message.getNoteNumber());
            }
            // 3. 判断是不是“松开琴键”
            else if (message.isNoteOff())
            {
                // 将该音符编号从集合中剔除
                currentNotes.removeValue(message.getNoteNumber());
            }
        }

        // 4. 【极度重要】清空原本的 MIDI 缓冲！
        // 如果不加这一句，宿主传来的和弦依然会直接漏给下方的合成器，
        // 你的琶音器就变成了“和弦 + 琶音”大乱炖。
        midiMessages.clear();


        // =====================================================================
        // 第二步：判断是否需要切换音符 (待完成...)
        // =====================================================================


        // =====================================================================
        // 第三步：根据 currentNoteIndex 来决定谁开谁关 (待完成...)
        // =====================================================================
    }

private:
    // 记录当前按下的音符
    juce::SortedSet<int> currentNotes;

    // 内部状态追踪
	float sampleRate = 44100.f; // 默认采样率，实际使用时应该从 prepareToPlay 获取
    int currentNoteIndex = 0;
    int timeElapsed = 0;
    int speedInSamples = 15000; // 切换速度，数值越小琶音越快，4000大约是十分之一秒（假设采样率44100）
    int lastNotePlayed = -1;   // 记录上一个发出的音，方便精准发送 Note Off
};