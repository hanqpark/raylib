#pragma once
#include <raylib.h>

// HFT의 '주문(Order)' 또는 '패킷(Packet)' 역할을 할 구조체
// 런타임에 동적으로 크기가 변하지 않는 가벼운 POD(Plain Old Data) 형태를 유지합니다.
// 변수가 추가되었지만 20바이트 내외이므로 CPU 레지스터에 충분히 담깁니다.
struct InputCommand final {
    float dx{0.0f}; // 좌우 이동 (Left/Right)
    float dy{0.0f}; // 상하 이동 (Up/Down)

    // Chapter 14. 단발성 트리거 (예: 총알 발사 시그널, 주문 전송)
    bool fireAction{false}; 

    // Chapter 15. 마우스 데이터
    float mouseX{0.0f};
    float mouseY{0.0f};
    bool leftClickDown{false};    // 누르고 있는 상태 (드래그, 연속 발사)
    bool leftClickPressed{false}; // 단발성 클릭 (UI 버튼 클릭)

    // [Chapter 30 추가] 게임 오버 시 재시작을 위한 시그널
    bool restartAction{false};
};

class InputManager final {
public:
    // 인스턴스화 불필요 (정적 함수로 캐시 오버헤드 제거)
    [[nodiscard]] static InputCommand Poll() noexcept {
        // 기본 상태: 이동 안함, 트리거 안눌림
        InputCommand cmd{0.0f, 0.0f, false, 0.0f, 0.0f, false, false};

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
        if (IsKeyPressed(KEY_SPACE)) cmd.fireAction = true;

        // [Chapter 30 추가] 엔터 키 엣지 트리거 (재시작 시그널 발생)
        if (IsKeyPressed(KEY_ENTER)) cmd.restartAction = true;

        /* 3. [추가] 마우스 폴링
        // 매 프레임 마우스의 정확한 픽셀 좌표를 캡처합니다. */
        cmd.mouseX = static_cast<float>(GetMouseX());
        cmd.mouseY = static_cast<float>(GetMouseY());

        // MOUSE_BUTTON_LEFT(0)를 사용해 좌클릭 상태를 캡처합니다.
        cmd.leftClickDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        cmd.leftClickPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        return cmd;
    }
};