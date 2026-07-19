#pragma once
#include <raylib.h>

// 메모리상에 연속적으로 배치될 순수 데이터 덩어리
struct PlayerState final {
    float x;
    float y;
    float speed;
    float radius;
    Color color;
};