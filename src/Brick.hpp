#pragma once
#include "raylib.h"

// [HFT Optimization]: 메모리 정렬 및 캐시 친화성을 고려한 데이터 컨테이너
struct BrickState {
    float x;
    float y;
    float width;
    float height;
    bool isAlive;  // 상태 플래그 (1 = 생존, 0 = 파괴)
    Color color;
};