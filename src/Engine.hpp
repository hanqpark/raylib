#pragma once
#include "Window.hpp"
#include "Config.hpp"
#include "InputManager.hpp"
#include "Player.hpp"

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

    void Render() const noexcept {
        m_window.BeginRender();
        
        // PlayerState의 데이터를 렌더링 API에 바인딩
        DrawCircle(static_cast<int>(m_player.x), static_cast<int>(m_player.y), m_player.radius, m_player.color);

        // 로우레이턴시 진단용 FPS 카운터
        DrawFPS(10, 10);

        m_window.EndRender();
    }

private:
    // RAII에 의해 가장 먼저 초기화되고 가장 나중에 파괴됨
    Window m_window; 

    // 상태 데이터를 담는 POD 구조체 인스턴스
    PlayerState m_player;
    
};