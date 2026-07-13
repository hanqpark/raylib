#pragma once
#include "Window.hpp"

class Engine final {
public:
    Engine() noexcept 
        : m_textX(Config::WindowWidth / 4)
        , m_textY(Config::WindowHeight / 2) {}

    ~Engine() noexcept = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void Run() noexcept {
        while (!m_window.ShouldClose()) {
            // HFT의 Tick 간격 계산과 동일한 물리 시간 델타값 확보
            float dt = GetFrameTime(); 
            
            Update(dt);
            Render();
        }
    }

private:
    void Update(float dt) noexcept {
        // 위키독스 가이드 연습: 매 프레임 글자를 우측으로 미세하게 이동 (픽셀/초 단위)
        // 로우레이턴시 환경에서는 분기문(if)을 최소화하는 것이 좋으나, 화면 이탈 방지를 위해 제어
        m_textX += static_cast<int>(50.0f * dt); 
        if (m_textX > Config::WindowWidth) {
            m_textX = -200; // 화면 왼쪽 밖으로 나가면 재진입
        }
    }

    void Render() const noexcept {
        m_window.BeginRender();
        
        // 데이터(위치, 텍스트)를 렌더링 API에 바인딩
        DrawText(Config::TargetText.data(), m_textX, m_textY, Config::FontSize, Config::TextColor);
        
        // 로우레이턴시 진단용 FPS 카운터
        DrawFPS(10, 10);

        m_window.EndRender();
    }

private:
    Window m_window; // RAII에 의해 가장 먼저 초기화되고 가장 나중에 파괴됨
    
    // Primitive Data 레이아웃 (CPU 캐시 적중률을 위해 연속된 메모리에 배치)
    int m_textX;
    int m_textY;
};