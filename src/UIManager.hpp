#pragma once
#include "RenderPipeline.hpp"
#include "InputManager.hpp"
#include "Config.hpp"
#include "raylib.h"

class UIManager final {
public:
    UIManager() noexcept = default;
    ~UIManager() noexcept = default;

    inline void Update(float dt, const InputCommand& cmd) noexcept;
    inline void RenderDashboard(RenderPipeline& pipeline, uint32_t score, float paddleX) const noexcept;

private:
    // [데이터 지향 정렬] 자주 함께 쓰이는 변수들을 하나로 모아 캐시 라인 적재 최적화
    
    // Chapter 15. UI 버튼의 현재 활성화 상태
    bool m_isButtonActive{false};

    // Chapter 16. 하트비트 타이머
    float m_timeAccumulator{0.0f};
    bool m_heartbeatState{false};
};

// =========================================================================
// [인라인 구현부]
// =========================================================================
inline void UIManager::Update(float dt, const InputCommand& cmd) noexcept{
    /* 1. 마우스 UI 버튼 로직 (Bounds Check)
       교재 내용: "마우스 x 좌표가 버튼의 왼쪽과 오른쪽 사이에 있고, y 좌표가 위쪽과 아래쪽 사이에 있으면..."
       이 논리는 HFT의 Price Band(가격 상하한선) 체크와 동일한 분기 구조를 가집니다. */
    bool isMouseOverButton =
        (cmd.mouseX >= Config::UIButtonX) &
        (cmd.mouseX <= Config::UIButtonX + Config::UIButtonWidth) &
        (cmd.mouseY >= Config::UIButtonY) &
        (cmd.mouseY <= Config::UIButtonY + Config::UIButtonHeight);

    // 마우스가 버튼 위에 '있고(AND)', '단발성 클릭'이 발생했다면 버튼 액션 수행
    if (isMouseOverButton & cmd.leftClickPressed) {
        m_isButtonActive = !m_isButtonActive; // 버튼 상태 토글
    }

    // 2. 논블로킹 누적 타이머 (Heartbeat / Timer)
    m_timeAccumulator += dt; // 매 프레임의 시간을 누적

    // 누적된 시간이 우리가 설정한 간격(3초)을 넘었는지 확인
    if (m_timeAccumulator >= Config::HeartbeatInterval) {
        // [HFT 미세 팁] m_timeAccumulator = 0.0f; 로 초기화하지 않고 초과한 기준치(3.0)만 빼서 잔여 시간 이월 (Drift 예방)
        m_timeAccumulator -= Config::HeartbeatInterval;

        // 3초마다 수행할 로직 (UI 상태 토글)
        m_heartbeatState = !m_heartbeatState;
    }
}

inline void UIManager::RenderDashboard(RenderPipeline& pipeline, uint32_t score, float paddleX) const noexcept{
    // LAYER 1: UI Dashboard & Telemetry Panel (상단 70px)
    
    // 1-1. 대시보드 배경
    pipeline.PushRectangle(0.0f, 0.0f, Config::WindowWidth, Config::UIPanelHeight, Config::Theme::UIPanelBg);

    // 1-2. [Chapter 30 변경] 타이틀 대신 현재 점수(Score) 출력
    const char* scoreText = TextFormat("SCORE: %u", score);
    pipeline.PushText(Config::ScreenCenterX - 40.0f, 15.0f, scoreText, 20, Config::Theme::TextTitle);

    // 1-3. 텔레메트리 디버그 정보 (좌측)
    // 외부에서 필요한 최소한의 데이터만 매개변수로 전달받음
    const char* debugInfo = TextFormat("Paddle X: %.1f | Timer: %.2f", paddleX, m_timeAccumulator);
    pipeline.PushText(Config::UIMargin, Config::UIPanelHeight - 25.0f, debugInfo, 16, Config::Theme::TextNormal);

    // 1-4. UI 상단 버튼 (우측 중간)
    Color btnColor = m_isButtonActive ? Config::Theme::ButtonActive : Config::Theme::ButtonDefault;
    pipeline.PushRectangle(Config::UIButtonX, Config::UIButtonY, Config::UIButtonWidth, Config::UIButtonHeight, btnColor);

    const char* btnText = "CLICK";
    int btnFontSize = 16;
    int textWidth = MeasureText(btnText, btnFontSize);
    float textX = Config::UIButtonX + (Config::UIButtonWidth - textWidth) / 2.0f;
    float textY = Config::UIButtonY + (Config::UIButtonHeight - btnFontSize) / 2.0f;
    pipeline.PushText(textX, textY, btnText, btnFontSize, BLACK);

    // 1-5. Heartbeat 인디케이터 (우측 끝)
    Color heartbeatColor = m_heartbeatState ? Config::Theme::HeartbeatActive : Config::Theme::HeartbeatNormal;
    pipeline.PushCircle(Config::WindowWidth - 30.0f, Config::UIPanelHeight / 2.0f, 12.0f, heartbeatColor);
}