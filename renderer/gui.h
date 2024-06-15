#pragma once
#include "device_manager.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

namespace GAS 
{
    class GameScene;
}
namespace Renderer 
{
    class BaseRenderer;

    class Gui {
    public:
        explicit Gui(std::weak_ptr<BaseRenderer> InRenderer);
        ~Gui();

        bool CreateGui(HWND InWindow);
        void BeginGui();
        void EndGui(ID3D12GraphicsCommandList* InCmd);
        void Render();

        void SceneMaterials();

        void SetCurrentScene(std::shared_ptr<GAS::GameScene> InGameScene);
        void SetRenderer(std::weak_ptr<BaseRenderer> InRenderer);
    private:
        void SceneUpdate();
        void EntityPanel(entt::entity e);
        void Property(std::string name,float* value,float min,float max);
        std::shared_ptr < GAS::GameScene> mCurrentScene;
        std::vector<entt::entity> mEntities;
        std::vector<std::string> mEntitiesDisplayName;
        int mCurrentIndex = 0;
        float t[3] = { 0.0f };
        float r[3] = { 0.0f };
        float s[3] = { 0.0f };
        entt::entity mCurrentEntity;
        std::weak_ptr<BaseRenderer> mRenderer;
        int mMatIndex = -1;
        void AddFile(std::function<void(const std::filesystem::path&)>);
        std::string_view mSelectedMatName;
    };
}