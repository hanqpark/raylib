#pragma once
#include <cmath>
#include "Window.hpp"
#include "Config.hpp"
#include "InputManager.hpp"
#include "Player.hpp"
#include "Ball.hpp"
#include "RenderPipeline.hpp"

class Engine final {
public:
    Engine() noexcept 
// [Chapter 28 적용] 플레이어 패들을 화면 하단 중앙으로 세팅
        : m_player{
            Config::ScreenCenterX - (Config::PaddleWidth / 2.0f),
            Config::PaddleY,
            Config::PaddleWidth,
            Config::PaddleHeight,
            Config::PlayerSpeed, 
            Config::Theme::PlayerNormal
        },
        // [Chapter 25 적용] 공의 시작 위치를 Play Area 내부 중앙으로 맞추고 테마 색상 적용
        m_ball{
            Config::BallInitialX,
            Config::BallInitialY,
            Config::BallSpeedX,
            Config::BallSpeedY,
            Config::BallRadius,
            Config::Theme::Ball
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
        // --- 1. [Chapter 28 변경] 패들 1D(좌우) 위치 갱신 ---
        // 상하 이동(cmd.dy)은 완전히 무시하고 좌우 이동(cmd.dx)만 반영합니다.
        m_player.x += cmd.dx * m_player.speed * dt;
        // m_player.y += cmd.dy * m_player.speed * dt;

        // [패들 경계 보정 (Clamping)]
        // 패들 좌측 끝(x)이 화면 왼쪽(0.0f) 밖으로 나가지 않도록 보정
        if (m_player.x < 0.0f) {
            m_player.x = 0.0f;
        }
        // 패들 우측 끝(x + width)이 화면 오른쪽(WindowWidth)을 넘지 않도록 보정
        else if (m_player.x + m_player.width > Config::WindowWidth) {
            m_player.x = Config::WindowWidth - m_player.width;
        }

        // --- 2. [Chapter 25 & 27] 공 위치 갱신 & 벽 충돌 반사 ---
        // 분기(Branch) 없는 캐시 친화적 시간 기반 2D 위치 연산
        m_ball.x += m_ball.vx * dt;
        m_ball.y += m_ball.vy * dt;

        // [좌측 벽 충돌]
        if (m_ball.x - m_ball.radius <= 0.0f) {
            m_ball.x = m_ball.radius;              // 1. 위치 보정 (Clamping)
            m_ball.vx = std::abs(m_ball.vx);       // 2. 오른쪽 방향(+) 속도 강제 지정
        }
        // [우측 벽 충돌]
        else if (m_ball.x + m_ball.radius >= Config::WindowWidth) {
            m_ball.x = Config::WindowWidth - m_ball.radius; // 1. 위치 보정 (Clamping)
            m_ball.vx = -std::abs(m_ball.vx);              // 2. 왼쪽 방향(-) 속도 강제 지정
        }

        // [상단 벽 충돌] (UI 패널 하단 boundary인 Config::PlayAreaY 기준)
        if (m_ball.y - m_ball.radius <= Config::PlayAreaY) {
            m_ball.y = Config::PlayAreaY + m_ball.radius;   // 1. 위치 보정 (Clamping)
            m_ball.vy = std::abs(m_ball.vy);               // 2. 아래쪽 방향(+) 속도 강제 지정
        }
        // [하단 벽 충돌]
        else if (m_ball.y + m_ball.radius >= Config::WindowHeight) {
            m_ball.y = Config::WindowHeight - m_ball.radius; // 1. 위치 보정 (Clamping)
            m_ball.vy = -std::abs(m_ball.vy);               // 2. 위쪽 방향(-) 속도 강제 지정
        }

        /* --- 3. 단발성 상태 갱신 (IsKeyPressed 기반) ---
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

        // --- 4. [Chapter 29 추가] 공 - 패들 충돌 처리 ---
        // [HFT Optimization]: 공이 아래로 이동 중(vy > 0)일 때만 충돌 검사를 수행합니다.
        // 불필요한 연산을 조기 차단(Early Exit)하고 다중 프레임 연쇄 충돌 버그를 완벽히 예방합니다.
        if (m_ball.vy > 0.0f) {
            Vector2 ballCenter{ m_ball.x, m_ball.y };
            Rectangle paddleRec{ m_player.x, m_player.y, m_player.width, m_player.height };

            // raylib의 사각형-원 충돌 판정 (스택 메모리 상에서 직접 처리)
            if (CheckCollisionCircleRec(ballCenter, m_ball.radius, paddleRec)) {
                // 1. Penetration Resolution: 공을 패들 상단 표면으로 즉시 보정
                m_ball.y = m_player.y - m_ball.radius;

                // 2. Y축 속도 반전: 무조건 위쪽 방향(-)으로 지정
                m_ball.vy = -std::abs(m_ball.vy);

                // 3. 패들 타격 위치 기반 X축 속도 변주 (Linear Interpolation)
                // 패들 중심점 기준 상대 위치 계산: [-1.0 (좌측 끝), 0.0 (중앙), 1.0 (우측 끝)]
                float paddleCenterX = m_player.x + (m_player.width * 0.5f);
                float hitOffset = (m_ball.x - paddleCenterX) / (m_player.width * 0.5f);

                // 오프셋 범위 안전 보정 (Clamping: -1.0 ~ 1.0)
                if (hitOffset < -1.0f) hitOffset = -1.0f;
                if (hitOffset > 1.0f) hitOffset = 1.0f;

                // 타격 지점에 비례하여 X축 속도를 변동시킴 (중앙: 직수직, 외곽: 빗겨 튕김)
                m_ball.vx = hitOffset * Config::BallSpeedX * 1.5f;
            }
        }

        /* --- 5. 마우스 UI 버튼 로직 (Bounds Check) ---
        // 교재 내용: "마우스 x 좌표가 버튼의 왼쪽과 오른쪽 사이에 있고, y 좌표가 위쪽과 아래쪽 사이에 있으면..."
        // 이 논리는 HFT의 Price Band(가격 상하한선) 체크와 완전히 동일한 분기 구조를 가집니다. */
        bool isMouseOverButton =
            (cmd.mouseX >= Config::UIButtonX) & 
            (cmd.mouseX <= Config::UIButtonX + Config::UIButtonWidth) &
            (cmd.mouseY >= Config::UIButtonY) & 
            (cmd.mouseY <= Config::UIButtonY + Config::UIButtonHeight);

        // 마우스가 버튼 위에 '있고(AND)', '단발성 클릭'이 발생했다면 버튼 액션 수행
        if (isMouseOverButton & cmd.leftClickPressed) {
            m_isButtonActive = !m_isButtonActive; // 버튼 상태 토글
        }

        // --- 6. 논블로킹 누적 타이머 (Heartbeat / Timer) ---
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
// =================================================================
        // LAYER 1: UI Dashboard & Telemetry Panel (상단 70px)
        // =================================================================
        // 1-1. 대시보드 배경
        m_renderPipeline.PushRectangle(0.0f, 0.0f, Config::WindowWidth, Config::UIPanelHeight, Config::Theme::UIPanelBg);

        // 1-2. 대시보드 타이틀
        const char* titleText = "System Status Dashboard";
        int titleWidth = MeasureText(titleText, 18);
        m_renderPipeline.PushText(Config::ScreenCenterX - (titleWidth / 2.0f), 8.0f, titleText, 18, Config::Theme::TextTitle);

        // 1-3. 텔레메트리 디버그 정보 (좌측)
        const char* debugInfo = TextFormat("Paddle X: %.1f | Timer: %.2f", m_player.x, m_timeAccumulator);
        m_renderPipeline.PushText(Config::UIMargin, Config::UIPanelHeight - 25.0f, debugInfo, 16, Config::Theme::TextNormal);

        // 1-4. UI 상단 버튼 (우측 중간)
        Color btnColor = m_isButtonActive ? Config::Theme::ButtonActive : Config::Theme::ButtonDefault;
        m_renderPipeline.PushRectangle(Config::UIButtonX, Config::UIButtonY, Config::UIButtonWidth, Config::UIButtonHeight, btnColor);

        const char* btnText = "CLICK";
        int btnFontSize = 16;
        int textWidth = MeasureText(btnText, btnFontSize);
        float textX = Config::UIButtonX + (Config::UIButtonWidth - textWidth) / 2.0f;
        float textY = Config::UIButtonY + (Config::UIButtonHeight - btnFontSize) / 2.0f;
        m_renderPipeline.PushText(textX, textY, btnText, btnFontSize, BLACK);

        // 1-5. Heartbeat 인디케이터 (우측 끝)
        Color heartbeatColor = m_heartbeatState ? Config::Theme::HeartbeatActive : Config::Theme::HeartbeatNormal;
        m_renderPipeline.PushCircle(Config::WindowWidth - 30.0f, Config::UIPanelHeight / 2.0f, 12.0f, heartbeatColor);


        // =================================================================
        // LAYER 2: Pure Game World (Play Area)
        // =================================================================
        // [Chapter 28 변경] 사각형 패들 렌더링
        m_renderPipeline.PushRectangle(m_player.x, m_player.y, m_player.width, m_player.height, m_player.color);

        // [Chapter 25 추가] 공(Ball) 렌더링 (커스텀 RenderPipeline 버퍼에 푸시)
        m_renderPipeline.PushCircle(m_ball.x, m_ball.y, m_ball.radius, m_ball.color);


        // =================================================================
        // LAYER 3: Overlay (Mouse Cursor)
        // =================================================================
        // 마우스 커서는 화면 내 모든 요소보다 위에 있어야 하므로 가장 마지막에 렌더링 큐에 넣습니다.
        Color cursorColor = cmd.leftClickDown ? Config::Theme::HeartbeatActive : Config::Theme::PlayerNormal;
        m_renderPipeline.PushCircle(cmd.mouseX, cmd.mouseY, 5.0f, cursorColor);


        // =================================================================
        // Flush Render Commands
        // =================================================================
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
    BallState m_ball;

    // 화면 렌더링 큐를 관리하는 객체
    RenderPipeline m_renderPipeline;

    // Chapter 15.UI 버튼의 현재 활성화 상태
    bool m_isButtonActive{false};

    // Chapter 16. 하트비트 타이머
    float m_timeAccumulator{0.0f};
    bool m_heartbeatState{false};
};