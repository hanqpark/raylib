#pragma once
#include "Window.hpp"
#include "Config.hpp"
#include "InputManager.hpp"
#include "Player.hpp"
#include "RenderPipeline.hpp"

class Engine final {
public:
    Engine() noexcept 
        : m_player{
            Config::WindowWidth / 2.0f, 
            Config::WindowHeight / 2.0f, 
            Config::PlayerSpeed, 
            Config::PlayerRadius, 
            Config::PlayerColor
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

        // [교재 내용 적용] 화면 밖으로 나가지 못하게 좌표(Config::WindowWidth 등)로 제한
        if (m_player.x < m_player.radius) m_player.x = m_player.radius;
        if (m_player.x > Config::WindowWidth - m_player.radius) m_player.x = Config::WindowWidth - m_player.radius;
        if (m_player.y < m_player.radius) m_player.y = m_player.radius;
        if (m_player.y > Config::WindowHeight - m_player.radius) m_player.y = Config::WindowHeight - m_player.radius;

        /* --- 2. 단발성 상태 갱신 (IsKeyPressed 기반) ---
        // 교재의 "한 번의 입력에 한 번만 반응" 원리 증명.
        // 스페이스바를 꾹 누르고 있어도 색상은 미친듯이 깜빡이지 않고 딱 한 번만 바뀝니다. */
        if (cmd.fireAction) {
            // 현재 색상이 원래 색상이면 파란색으로, 파란색이면 다시 원래 색상으로 변경
            if (m_player.color.r == Config::PlayerColor.r) {
                m_player.color = BLUE; 
            } else {
                m_player.color = Config::PlayerColor;
            }
        }

        /* --- 3. [추가] 마우스 UI 버튼 로직 (Bounds Check) ---
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
    }

    // m_renderPipeline의 상태를 변경하므로 const를 제거합니다.
    void Render(const InputCommand& cmd) noexcept {
        // --- [Chapter 13 실습: 도형의 기준점과 좌표 계산] ---

        // 실습 1. (0, 0) 원점 증명
        // 사각형은 '좌측 상단'이 기준점이므로, (0,0)에 그리면 화면 왼쪽 맨 위에 딱 붙어서 그려집니다.
        m_renderPipeline.PushRectangle(0.0f, 0.0f, Config::SampleRectWidth, Config::SampleRectHeight, BLUE);

        // 실습 2. 사각형을 정확히 화면 중앙에 배치하기 위한 오프셋(Offset) 계산
        // 사각형을 중앙에 두려면 '화면 중앙 좌표 - (내 크기의 절반)'을 해야 합니다.
        // [HFT 설계] 이 나눗셈과 뺄셈 연산 역시 constexpr을 이용해 컴파일 타임에 미리 끝내버립니다.
        constexpr float centeredRectX = Config::ScreenCenterX - (Config::SampleRectWidth / 2.0f);
        constexpr float centeredRectY = Config::ScreenCenterY - (Config::SampleRectHeight / 2.0f);
        
        m_renderPipeline.PushRectangle(centeredRectX, centeredRectY, Config::SampleRectWidth, Config::SampleRectHeight, DARKGRAY);

        // --- [Chapter 15 실습: 도형의 기준점과 좌표 계산] ---
        // 1. 원의 기준점 증명 (동적으로 움직이는 플레이어)
        // 원은 '중심점'이 기준이므로, 추가 연산 없이 바로 그리면 됩니다.
        m_renderPipeline.PushCircle(m_player.x, m_player.y, m_player.radius, m_player.color);

        // 2. UI 버튼 렌더링 (상태에 따라 색상 변경)
        Color btnColor = m_isButtonActive ? LIME : GRAY;
        m_renderPipeline.PushRectangle(Config::UIButtonX, Config::UIButtonY, Config::UIButtonWidth, Config::UIButtonHeight, btnColor);

        // 3. 교재 실습: "원을 마우스 위치에 그리면 마우스를 따라다니는 것처럼 보입니다"
        // 마우스 커서 역할을 할 작은 빨간색 원 (클릭 상태일 때 투명도 변경)
        Color cursorColor = cmd.leftClickDown ? RED : MAROON;
        m_renderPipeline.PushCircle(cmd.mouseX, cmd.mouseY, 5.0f, cursorColor);

        /** 렌더링 파이프라인 일괄 처리(Flush) **/
        m_window.BeginRender();    // ClearBackground 포함
        m_renderPipeline.Flush();  // HFT 스타일 순차 메모리 렌더링
        DrawFPS(10, 10);           // 로우레이턴시 진단용 FPS 카운터
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
};