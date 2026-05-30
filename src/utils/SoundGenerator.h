#pragma once

/// 程序化音效生成器：用波形合成音效，不依赖外部文件
class SoundGenerator {
public:
    /// 初始化音频设备并生成所有音效
    static void GenerateAll();

    /// 生成射击音效 (短促高频哔声)
    static void GenerateShoot();

    /// 生成爆炸音效 (低频噪声)
    static void GenerateExplosion();

    /// 生成拾取道具音效 (上升音阶)
    static void GeneratePickup();

    /// 生成游戏结束音效
    static void GenerateGameOver();
};
