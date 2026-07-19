#pragma once
#include <raylib.h>

// HFT의 '주문(Order)' 또는 '패킷(Packet)' 역할을 할 구조체
// 런타임에 동적으로 크기가 변하지 않는 가벼운 POD(Plain Old Data) 형태를 유지합니다.
struct InputCommand final {
    float dx;
    float dy;
    bool fireAction; // [추가] 단발성 트리거 (예: 총알 발사 시그널, 주문 전송)
};

class InputManager final {
public:
    // 인스턴스화 불필요 (정적 함수로 캐시 오버헤드 제거)
    [[nodiscard]] static InputCommand Poll() noexcept {
        // 기본 상태: 이동 안함, 트리거 안눌림
        InputCommand cmd{0.0f, 0.0f, false};

        /* 1. [연속적 입력] IsKeyDown
        // 누르고 있는 동안 매 프레임 참(true)을 반환합니다. (지속적인 좌표 갱신용)
        // 분기 예측(Branch Prediction) 오버헤드가 다소 있으나, 
        // 현 단계의 클라이언트 뷰에서는 허용 가능한 수준의 연속 if문 사용 */
        if (IsKeyDown(KEY_RIGHT)) cmd.dx += 1.0f;
        if (IsKeyDown(KEY_LEFT))  cmd.dx -= 1.0f;
        if (IsKeyDown(KEY_DOWN))  cmd.dy += 1.0f;
        if (IsKeyDown(KEY_UP))    cmd.dy -= 1.0f;

        /* 2. [단발성 입력] IsKeyPressed
        // 키를 꾹 누르고 있어도, 처음 눌린 그 1프레임(1번)만 참(true)을 반환합니다.
        // HFT에서 특정 조건을 만족했을 때 단 1번의 주문(Order)을 발생시키는 엣지 트리거(Edge-trigger)와 같습니다. */
        if (IsKeyPressed(KEY_SPACE)) {
            cmd.fireAction = true;
        }

        return cmd;
    }
};