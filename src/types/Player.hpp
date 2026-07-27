#pragma once
#include <raylib.h>

// [Chapter 28 변경] 원형 플레이어에서 사각형 패들(Paddle) POD 구조체로 재정의
// alignas(16): SIMD 벡터 연산 및 16바이트 C++ 캐시 라인 정렬 최적화
struct alignas(16) PlayerState final {
    float x{0.0f};      // 패들 좌상단 X 좌표
    float y{0.0f};      // 패들 좌상단 Y 좌표 (고정 값)
    float width{0.0f};  // 패들 너비
    float height{0.0f}; // 패들 높이
    float speed{0.0f};  // X축 이동 속도
    Color color{};      // 패들 색상
};