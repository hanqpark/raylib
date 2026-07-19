// 1. #pragma once: 이 헤더 파일이 딱 한 번만 컴파일되도록 보장하는 컴파일러 지시어
#pragma once

#include <raylib.h>
#include <string_view> // 2. std::string_view를 사용하기 위한 헤더

// 3. namespace: 단순히 이름 충돌을 막고 논리적으로 묶어주는 역할 (인스턴스화 불가)
namespace Config {
    // 4. constexpr: 실행 시(Runtime)가 아니라 컴파일 시(Compile-time)에 값을 완전히 결정함
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 450;
    
    // std::string_view는 문자열의 '시작 주소'와 '길이'만 가지는 아주 가벼운 참조 구조체
    constexpr std::string_view WindowTitle = "HFT Low-Latency Base v1.1";
    constexpr int TargetFPS = 60;
    constexpr Color BackgroundColor = RAYWHITE;

    // Part 3: 플레이어(원형 도형) 컴파일 타임 설정
    constexpr float PlayerSpeed = 300.0f; // 초당 이동 픽셀 (float형 명시)
    constexpr float PlayerRadius = 25.0f;
    constexpr Color PlayerColor = MAROON;

    // [HFT 추가] Zero-Allocation 메모리 풀의 고정 크기 지정
    constexpr std::size_t MaxRenderCommands = 10000;

    // [HFT 추가] 교재 실습용 상수 및 컴파일 타임(Compile-time) 사전 연산
    // 화면 크기가 변경되어도 아래의 중앙 좌표는 컴파일러가 빌드할 때 자동으로 다시 계산해 둡니다.
    // 즉, 런타임(게임 실행 중)에는 나눗셈 연산 오버헤드가 '제로'가 됩니다.
    constexpr float ScreenCenterX = WindowWidth / 2.0f;
    constexpr float ScreenCenterY = WindowHeight / 2.0f;

    // 테스트용 사각형 크기 상수화 (매직 넘버 제거)
    constexpr float SampleRectWidth = 100.0f;
    constexpr float SampleRectHeight = 50.0f;
}