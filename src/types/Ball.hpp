#pragma once
#include <raylib.h>

// HFT Low-Latency: 16바이트 메모리 경계 정렬을 적용한 POD 구조체
// CPU 캐시 라인 단위를 효율적으로 활용하며 Zero-Allocation을 보장합니다.
struct alignas(16) BallState {
    float x;
    float y;
    float vx; // x축 속도 (Velocity X, 초당 이동 픽셀)
    float vy; // y축 속도 (Velocity Y, 초당 이동 픽셀)
    float radius;
    Color color;
};