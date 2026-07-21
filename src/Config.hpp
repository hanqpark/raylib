// 1. #pragma once: 이 헤더 파일이 딱 한 번만 컴파일되도록 보장하는 컴파일러 지시어
#pragma once

#include <raylib.h>
#include <string_view>

// 3. namespace: 단순히 이름 충돌을 막고 논리적으로 묶어주는 역할 (인스턴스화 불가)
namespace Config {
    // 4. constexpr: 실행 시(Runtime)가 아니라 컴파일 시(Compile-time)에 값을 완전히 결정함
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 450;
    
    // Chapter 11. std::string_view는 문자열의 '시작 주소'와 '길이'만 가지는 아주 가벼운 참조 구조체
    constexpr std::string_view WindowTitle = "HFT Low-Latency Base Ch 28. - Paddle Entity";
    constexpr int TargetFPS = 60;

    // --- [Chapter 18 추가] 레이아웃 분할 상수 ---
    // 화면을 시각적으로 상단 UI 영역과 하단 Play 영역으로 나눕니다.
    constexpr float UIPanelHeight = 70.0f;                      // 상단 UI 영역 높이
    constexpr float PlayAreaY = UIPanelHeight;                  // 플레이 영역의 Y 시작점
    constexpr float PlayAreaHeight = WindowHeight - UIPanelHeight;
    constexpr float UIMargin = 15.0f;                           // 정보 우선순위를 위한 여백(Margin)

    // --- [Chapter 18 추가] 시맨틱 컬러(Semantic Colors) 테마 ---
    // HFT 시스템 대시보드처럼 상태와 목적에 따라 색상 이름을 부여하여 중앙 통제합니다.
    namespace Theme {
        // 배경 영역 분리 (다크 모드 기준)
        constexpr Color Background = { 20, 20, 20, 255 };       // 전체 기본 배경
        constexpr Color UIPanelBg  = { 35, 35, 35, 255 };       // 상단 UI 패널 배경

        // 텍스트 계층 분리
        constexpr Color TextTitle  = WHITE;                     // 강조 텍스트
        constexpr Color TextNormal = LIGHTGRAY;                 // 일반 정보 텍스트

        // 플레이어/오브젝트 상태 분리
        constexpr Color PlayerNormal = { 0, 121, 241, 255 };    // 파란색 (기본/안전)
        constexpr Color PlayerAction = { 0, 228, 48, 255 };     // 녹색 (액션/성공)

        // 시스템 상태 (하트비트 등)
        constexpr Color HeartbeatNormal = DARKGRAY;             // 대기
        constexpr Color HeartbeatActive = RED;                  // 알림/경고

        // UI 컴포넌트 상호작용
        constexpr Color ButtonDefault = GRAY;
        constexpr Color ButtonActive  = LIME;

        // --- [Chapter 25 추가] 공 시맨틱 컬러 (가독성 높은 골드 톤) ---
        constexpr Color Ball           = { 240, 200, 80, 255 };
    }

    // --- [Chapter 28 추가] 플레이어 패들(Paddle) 규격 설정 ---
    constexpr float PaddleWidth  = 100.0f; // 패들 너비
    constexpr float PaddleHeight = 15.0f;  // 패들 높이
    constexpr float PaddleY      = WindowHeight - 30.0f; // 화면 하단 근처 고정 Y 위치
    constexpr float PlayerSpeed  = 400.0f; // 좌우 이동 속도 (픽셀/초)

    // --- [Chapter 25 추가] 공 물리 및 초기화 설정 상수 ---
    constexpr float BallRadius   = 12.0f;
    constexpr float BallInitialX = WindowWidth / 2.0f;
    constexpr float BallInitialY = PlayAreaY + (PlayAreaHeight / 2.0f);
    constexpr float BallSpeedX   = 180.0f; // x축 이동 속도 (픽셀/초)
    constexpr float BallSpeedY   = 120.0f; // y축 이동 속도 (픽셀/초)

    // Chapter 13. Zero-Allocation 메모리 풀의 고정 크기 지정
    constexpr std::size_t MaxRenderCommands = 10000;

    /* Chapter 14. C교재 실습용 상수 및 컴파일 타임(Compile-time) 사전 연산
    // 화면 크기가 변경되어도 아래의 중앙 좌표는 컴파일러가 빌드할 때 자동으로 다시 계산해 둡니다.
    // 즉, 런타임(게임 실행 중)에는 나눗셈 연산 오버헤드가 '제로'가 됩니다. */
    constexpr float ScreenCenterX = WindowWidth / 2.0f;
    constexpr float ScreenCenterY = WindowHeight / 2.0f;

    // Chapter 15. 마우스 클릭 테스트용 UI 버튼 영역 설정 (매직 넘버 제거)
    constexpr float UIButtonX = 350.0f;
    constexpr float UIButtonY = 350.0f;
    constexpr float UIButtonWidth = 100.0f;
    constexpr float UIButtonHeight = 40.0f;

    // Chapter 16.  교재 실습용 누적 타이머 주기 (3초)
    constexpr float HeartbeatInterval = 3.0f;
}