#include "gui.h"
#include "game_scene.h"
#include "components.h"
#include "renderer.h"
#include <commdlg.h>

Renderer::Gui::Gui(std::weak_ptr<BaseRenderer> InRenderer):
    mRenderer(InRenderer)
{
  
}

Renderer::Gui::~Gui() {
}

bool Renderer::Gui::CreateGui(HWND InWindow) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // IF using Docking Branch

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(InWindow);
    ID3D12DescriptorHeap* srvHeap =  Renderer::g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap();
    ImGui_ImplDX12_Init(
            Renderer::g_Device, Renderer::SWAP_CHAIN_BUFFER_COUNT, Renderer::g_DisplayFormat, srvHeap,
            // You'll need to designate a descriptor from your descriptor heap for Dear ImGui to use internally for its font texture's SRV
            srvHeap->GetCPUDescriptorHandleForHeapStart(), srvHeap->GetGPUDescriptorHandleForHeapStart());
    return true;
}

void Renderer::Gui::BeginGui() {
    // (Your code process and dispatch Win32 messages)
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

}

void Renderer::Gui::EndGui(ID3D12GraphicsCommandList* InCmd) {
    // Rendering
    // (Your code clears your framebuffer, renders your other stuff etc.)
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), InCmd);
    // (Your code calls ExecuteCommandLists, swapchain's Present(), etc.)
}

void Renderer::Gui::Render() {
    ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "Hello Re3D");
    // Custom size: use all width, 5 items tall
    //ImGui::Text("Full-width:");
    if (mCurrentScene)
    {
        SceneUpdate();
        if (!mEntities.empty())
        {
			EntityPanel(mCurrentEntity);
        }
    }
}

void Renderer::Gui::SceneMaterials()
{
	if (ImGui::TreeNode("Scene Material"))
	{
		auto& textureMap = mRenderer.lock()->GetSceneTextureMap();
		if (textureMap.size() != 0)
		{
			int n = 0;
			std::for_each(textureMap.begin(), textureMap.end(), [&n, this](std::pair<std::string_view, std::shared_ptr<Resource::Texture>> texture)
				{
					if (ImGui::Selectable(texture.first.data(), mMatIndex == n))
					{
						mMatIndex = n;
                        mSelectedMatName = texture.first;
					}
					n++;
				});
		}
		ImGui::TreePop();
	}
    if (ImGui::Button("Add Material"))
    {
        AddFile([=](const std::filesystem::path& fileName)
            {
				mRenderer.lock()->LoadMaterial(fileName.string());
            });
		
    }
}

void Renderer::Gui::SetCurrentScene(std::shared_ptr<GAS::GameScene> InGameScene) 
{
    mCurrentScene = InGameScene;
    auto& sceneRegistry = mCurrentScene->GetRegistery();
    for (auto entt: sceneRegistry.view<entt::entity>()) {
        mEntities.push_back(entt);
    }

    if (!mEntities.empty())
    {
        GameSceneUpdate(InGameScene, mEntities);
		mCurrentEntity = mEntities[0];
    }
	GAS::GameScene::sOnNewEntityAdded.push_back(std::bind(&Gui::GameSceneUpdate, this, std::placeholders::_1, std::placeholders::_2));
}

void Renderer::Gui::SetRenderer(std::weak_ptr<BaseRenderer> InRenderer)
{
    mRenderer = InRenderer;
}

void Renderer::Gui::SceneUpdate() 
{
    if (ImGui::Button("Add Entity"))
    {
        AddFile([this](const std::filesystem::path& InFilePath) 
            {
                auto extension = InFilePath.extension();
                if (extension.string() == ".fbx" || extension.string() == ".FBX")
                {
                    auto newEntities =  mCurrentScene->CreateEntitiesWithMesh(InFilePath.string());
					for (const auto& newEntity : newEntities)
					{
						
					}
                }
            });
    }
    
    if (ImGui::BeginListBox("##Entities", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
        static int mCurrentIndex = 0;
        for (int n = 0; n < mEntities.size(); n++) {
            const bool is_selected = (mCurrentIndex == n);
            if (ImGui::Selectable(mEntitiesDisplayName[n].c_str(), is_selected)) 
            {
                //update entity
                mCurrentIndex = n;
                mCurrentEntity = mEntities[mCurrentIndex];
                auto& registry = mCurrentScene->GetRegistery();
                auto& translate = registry.get<ECS::TransformComponent>(mCurrentEntity).GetTranslate();
                auto& scale = registry.get<ECS::TransformComponent>(mCurrentEntity).GetScale();
                auto& rotation = registry.get<ECS::TransformComponent>(mCurrentEntity).GetRotation();

                t[0] = translate.x;
                t[1] = translate.y;
                t[2] = translate.z;

                r[0] = rotation.x;
                r[1] = rotation.y;
                r[2] = rotation.z;

                s[0] = scale.x;
                s[1] = scale.y;
                s[2] = scale.z;
            }
        
            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected) 
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
	SceneMaterials();
}

void Renderer::Gui::GameSceneUpdate(std::shared_ptr<GAS::GameScene> InGameScene, std::span<entt::entity> InEntities)
{
	int n = 0;
    auto& sceneRegistry = InGameScene->GetRegistery();
    for (auto entity : InEntities)
    {
		auto name = std::string("Entity") + std::to_string(n);
		if (ECS::StaticMeshComponent* lStaticComponent = sceneRegistry.try_get<ECS::StaticMeshComponent>(entity))
		{
			name += "-" + lStaticComponent->mName;
			mRenderer.lock()->LoadStaticMeshToGpu(*lStaticComponent);
		}
		mEntitiesDisplayName.push_back(name);
        mEntities.push_back(entity);
		n++;
    }
    mCurrentEntity = mEntities[0];
}

void Renderer::Gui::EntityPanel(entt::entity e) 
{
    auto& registry = mCurrentScene->GetRegistery();
    using namespace DirectX::SimpleMath;
    //Transform 
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Entity Component", tab_bar_flags)) {
        if (ImGui::BeginTabItem("Transform")) {
           
            Property("Translation", t, -100.0f, 100.0f);
            Property("Rotation", r, -360.0, 360.0f);
            Property("Scale", s, -100.0f, 100.0f);
            auto& transformComponent = registry.get<ECS::TransformComponent>(e);
            transformComponent.Translate(DirectX::SimpleMath::Vector3(t[0],t[1],t[2]));
            transformComponent.Rotate(Vector3(r[0], r[1], r[2]));
            transformComponent.Scale(DirectX::SimpleMath::Vector3(s[0], s[1], s[2]));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Material")) {
			auto& meshComponent = registry.get<ECS::StaticMeshComponent>(e);
            if (ImGui::Button("Apply Diffuse Texture"))
            {
                meshComponent.MatName = mSelectedMatName;
            }
			if (ImGui::Button("Apply Normal Texture"))
			{
				meshComponent.NormalMap = mSelectedMatName;
			}
            ImGui::Text("This is the Broccoli tab!\nblah blah blah blah blah");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void Renderer::Gui::Property(std::string name,float* value, float min, float max) 
{
    ImGui::DragFloat3(name.c_str(), value,1.0f, min, max);
}

void Renderer::Gui::AddFile(std::function<void(const std::filesystem::path&)> InCallBack)
{
	OPENFILENAME ofn;       // common dialog box structure
    char szFile[MAX_PATH] = {};       // buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = "All Files\0*.*\0Text Files\0*.TXT\0";
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;

	if (GetOpenFileName(&ofn)) {
        InCallBack(szFile);
	}
	else {
        return InCallBack({});
	}
}
