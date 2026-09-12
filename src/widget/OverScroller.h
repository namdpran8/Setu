/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#pragma once

#include <functional>
#include <vector>
#include <memory>

namespace setu {
namespace widget {

class OverScroller : public std::enable_shared_from_this<OverScroller> {
public:
    OverScroller();
    ~OverScroller();

    void fling(int startX, int startY, int velocityX, int velocityY,
               int minX, int maxX, int minY, int maxY);
    bool computeScrollOffset();
    
    int getCurrX() const { return m_currX; }
    int getCurrY() const { return m_currY; }
    bool isFinished() const { return m_finished; }
    void abortAnimation();

    static void doFrame(long long frameTimeNanos);

private:
    static std::vector<std::shared_ptr<OverScroller>>& getActiveScrollers();
    void processTick(long long frameTimeNanos);

    bool m_finished = true;
    int m_currX = 0;
    int m_currY = 0;
    
    int m_startX = 0;
    int m_startY = 0;
    int m_minX = 0;
    int m_maxX = 0;
    int m_minY = 0;
    int m_maxY = 0;
    
    float m_velocityX = 0;
    float m_velocityY = 0;
    
    long long m_startTime = 0;
    long long m_lastTime = 0;
    
    // Friction
    float m_friction = 0.015f; 
};

} // namespace widget
} // namespace setu
