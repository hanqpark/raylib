#pragma once
#include <raylib.h>

// HFT의 '주문(Order)' 또는 '패킷(Packet)' 역할을 할 구조체
struct InputCommand final {
    float dx;
    float dy;
};

class InputManager final {
public:
    // 인스턴스화 불필요 (정적 함수로 캐시 오버헤드 제거)
    [[nodiscard]] static InputCommand Poll() noexcept {
        InputCommand cmd{0.0f, 0.0f};

        // 분기 예측(Branch Prediction) 오버헤드가 다소 있으나, 
        // 현 단계의 클라이언트 뷰에서는 허용 가능한 수준의 연속 if문 사용
        if (IsKeyDown(KEY_RIGHT)) cmd.dx += 1.0f;
        if (IsKeyDown(KEY_LEFT))  cmd.dx -= 1.0f;
        if (IsKeyDown(KEY_DOWN))  cmd.dy += 1.0f;
        if (IsKeyDown(KEY_UP))    cmd.dy -= 1.0f;

        return cmd;
    }
};