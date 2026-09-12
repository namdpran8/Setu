/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#include "OverScroller.h"
#include "../utils/SystemClock.h"
#include "../view/View.h"
#include <cmath>
#include <algorithm>

namespace setu {
namespace widget {

OverScroller::OverScroller() {}
OverScroller::~OverScroller() { abortAnimation(); }

std::vector<std::shared_ptr<OverScroller>>& OverScroller::getActiveScrollers() {
    static std::vector<std::shared_ptr<OverScroller>> s_scrollers;
    return s_scrollers;
}

void OverScroller::doFrame(long long frameTimeNanos) {
    auto& scrollers = getActiveScrollers();
    if (scrollers.empty()) return;
    
    std::vector<std::shared_ptr<OverScroller>> current = scrollers;
    for (auto& s : current) {
        if (!s->m_finished) {
            s->processTick(frameTimeNanos);
        }
    }
}

void OverScroller::fling(int startX, int startY, int velocityX, int velocityY,
                         int minX, int maxX, int minY, int maxY) {
    m_startX = startX;
    m_startY = startY;
    m_currX = startX;
    m_currY = startY;
    
    m_velocityX = (float)velocityX;
    m_velocityY = (float)velocityY;
    
    m_minX = minX;
    m_maxX = maxX;
    m_minY = minY;
    m_maxY = maxY;
    
    m_finished = false;
    m_startTime = setu::uptimeMillis();
    m_lastTime = m_startTime;
    
    // Register
    auto& scrollers = getActiveScrollers();
    if (std::find(scrollers.begin(), scrollers.end(), shared_from_this()) == scrollers.end()) {
        scrollers.push_back(shared_from_this());
    }
    
    // Request a frame tick
    setu::view::View::requestHostRedraw();
}

bool OverScroller::computeScrollOffset() {
    return !m_finished;
}

void OverScroller::processTick(long long frameTimeNanos) {
    if (m_finished) return;
    
    long long now = frameTimeNanos / 1000000LL;
    float dt = (now - m_lastTime) / 1000.0f; // seconds
    if (dt <= 0) return;
    
    m_lastTime = now;
    
    // Simple exponential decay friction
    m_velocityX *= std::exp(-m_friction * dt * 1000.0f);
    m_velocityY *= std::exp(-m_friction * dt * 1000.0f);
    
    m_currX += (int)(m_velocityX * dt);
    m_currY += (int)(m_velocityY * dt);
    
    m_currX = std::clamp(m_currX, m_minX, m_maxX);
    m_currY = std::clamp(m_currY, m_minY, m_maxY);
    
    if (std::abs(m_velocityX) < 10.0f && std::abs(m_velocityY) < 10.0f) {
        m_finished = true;
    }
    
    if (m_currX <= m_minX && m_velocityX < 0) { m_currX = m_minX; m_velocityX = 0; }
    if (m_currX >= m_maxX && m_velocityX > 0) { m_currX = m_maxX; m_velocityX = 0; }
    if (m_currY <= m_minY && m_velocityY < 0) { m_currY = m_minY; m_velocityY = 0; }
    if (m_currY >= m_maxY && m_velocityY > 0) { m_currY = m_maxY; m_velocityY = 0; }
    
    if (m_velocityX == 0 && m_velocityY == 0) {
        m_finished = true;
    }
    
    if (m_finished) {
        auto& scrollers = getActiveScrollers();
        scrollers.erase(std::remove(scrollers.begin(), scrollers.end(), shared_from_this()), scrollers.end());
    } else {
        setu::view::View::requestHostRedraw();
    }
}

void OverScroller::abortAnimation() {
    if (!m_finished) {
        m_finished = true;
        auto& scrollers = getActiveScrollers();
        scrollers.erase(std::remove(scrollers.begin(), scrollers.end(), shared_from_this()), scrollers.end());
    }
}

} // namespace widget
} // namespace setu
