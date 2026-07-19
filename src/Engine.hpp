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

        if (m_player.x < m_player.radius) m_player.x = m_player.radius;
        if (m_player.x > Config::WindowWidth - m_player.radius) m_player.x = Config::WindowWidth - m_player.radius;
        if (m_player.y < m_player.radius) m_player.y = m_player.radius;
        if (m_player.y > Config::WindowHeight - m_player.radius) m_player.y = Config::WindowHeight - m_player.radius;
    }

    // m_renderPipeline의 상태를 변경하므로 const를 제거합니다.
    void Render() noexcept {
        // 1. Queueing: 비즈니스 데이터를 렌더링 커맨드로 변환하여 적재

        // 이동하는 플레이어(원)
        m_renderPipeline.PushCircle(m_player.x, m_player.y, m_player.radius, m_player.color);

        // 이전 실습의 정적 도형 테스트 데이터 (적과 총알 궤적 가상)
        m_renderPipeline.PushRectangle(50.0f, 50.0f, 40.0f, 40.0f, RED);
        m_renderPipeline.PushLine(400.0f, 300.0f, 750.0f, 100.0f, BLACK);

        // 2. 렌더링 실행
        
        // ClearBackground 포함
        m_window.BeginRender();
        
        // HFT 스타일 순차 메모리 렌더링
        m_renderPipeline.Flush();

        // 로우레이턴시 진단용 FPS 카운터
        DrawFPS(10, 10);
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