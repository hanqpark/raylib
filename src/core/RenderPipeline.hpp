#pragma once
#include <raylib.h>
#include <array>
#include <cstdint>
#include "Config.hpp"

// [HFT 설계] 32바이트 정렬(alignas). 
// CPU 캐시 라인(64 byte)에 정확히 2개씩 탑재되도록 강제하여 캐시 미스를 원천 차단합니다.
// 데이터 크기 검증: float(4)*4 + 포인터(8) + uint32(4) + Color(4) = 정확히 32바이트 (64비트 OS 기준)
struct alignas(32) RenderCommand final {
    float x1, y1;
    float x2, y2;       // 사각형 크기, 선의 끝점, 또는 텍스트 폰트 사이즈(x2)로 메모리 재사용
    const char* text;   // [추가] 텍스트 렌더링용 문자열 포인터 (std::string 절대 사용 금지)
    uint32_t type;      // 0: Circle, 1: Rectangle, 2: Line, 3: Text
    Color color;
};

class RenderPipeline final {
public:
    RenderPipeline() noexcept = default;
    ~RenderPipeline() noexcept = default;

    // 복사 및 이동 금지 (단일 인스턴스로만 존재)
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    // inline 및 noexcept: 함수 호출 비용과 예외 처리 비용 제거
    inline void PushCircle(float x, float y, float radius, Color color) noexcept {
        if (m_count < Config::MaxRenderCommands) {
            m_queue[m_count++] = {x, y, radius, 0.0f, nullptr, 0, color};
        }
    }

    inline void PushRectangle(float x, float y, float width, float height, Color color) noexcept {
        if (m_count < Config::MaxRenderCommands) {
            m_queue[m_count++] = {x, y, width, height, nullptr, 1, color};
        }
    }

    inline void PushLine(float startX, float startY, float endX, float endY, Color color) noexcept {
        if (m_count < Config::MaxRenderCommands) {
            m_queue[m_count++] = {startX, startY, endX, endY, nullptr, 2, color};
        }
    }

    inline void PushText(float x, float y, const char* text, int fontSize, Color color) noexcept {
        if (m_count < Config::MaxRenderCommands) {
            m_queue[m_count++] = {x, y, 0.0f, static_cast<float>(fontSize), text, 3, color};
        }
    }

    // [HFT 설계] 일괄 처리(Batch Processing)
    void Flush() noexcept {
        // 하드웨어 프리패처(Prefetcher)가 메모리를 연속으로 읽어들이며 성능 극대화
        for (std::size_t i = 0; i < m_count; ++i) {
            const auto& cmd = m_queue[i];
            
            switch (cmd.type) {
                case 0: 
                    DrawCircle(static_cast<int>(cmd.x1), static_cast<int>(cmd.y1), cmd.x2, cmd.color); 
                    break;
                case 1: 
                    DrawRectangle(static_cast<int>(cmd.x1), static_cast<int>(cmd.y1), 
                                  static_cast<int>(cmd.x2), static_cast<int>(cmd.y2), cmd.color); 
                    break;
                case 2: 
                    DrawLine(static_cast<int>(cmd.x1), static_cast<int>(cmd.y1), 
                             static_cast<int>(cmd.x2), static_cast<int>(cmd.y2), cmd.color); 
                    break;
                case 3:
                    DrawText(cmd.text, static_cast<int>(cmd.x1), static_cast<int>(cmd.y1), 
                             static_cast<int>(cmd.x2), cmd.color);
                    break;
            }
        }
        
        // 메모리 해제 없이 카운터만 초기화 (Zero-Cost)
        m_count = 0;
    }

private:
    std::array<RenderCommand, Config::MaxRenderCommands> m_queue{};
    std::size_t m_count{0};
};