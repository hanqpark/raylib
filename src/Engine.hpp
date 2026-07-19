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
            Render();
        }
    }

private:
    void Update(float dt, const InputCommand& cmd) noexcept {
        m_player.x += cmd.dx * m_player.speed * dt;
        m_player.y += cmd.dy * m_player.speed * dt;

        // [교재 내용 적용] 화면 밖으로 나가지 못하게 좌표(Config::WindowWidth 등)로 제한
        if (m_player.x < m_player.radius) m_player.x = m_player.radius;
        if (m_player.x > Config::WindowWidth - m_player.radius) m_player.x = Config::WindowWidth - m_player.radius;
        if (m_player.y < m_player.radius) m_player.y = m_player.radius;
        if (m_player.y > Config::WindowHeight - m_player.radius) m_player.y = Config::WindowHeight - m_player.radius;
    }

    // m_renderPipeline의 상태를 변경하므로 const를 제거합니다.
    void Render() noexcept {
        // --- [교재 실습: 도형의 기준점과 좌표 계산] ---

        // 실습 1. (0, 0) 원점 증명
        // 사각형은 '좌측 상단'이 기준점이므로, (0,0)에 그리면 화면 왼쪽 맨 위에 딱 붙어서 그려집니다.
        m_renderPipeline.PushRectangle(0.0f, 0.0f, Config::SampleRectWidth, Config::SampleRectHeight, BLUE);

        // 실습 2. 사각형을 정확히 화면 중앙에 배치하기 위한 오프셋(Offset) 계산
        // 사각형을 중앙에 두려면 '화면 중앙 좌표 - (내 크기의 절반)'을 해야 합니다.
        // [HFT 설계] 이 나눗셈과 뺄셈 연산 역시 constexpr을 이용해 컴파일 타임에 미리 끝내버립니다.
        constexpr float centeredRectX = Config::ScreenCenterX - (Config::SampleRectWidth / 2.0f);
        constexpr float centeredRectY = Config::ScreenCenterY - (Config::SampleRectHeight / 2.0f);
        
        m_renderPipeline.PushRectangle(centeredRectX, centeredRectY, Config::SampleRectWidth, Config::SampleRectHeight, DARKGRAY);

        // 실습 3. 원의 기준점 증명 (동적으로 움직이는 플레이어)
        // 원은 '중심점'이 기준이므로, 추가 연산 없이 바로 그리면 됩니다.
        m_renderPipeline.PushCircle(m_player.x, m_player.y, m_player.radius, m_player.color);

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
    
    // [HFT 추가] 화면 렌더링 큐를 관리하는 객체
    RenderPipeline m_renderPipeline;
};