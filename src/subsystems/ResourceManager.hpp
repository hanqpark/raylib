#pragma once
#include <array>
#include <cstdint>
#include "ResourceHandle.hpp" // (TextureHandle, SoundHandle이 정의된 헤더)
#include "Config.hpp"

// =========================================================================
// [경로 상수 정의] 컴파일 타임에 문자열이 하나로 결합되어 Zero-Cost 달성
// =========================================================================
#define DIR_TEX "resources/textures/"
#define DIR_SND "resources/audio/"

// [HFT Optimization] 1바이트 크기의 엄격한 Enum을 배열 인덱스로 사용
enum class TextureID : uint8_t {
    Background = 0,
    Paddle,
    Brick,
    BallFrame0,
    BallFrame1,
    BallFrame2,
    Max // 배열의 크기를 동적으로 결정하기 위한 마커
};

enum class SoundID : uint8_t {
    Hit = 0,
    Break,
    GameOver,
    Max
};

class ResourceManager final {
public:
    ResourceManager() noexcept = default;
    ~ResourceManager() noexcept = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // 엔진 초기화 시점에 디스크 I/O를 일괄 처리
    void PreloadResources() noexcept;

    // O(1) 인라인 접근: 분기문이나 해싱 없이 즉각적인 메모리 주소 반환
    inline const Texture2D& GetTexture(TextureID id) const noexcept{
        return m_textures[static_cast<size_t>(id)].Get();
    }

    inline bool IsTextureValid(TextureID id) const noexcept{
        return m_textures[static_cast<size_t>(id)].IsValid();
    }

    inline void PlaySoundTrack(SoundID id) const noexcept{
        m_sounds[static_cast<size_t>(id)].Play();
    }

private:
    // [HFT 구조] 힙 할당(unordered_map)을 배제한 스택/플랫 메모리 연속 배열
    
    // Chapter 39. 리소스 핸들 (RAII)
    // [Chapter 41 추가] 스프라이트 애니메이션 상태 관리 변수 (기존 m_ballTex 대체)
    // 힙 할당을 배제하고 인접한 메모리 공간에 연속으로 배치하여 캐시 히트율 보장
    std::array<TextureHandle, static_cast<size_t>(TextureID::Max)> m_textures;
    
    // [Chapter 43 추가] 오디오 리소스 핸들 (RAII)
    std::array<SoundHandle, static_cast<size_t>(SoundID::Max)> m_sounds;
};

// =========================================================================
// [인라인 구현부]
// =========================================================================
inline void ResourceManager::PreloadResources() noexcept{
    // [Chapter 39 추가] 게임 루프 진입 전 I/O를 100% 끝내서(Pre-load) 런타임 지연시간(Jitter) 차단
    
    // 배경은 투명화가 필요 없으므로 게임에 사용하지 않는 더미 컬러(MAGENTA)를 키로 주고 화면 꽉 차게 설정
    m_textures[static_cast<size_t>(TextureID::Background)] = TextureHandle(DIR_TEX "background.png", Config::WindowWidth, Config::WindowHeight, MAGENTA);
    
    // [수정됨] Config에 정의된 물리적 크기에 맞춰 픽셀 리사이징 및 투명화 적용
    m_textures[static_cast<size_t>(TextureID::Paddle)] = TextureHandle(DIR_TEX "paddle.png", static_cast<int>(Config::PaddleWidth), static_cast<int>(Config::PaddleHeight));
    m_textures[static_cast<size_t>(TextureID::Brick)] = TextureHandle(DIR_TEX "brick.png", static_cast<int>(Config::BrickWidth), static_cast<int>(Config::BrickHeight));

    // 공의 경우 반지름(Radius)을 사용하므로 지름(Diameter)으로 변환하여 리사이징
    int ballDiameter = static_cast<int>(Config::BallRadius * 2.0f);
    
    // [Chapter 41 변경] 단일 텍스처(m_ballTex)를 제거하고 프레임 텍스처 배열을 VRAM에 사전 적재(Pre-load)
    // 게임 루프 중 디스크 I/O를 원천 차단합니다.
    m_textures[static_cast<size_t>(TextureID::BallFrame0)] = TextureHandle(DIR_TEX "ball_0.png", ballDiameter, ballDiameter);
    m_textures[static_cast<size_t>(TextureID::BallFrame1)] = TextureHandle(DIR_TEX "ball_1.png", ballDiameter, ballDiameter);
    m_textures[static_cast<size_t>(TextureID::BallFrame2)] = TextureHandle(DIR_TEX "ball_2.png", ballDiameter, ballDiameter);

    // [Chapter 43 추가] 사운드 파일 Pre-load (디스크 I/O 완전 차단)
    m_sounds[static_cast<size_t>(SoundID::Hit)] = SoundHandle(DIR_SND "hit.wav");
    m_sounds[static_cast<size_t>(SoundID::Break)] = SoundHandle(DIR_SND "break.wav");
    m_sounds[static_cast<size_t>(SoundID::GameOver)] = SoundHandle(DIR_SND "gameover.wav");
}