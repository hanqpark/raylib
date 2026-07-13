#pragma once
#include "Config.hpp"

// 1. final: 이 클래스는 자식을 가질 수 없다(상속 금지)고 컴파일러에게 선언합니다.
class Window final {
public:
    // 2. noexcept: 이 함수는 실행 중 예외(Exception, 에러 발생 시 던지는 것)를 절대 발생시키지 않는다고 보장합니다.
    Window() noexcept {
        InitWindow(Config::WindowWidth, Config::WindowHeight, Config::WindowTitle.data());
        SetTargetFPS(Config::TargetFPS);
    }

    ~Window() noexcept {
        CloseWindow();
    }

    // 4. = delete: 이 객체는 복사(Copy)되거나 이동(Move)될 수 없도록 해당 기능들을 완전히 삭제합니다.
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    // 5. [[nodiscard]]: 이 함수의 결과값을 변수에 안 담고 버리면 컴파일러가 경고를 냅니다.
    // 2. const: 이 함수 안에서는 클래스의 멤버 변수를 수정할 수 없음을 의미합니다. (읽기 전용)
    [[nodiscard]] bool ShouldClose() const noexcept {
        return WindowShouldClose();
    }

    void BeginRender() const noexcept {
        BeginDrawing();
        ClearBackground(Config::BackgroundColor);
    }

    void EndRender() const noexcept {
        EndDrawing();
    }
};