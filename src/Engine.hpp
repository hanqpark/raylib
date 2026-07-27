#pragma once
#include <cmath>
#include <cstdint>
#include <array>
#include <utility>
#include "Window.hpp"
#include "Config.hpp"
#include "InputManager.hpp"
#include "Player.hpp"
#include "Ball.hpp"
#include "Brick.hpp"
#include "RenderPipeline.hpp"
#include "ResourceManager.hpp" // [추가] 리소스 관리 매니저
#include "UIManager.hpp"       // [추가] UI 및 타이머 로직 관리 매니저

// [HFT Optimization]: 1바이트 크기의 엄격한 타입(Enum Class)으로 FSM(유한 상태 기계) 구성
enum class GameState : uint8_t {
    Ready = 0,  // [추가] 시작 대기 상태 (공이 패들을 따라다님)
    Playing,    // 게임 진행 상태
    GameOver,   // 실패 상태
    GameClear   // [Chapter 34 추가] 승리 상태
};

class Engine final {
public:
    Engine() noexcept;
    ~Engine() noexcept = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void Run() noexcept;

private:
    // --- 1. 상태 통제 파이프라인 ---
    void ResetGame() noexcept;
    void Update(float dt, const InputCommand& cmd) noexcept;
    void Render(const InputCommand& cmd) noexcept;

    // --- 2. Update 세부 시스템 (Hot Path) ---
    inline void UpdatePaddle(float dt, const InputCommand& cmd) noexcept;
    inline void UpdateBall(float dt, const InputCommand& cmd) noexcept;
    inline void UpdateAnimations(float dt) noexcept;
    inline void CheckPaddleCollision() noexcept;
    inline void CheckBrickCollisions() noexcept;
    inline void UpdateUIAndTimers(float dt, const InputCommand& cmd) noexcept;

    // --- 3. Render 세부 시스템 (Cold Path) ---
    inline void RenderUIDashboard() noexcept;
    inline void RenderGameWorld() noexcept;
    inline void RenderOverlay(const InputCommand& cmd) noexcept;

    // RAII에 의해 가장 먼저 초기화되고 가장 나중에 파괴됨
    Window m_window; 
    AudioDeviceRAII m_audioDevice; 

    // 상태 데이터를 담는 POD 구조체 인스턴스
    PlayerState m_player;
    BallState m_ball;

    // 화면 렌더링 큐를 관리하는 객체
    RenderPipeline m_renderPipeline;

    // Chapter 30. 상태 및 점수 변수
    GameState m_gameState;
    uint32_t m_score;
    uint32_t m_activeBrickCount; // [Chapter 34 추가] 남은 벽돌 수

    // Chapter 32. 벽돌(Brick) 상태 배열
    // [HFT Optimization] 2차원 포인터 배열 대신, 1차원 std::array 사용
    // 모든 벽돌 데이터가 메모리 상에 파편화 없이 한 줄로 배치되어 L1/L2 캐시 히트율을 극대화합니다.
    std::array<BrickState, Config::TotalBricks> m_bricks;

    // [Chapter 41 추가] 스프라이트 애니메이션 상태 관리 변수
    // 힙 할당을 배제하고 인접한 메모리 공간에 연속으로 배치하여 캐시 히트율 보장
    float m_ballAnimTimer{0.0f};                              
    uint8_t m_ballFrameIndex{0};

    // [새로운 DOD 기반 분리 매니저 적용]
    ResourceManager m_resourceMgr;
    UIManager m_uiMgr;
};


inline Engine::Engine() noexcept {
    // [Chapter 39 추가] 게임 루프 진입 전 I/O를 100% 끝내서(Pre-load) 런타임 지연시간(Jitter) 차단
    // 외부 ResourceManager 위임을 통해 하드코딩된 리소스 코드가 모두 사라짐
    m_resourceMgr.PreloadResources();

    ResetGame();
}

inline void Engine::Run() noexcept {
    while (!m_window.ShouldClose()) {
        // HFT의 Tick 간격 계산과 동일한 물리 시간 델타값 확보
        float dt = GetFrameTime(); 
        
        // [교재의 핵심 3단계 파이프라인 Hot Path]
        InputCommand cmd = InputManager::Poll(); // 1. 입력 (Input Polling)
        Update(dt, cmd);                         // 2. 갱신 (State Update)
        Render(cmd);                             // 3. 렌더링 (Render View)
    }
}


// =========================================================================
// 1. Hot Path: 메인 오케스트레이션 함수 (High-Level Pipeline)
// =========================================================================

// [Chapter 30 추가] 메모리 재할당 없는 결정론적 상태 리셋 (Deterministic Reset)
inline void Engine::ResetGame() noexcept {
    m_gameState = GameState::Ready;
    m_score = 0;

    // [Chapter 34 추가] 남은 벽돌 카운터를 전체 개수로 초기화 (O(1) 판정용)
    m_activeBrickCount = Config::TotalBricks;

    // [Chapter 28 적용] 플레이어 패들을 화면 하단 중앙으로 세팅
    // C++17 Aggregate Initialization을 통한 덮어쓰기
    m_player = PlayerState{
        Config::ScreenCenterX - (Config::PaddleWidth / 2.0f),
        Config::PaddleY,
        Config::PaddleWidth,
        Config::PaddleHeight,
        Config::PlayerSpeed, 
        Config::Theme::PlayerNormal
    };

    // [Chapter 25 적용] 공의 시작 위치를 Play Area 내부 중앙으로 맞추고 테마 색상 적용
    m_ball = BallState{
        Config::BallInitialX,
        Config::BallInitialY,
        Config::BallSpeedX,
        Config::BallSpeedY,
        Config::BallRadius,
        Config::Theme::Ball
    };

    // [Chapter 32 추가] 1차원 연속 배열을 통한 벽돌 초기화
    // 루프를 돌며 가상의 2차원 그리드 좌표를 계산하여 할당합니다.
    float startX = Config::ScreenCenterX - ((Config::BrickCols * (Config::BrickWidth + Config::BrickSpacing)) / 2.0f);

    for (int row = 0; row < Config::BrickRows; ++row) {
        for (int col = 0; col < Config::BrickCols; ++col) {
            // 2D 인덱스를 1D 인덱스로 변환 (HFT 메모리 평탄화 기법)
            int index = row * Config::BrickCols + col;

            m_bricks[index] = BrickState{
                startX + (col * (Config::BrickWidth + Config::BrickSpacing)), // X 위치
                Config::BrickStartY + (row * (Config::BrickHeight + Config::BrickSpacing)), // Y 위치
                Config::BrickWidth,
                Config::BrickHeight,
                true, // 생성 시 무조건 살아있음
                Config::Theme::PlayerNormal // 벽돌 색상
            };
        }
    }
}

// [Update Hot Path]: 프레임별 상태 및 물리 연산 통제
inline void Engine::Update(float dt, const InputCommand& cmd) noexcept {
    // --- [HFT FSM] 상태에 따른 조기 차단 (Early Exit) ---
    // 게임 오버 또는 게임 클리어 상태라면 물리 연산을 전부 건너뛰어 CPU 사이클 절약
    if (m_gameState == GameState::GameOver || m_gameState == GameState::GameClear) {
        if (cmd.restartAction) {
            ResetGame(); // 엔터 키 입력 시 즉각적인 무할당 상태 복구
        }
        return; 
    }

    // ==========================================
    // 세부 서브시스템 갱신 (Inlined execution)
    // ==========================================
    UpdatePaddle(dt, cmd);
    UpdateBall(dt, cmd);

    // [Chapter 41 추가] 물리 로직 연산이 끝난 후, 독립적인 시각 애니메이션 상태만 갱신
    UpdateAnimations(dt);

    // [HFT Optimization] Playing 상태일 때만 충돌 연산 수행
    if (m_gameState == GameState::Playing) {
        CheckPaddleCollision();
        CheckBrickCollisions();
    }

    UpdateUIAndTimers(dt, cmd);
}

// [Render Hot Path]: 레이어별 순차적 렌더링 명령 푸시
// m_renderPipeline의 상태를 변경하므로 const를 제거합니다.
inline void Engine::Render(const InputCommand& cmd) noexcept {
    // 직접 렌더링 호출을 위해 파이프라인 상단으로 BeginRender 이동
    m_window.BeginRender(); // HFT 권장: Window 내부에 ClearBackground(Config::Theme::Background) 적용

    // 1. [Chapter 39 추가] 배경 텍스처 렌더링 (가장 밑단)
    if (m_resourceMgr.IsTextureValid(TextureID::Background)) {
        DrawTexture(m_resourceMgr.GetTexture(TextureID::Background), 0, 0, WHITE);
    }

    // 파이프라인 버퍼에 렌더링 명령 적재
    RenderUIDashboard(); // LAYER 1: UI Dashboard & Telemetry Panel (상단 70px)
    RenderGameWorld();   // LAYER 2: Pure Game World (Play Area) (텍스처 직접 렌더링 + 미지원 시 기존 도형 푸시)
    RenderOverlay(cmd);  // LAYER 3: Overlay (Mouse Cursor & Game State Overlay)

    // 3. 파이프라인 큐 플러시 (UI 및 도형 일괄 렌더링)
    m_renderPipeline.Flush(); // 메모리 풀에 순차적으로 담긴 명령을 한 번에 CPU 캐시 프렌들리하게 렌더링
    
    DrawFPS(static_cast<int>(Config::UIMargin), static_cast<int>(Config::UIMargin)); //
    m_window.EndRender(); //
}

// =========================================================================
// 2. Update Sub-Pipelines (Zero-Cost Inlined Private Helpers)
// =========================================================================

// --- 1. [Chapter 28 변경] 패들 1D(좌우) 위치 갱신 ---
// 상하 이동(cmd.dy)은 완전히 무시하고 좌우 이동(cmd.dx)만 반영합니다.
inline void Engine::UpdatePaddle(float dt, const InputCommand& cmd) noexcept {
    m_player.x += cmd.dx * m_player.speed * dt;

    // [패들 경계 보정 (Clamping)]
    // 패들 좌측 끝(x)이 화면 왼쪽(0.0f) 밖으로 나가지 않도록 보정
    if (m_player.x < 0.0f) {
        m_player.x = 0.0f;
    }
    // 패들 우측 끝(x + width)이 화면 오른쪽(WindowWidth)을 넘지 않도록 보정
    else if (m_player.x + m_player.width > Config::WindowWidth) {
        m_player.x = Config::WindowWidth - m_player.width;
    }
}

// --- 2. [Chapter 25 & 27] 공 위치 갱신 & 벽 충돌 반사 ---
inline void Engine::UpdateBall(float dt, const InputCommand& cmd) noexcept {
    if (m_gameState == GameState::Ready) {
        // 게임 시작 전에는 공이 패들을 따라다니도록 위치를 고정합니다.
        m_ball.x = m_player.x + (m_player.width * 0.5f);
        m_ball.y = m_player.y - m_ball.radius - 1.0f; // 패들 상단 바로 위에 위치

        if (cmd.fireAction) {
            m_gameState = GameState::Playing; // 스페이스바 입력 시 게임 시작

            m_ball.vx = Config::BallSpeedX; // 초기 속도 설정
            m_ball.vy = Config::BallSpeedY; // 초기 속도 설정
        }
    } else if (m_gameState == GameState::Playing) {
        // 게임 진행 중에는 공이 독립적으로 움직입니다.

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
        // [Chapter 30 변경] 기존의 하단 반사 로직을 '게임 오버 판정'으로 교체
        else if (m_ball.y + m_ball.radius > Config::WindowHeight) {
            m_gameState = GameState::GameOver; // 1. 게임 오버 상태 전환
            m_resourceMgr.PlaySoundTrack(SoundID::GameOver); // 2. 게임 오버 사운드 재생
        }
    }
}

// =========================================================================
// [Chapter 41 추가] Zero-Cost Inlined 애니메이션 서브 파이프라인
// =========================================================================
inline void Engine::UpdateAnimations(float dt) noexcept {
    m_ballAnimTimer += dt;
    
    if (m_ballAnimTimer >= Config::BallAnimInterval) {
        // 잔여 시간 이월을 통한 애니메이션 Drift(밀림) 현상 방지
        m_ballAnimTimer -= Config::BallAnimInterval;
        
        m_ballFrameIndex++;
        // 분기 예측(Branch Prediction) 친화적인 O(1) 인덱스 순환
        if (m_ballFrameIndex >= Config::MaxBallFrames) {
            m_ballFrameIndex = 0;
        }
    }
}

// --- 4. [Chapter 29 추가] 공 - 패들 충돌 처리 ---
// [HFT Optimization]: 공이 아래로 이동 중(vy > 0)일 때만 충돌 검사를 수행합니다.
// 불필요한 연산을 조기 차단(Early Exit)하고 다중 프레임 연쇄 충돌 버그를 완벽히 예방합니다.
inline void Engine::CheckPaddleCollision() noexcept {
    if (m_ball.vy <= 0.0f) return;

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

        // ==========================================================
        // 벡터 정규화를 통한 총 속력(Magnitude) 보존
        // ==========================================================
        // 충돌 직전의 실제 속력을 피타고라스 정리로 계산하여 백업
        float currentSpeed = std::sqrt(m_ball.vx * m_ball.vx + m_ball.vy * m_ball.vy);

        // 반사될 방향 벡터 설정 (x는 타격 위치 비례, y는 무조건 위쪽)
        float dirX = hitOffset * 1.5f;
        float dirY = -1.0f;

        // 방향 벡터의 길이(Length) 계산
        float length = std::sqrt(dirX * dirX + dirY * dirY);

        // 길이를 1로 만든 단위 벡터(Unit Vector)에 기존 속력을 곱해 x, y축으로 재분배
        m_ball.vx = (dirX / length) * currentSpeed;
        m_ball.vy = (dirY / length) * currentSpeed;
        // ==========================================================

        // [Chapter 30 추가] 패들에 공을 튕겨낼 때마다 점수 증가
        m_score += 10;

        // 패들 충돌 시 사운드 재생
        m_resourceMgr.PlaySoundTrack(SoundID::Hit); 
    }
}

// --- 5. [Chapter 33 추가] 공 - 벽돌 충돌 처리 ---
// 1차원 배열(m_bricks)을 순회하며 충돌을 검사합니다.
// HFT 최적화: 메모리가 1차원으로 완벽히 연속되어 있어 하드웨어 프리페처(Prefetcher)가
// 데이터를 CPU L1 캐시에 매우 빠르게 올려두므로 순회 비용이 극단적으로 낮습니다.
inline void Engine::CheckBrickCollisions() noexcept {
    for (auto& brick : m_bricks) {
        // [Early Exit] 이미 파괴된 벽돌은 물리 연산을 즉시 건너뜁니다 (CPU 사이클 절약).
        if (!brick.isAlive) {
            continue;
        }

        Rectangle brickRec{ brick.x, brick.y, brick.width, brick.height };
        Vector2 ballCenter{ m_ball.x, m_ball.y };

        if (CheckCollisionCircleRec(ballCenter, m_ball.radius, brickRec)) {
            // 1. 상태 갱신: 벽돌 파괴 처리 (isAlive 플래그 변경)
            brick.isAlive = false;

            // 2. 점수 증가
            m_score += 50;

            // 3. 물리 반사: 충돌면 판별 및 속도 반전 (새로 추가된 핵심 로직)
            // 벽돌의 중심 좌표 계산
            float brickCenterX = brick.x + (brick.width * 0.5f);
            float brickCenterY = brick.y + (brick.height * 0.5f);

            // 중심점 간의 거리(차이) 계산
            float diffX = m_ball.x - brickCenterX;
            float diffY = m_ball.y - brickCenterY;

            // 직사각형 형태의 벽돌 규격에 맞춰 거리를 정규화(비율화)
            float ratioX = std::abs(diffX) / (brick.width * 0.5f);
            float ratioY = std::abs(diffY) / (brick.height * 0.5f);

            // 중심축 대비 X축으로 더 치우쳐서 맞았다면 좌/우측 충돌, 아니면 상/하단 충돌
            if (ratioX > ratioY) {
                m_ball.vx = -m_ball.vx; // 측면 충돌: X축 속도 반전
            } else {
                m_ball.vy = -m_ball.vy; // 상하 충돌: Y축 속도 반전
            }

            // =================================================================
            // [속도 증가 로직 추가]
            // 값비싼 수학 연산 없이 스칼라(Scalar) 배수 곱셈으로 벡터의 크기만 확장
            // =================================================================
            m_ball.vx *= Config::BallAccelerationRate;
            m_ball.vy *= Config::BallAccelerationRate;

            // 4. [Chapter 34 추가] O(1) 증감 연산으로 남은 벽돌 수 차감 및 승리 조건 검사
            m_activeBrickCount--;
            if (m_activeBrickCount == 0) {
                m_gameState = GameState::GameClear;
            }

            m_resourceMgr.PlaySoundTrack(SoundID::Break); // 벽돌 충돌 시 사운드 재생

            // [중요 HFT 분기 팁] 충돌 직후 즉시 루프 탈출(break)
            // 한 프레임(Tick) 안에 공이 2개 이상의 벽돌과 동시에 충돌 판정이 일어나
            // 속도가 두 번 뒤집혀 벽돌 안에서 공이 갇히는 기이한 다중 충돌 버그를 예방합니다.
            break;
        }
    }
}

// --- 6. 마우스 UI 버튼 로직 & 7. 논블로킹 누적 타이머 ---
inline void Engine::UpdateUIAndTimers(float dt, const InputCommand& cmd) noexcept {
    // 모든 UI 및 타이머 로직을 UIManager로 완벽히 위임
    m_uiMgr.Update(dt, cmd);
}

// =========================================================================
// 3. Render Sub-Pipelines (Zero-Cost Inlined Private Helpers)
// =========================================================================

// LAYER 1: UI Dashboard & Telemetry Panel (상단 70px)
inline void Engine::RenderUIDashboard() noexcept {
    // 렌더링 파이프라인과 게임 상태 데이터(점수, 패들위치)만 주입하여 렌더링 위임
    m_uiMgr.RenderDashboard(m_renderPipeline, m_score, m_player.x);
}

// LAYER 2: Pure Game World (Play Area)
// [Chapter 39 변경] 도형 렌더링에서 텍스처 렌더링 우선으로 업데이트
inline void Engine::RenderGameWorld() noexcept {
    // 1. 패들 렌더링
    if (m_resourceMgr.IsTextureValid(TextureID::Paddle)) {
        DrawTextureV(m_resourceMgr.GetTexture(TextureID::Paddle), {m_player.x, m_player.y}, WHITE);
    } else {
        // [Chapter 28 변경] 사각형 패들 렌더링
        m_renderPipeline.PushRectangle(m_player.x, m_player.y, m_player.width, m_player.height, m_player.color);
    }

    // 2. [Chapter 41 변경] 공 애니메이션 렌더링
    // O(1) 캐시 접근을 위한 텍스처 배열 직접 계산
    TextureID currentBallTex = static_cast<TextureID>(static_cast<uint8_t>(TextureID::BallFrame0) + m_ballFrameIndex);
    if (m_resourceMgr.IsTextureValid(currentBallTex)) {
        DrawTextureV(m_resourceMgr.GetTexture(currentBallTex), {m_ball.x - m_ball.radius, m_ball.y - m_ball.radius}, WHITE);
    } else {
        // [Chapter 25 추가] 공(Ball) 렌더링 (커스텀 RenderPipeline 버퍼에 푸시)
        m_renderPipeline.PushCircle(m_ball.x, m_ball.y, m_ball.radius, m_ball.color);
    }   
    
    // 3. 벽돌 렌더링
    // [Chapter 32 추가] 살아있는 벽돌만 렌더링 파이프라인에 푸시 (분기 최소화)
    // 메모리가 연속되어 있으므로 CPU 프리페처(Prefetcher)가 데이터를 매우 빠르게 미리 당겨옵니다.
    for (const auto& brick : m_bricks) {
        if (brick.isAlive) {
            if (m_resourceMgr.IsTextureValid(TextureID::Brick)) {
                DrawTextureV(m_resourceMgr.GetTexture(TextureID::Brick), {brick.x, brick.y}, WHITE);
            } else {
                m_renderPipeline.PushRectangle(brick.x, brick.y, brick.width, brick.height, brick.color);
            }
        }
    }
}

// LAYER 3: Overlay (Mouse Cursor & Game State Overlay)
inline void Engine::RenderOverlay(const InputCommand& cmd) noexcept {
    // 마우스 커서는 화면 내 모든 요소보다 위에 있어야 하므로 가장 마지막에 렌더링 큐에 넣습니다.
    Color cursorColor = cmd.leftClickDown ? Config::Theme::HeartbeatActive : Config::Theme::PlayerNormal;
    m_renderPipeline.PushCircle(cmd.mouseX, cmd.mouseY, 5.0f, cursorColor);

    // 대기 및 진행 상태일 때는 오버레이 패스 (Early Exit)
    if (m_gameState == GameState::Ready || m_gameState == GameState::Playing) {
        return;
    }

    // [Chapter 30/34 추가] 상태 오버레이 (반투명 배경 덮개)
    m_renderPipeline.PushRectangle(
        0.0f, 
        Config::PlayAreaY, 
        Config::WindowWidth, 
        Config::WindowHeight - Config::PlayAreaY, 
        Fade(BLACK, 0.7f)
    );

    const char* mainText = nullptr;
    Color textColor = WHITE;

    // [HFT Optimization]: switch 문을 통한 O(1) Jump Table 상태 분기
    switch (m_gameState) {
        case GameState::GameOver:
            mainText = "GAME OVER";
            textColor = RED;
            break;
        case GameState::GameClear:
            mainText = "VICTORY!";
            textColor = GREEN;
            break;
        default:
            return;
    }

    const char* restartText = "Press ENTER to Restart";
    int mainTextWidth = MeasureText(mainText, 40);
    int restartTextWidth = MeasureText(restartText, 20);

    m_renderPipeline.PushText(
        Config::ScreenCenterX - (mainTextWidth / 2.0f), 
        Config::ScreenCenterY - 30.0f, 
        mainText, 
        40, 
        textColor
    );

    m_renderPipeline.PushText(
        Config::ScreenCenterX - (restartTextWidth / 2.0f), 
        Config::ScreenCenterY + 20.0f, 
        restartText, 
        20, 
        RAYWHITE
    );
}