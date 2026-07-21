#pragma once
#include "Window.hpp"
#include "Config.hpp"
#include "InputManager.hpp"
#include "Player.hpp"
#include "RenderPipeline.hpp"

class Engine final {
public:
    Engine() noexcept 
        // [Chapter 18 적용] 플레이어의 시작 위치를 Play Area 내부 중앙으로 맞추고 테마 색상 적용
        : m_player{
            Config::ScreenCenterX,
            Config::PlayAreaY + (Config::PlayAreaHeight / 2.0f),
            Config::PlayerSpeed, 
            Config::PlayerRadius, 
            Config::Theme::PlayerNormal
        } {}

    ~Engine() noexcept = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void Run() noexcept {
        while (!m_window.ShouldClose()) {
            // HFT의 Tick 간격 계산과 동일한 물리 시간 델타값 확보
            float dt = GetFrameTime(); 
            
            // [교재의 핵심 3단계 파이프라인]
            // 1. 입력 (Input Polling)
            InputCommand cmd = InputManager::Poll();
            
            // 2. 갱신 (State Update)
            Update(dt, cmd);
            
            // 3. 렌더링 (Render View)
            Render(cmd);
        }
    }

private:
    void Update(float dt, const InputCommand& cmd) noexcept {
        // --- 1. 연속성 상태 갱신 (IsKeyDown 기반) ---
        m_player.x += cmd.dx * m_player.speed * dt;
        m_player.y += cmd.dy * m_player.speed * dt;

        // [Chapter 18 적용] 레이아웃 분리에 따른 Bounds Check
        // 플레이어가 UI 패널(PlayAreaY) 위로 올라가지 못하도록 상단 한계선을 조정합니다.
        if (m_player.x < m_player.radius) m_player.x = m_player.radius;
        if (m_player.x > Config::WindowWidth - m_player.radius) m_player.x = Config::WindowWidth - m_player.radius;
        if (m_player.y < Config::PlayAreaY + m_player.radius) m_player.y = Config::PlayAreaY + m_player.radius;
        if (m_player.y > Config::WindowHeight - m_player.radius) m_player.y = Config::WindowHeight - m_player.radius;

        /* --- 2. 단발성 상태 갱신 (IsKeyPressed 기반) ---
        // 교재의 "한 번의 입력에 한 번만 반응" 원리 증명.
        // 스페이스바를 꾹 누르고 있어도 색상은 미친듯이 깜빡이지 않고 딱 한 번만 바뀝니다. 
        // [Chapter 18 적용] 하드코딩된 색상 대신 시맨틱 컬러 사용 */
        if (cmd.fireAction) {
            // 현재 색상이 원래 색상이면 파란색으로, 파란색이면 다시 원래 색상으로 변경
            if (m_player.color.r == Config::Theme::PlayerNormal.r) {
                m_player.color = Config::Theme::PlayerAction;
            } else {
                m_player.color = Config::Theme::PlayerNormal;
            }
        }

        /* --- 3. 마우스 UI 버튼 로직 (Bounds Check) ---
        // 교재 내용: "마우스 x 좌표가 버튼의 왼쪽과 오른쪽 사이에 있고, y 좌표가 위쪽과 아래쪽 사이에 있으면..."
        // 이 논리는 HFT의 Price Band(가격 상하한선) 체크와 완전히 동일한 분기 구조를 가집니다. */
        bool isMouseOverButton =
            (cmd.mouseX >= Config::UIButtonX) & 
            (cmd.mouseX <= Config::UIButtonX + Config::UIButtonWidth) &
            (cmd.mouseY >= Config::UIButtonY) & 
            (cmd.mouseY <= Config::UIButtonY + Config::UIButtonHeight);

        // 마우스가 버튼 위에 '있고(AND)', '단발성 클릭'이 발생했다면 버튼 액션 수행
        if (isMouseOverButton && cmd.leftClickPressed) {
            m_isButtonActive = !m_isButtonActive; // 버튼 상태 토글
        }

        // --- 4. [추가] 논블로킹 누적 타이머 (Heartbeat / Timer) ---
        m_timeAccumulator += dt; // 매 프레임의 시간을 누적

        // 누적된 시간이 우리가 설정한 간격(3초)을 넘었는지 확인
        if (m_timeAccumulator >= Config::HeartbeatInterval) {
            // [HFT 미세 팁] m_timeAccumulator = 0.0f; 로 초기화하지 않습니다!
            // 프레임 시간이 정확히 3.0이 아니라 3.016초일 수 있습니다.
            // 0으로 덮어쓰면 0.016초의 오차가 매번 유실(Drift)되어 나중에는 스텝이 완전히 꼬입니다.
            // 초과한 기준치(3.0)만 빼주어 잔여 시간을 다음 주기로 이월시켜야 정확도가 유지됩니다.
            m_timeAccumulator -= Config::HeartbeatInterval;

            // 3초마다 수행할 로직 (UI 상태 토글)
            m_heartbeatState = !m_heartbeatState;
        }
    }

    // m_renderPipeline의 상태를 변경하므로 const를 제거합니다.
    void Render(const InputCommand& cmd) noexcept {
        // --- 1. 배경 및 UI 패널 레이아웃 렌더링 ---
        // 이전 챕터의 (0,0) 원점 증명용 사각형들은 UI 영역과 겹치므로 제거했습니다.
        // 상단 70 픽셀을 정보 표시를 위한 'UI 패널' 영역으로 할당하고 배경색을 다르게 칠합니다.
        m_renderPipeline.PushRectangle(0.0f, 0.0f, Config::WindowWidth, Config::UIPanelHeight, Config::Theme::UIPanelBg);

        // --- 2. UI 영역 (UI Panel) 정보 렌더링 ---
        // 대시보드 타이틀 (상단 중앙 정렬)
        const char* titleText = "System Status Dashboard";
        int titleWidth = MeasureText(titleText, 20);
        m_renderPipeline.PushText(Config::ScreenCenterX - (titleWidth / 2.0f), Config::UIMargin, titleText, 20, Config::Theme::TextTitle);

        // 동적 디버깅 텍스트 (UI 패널 좌측 하단 여백 배치, Zero-Allocation)
        const char* debugInfo = TextFormat("Pos: (%.1f, %.1f) | Timer: %.2f", m_player.x, m_player.y, m_timeAccumulator);
        m_renderPipeline.PushText(Config::UIMargin, Config::UIPanelHeight - 25.0f, debugInfo, 20, Config::Theme::TextNormal);

        // 하트비트 인디케이터 (UI 패널 우측 정렬)
        // 매직 넘버(RED, DARKGRAY) 대신 Config::Theme의 시맨틱 컬러를 사용합니다.
        Color heartbeatColor = m_heartbeatState ? Config::Theme::HeartbeatActive : Config::Theme::HeartbeatNormal;
        m_renderPipeline.PushCircle(Config::WindowWidth - 30.0f, Config::UIPanelHeight / 2.0f, 15.0f, heartbeatColor);


        // --- 3. 플레이 영역 (Play Area) 오브젝트 렌더링 ---
        // 플레이어 (Update()에서 테마 색상이 이미 결정되어 m_player.color에 반영됨)
        m_renderPipeline.PushCircle(m_player.x, m_player.y, m_player.radius, m_player.color);

        // 상호작용 UI 버튼 (플레이 영역 내 하단 배치)
        Color btnColor = m_isButtonActive ? Config::Theme::ButtonActive : Config::Theme::ButtonDefault;
        m_renderPipeline.PushRectangle(Config::UIButtonX, Config::UIButtonY, Config::UIButtonWidth, Config::UIButtonHeight, btnColor);

        // UI 버튼 텍스트 (버튼 크기에 맞춘 중앙 정렬)
        const char* btnText = "CLICK";
        int btnFontSize = 20;
        int textWidth = MeasureText(btnText, btnFontSize);
        float textX = Config::UIButtonX + (Config::UIButtonWidth - textWidth) / 2.0f;
        float textY = Config::UIButtonY + (Config::UIButtonHeight - btnFontSize) / 2.0f;
        m_renderPipeline.PushText(textX, textY, btnText, btnFontSize, BLACK);


        // --- 4. 오버레이 렌더링 (마우스 커서) ---
        // 마우스 커서는 화면 내 모든 요소보다 위에 있어야 하므로 가장 마지막에 렌더링 큐에 넣습니다.
        Color cursorColor = cmd.leftClickDown ? Config::Theme::HeartbeatActive : Config::Theme::PlayerNormal;
        m_renderPipeline.PushCircle(cmd.mouseX, cmd.mouseY, 5.0f, cursorColor);


        /** --- 5. 렌더링 파이프라인 일괄 처리(Flush) --- **/
        m_window.BeginRender();    // HFT 권장: Window 내부에 ClearBackground(Config::Theme::Background) 적용
        m_renderPipeline.Flush();  // 메모리 풀에 순차적으로 담긴 명령을 한 번에 CPU 캐시 프렌들리하게 렌더링
        
        // FPS 카운터는 UI 패널의 좌측 상단 여백에 위치시킵니다.
        DrawFPS(static_cast<int>(Config::UIMargin), static_cast<int>(Config::UIMargin));
        m_window.EndRender();
    }

private:
    // RAII에 의해 가장 먼저 초기화되고 가장 나중에 파괴됨
    Window m_window; 

    // 상태 데이터를 담는 POD 구조체 인스턴스
    PlayerState m_player;
    
    // 화면 렌더링 큐를 관리하는 객체
    RenderPipeline m_renderPipeline;

    // Chapter 15.UI 버튼의 현재 활성화 상태
    bool m_isButtonActive{false};

    // Chapter 16. 하트비트 타이머
    float m_timeAccumulator{0.0f};
    bool m_heartbeatState{false};
};