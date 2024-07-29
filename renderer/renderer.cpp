#include "renderer.h"
#include "obj_model_loader.h"
#include "utility.h"
#include <camera.h>
#include "components.h"
#include "gui.h"
#include "renderer_context.h"
#include "PostProcess.h"
#include "GraphicsMemory.h"

Renderer::BaseRenderer::BaseRenderer():
	mDeviceManager(std::make_unique<DeviceManager>()),
	mCmdManager(mDeviceManager->GetCmdManager()),
	mIsFirstFrame(true),
	mComputeFence(nullptr),
	mGraphicsCmd(nullptr),
	mSkybox(nullptr),
	mComputeFenceValue(1),
	mCurrentScene(nullptr),
	mGraphicsFenceValue(1)
{
	Ensures(AssetLoader::gStbTextureLoader);
	Ensures(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mComputeFence)) == S_OK);
	mComputeFenceHandle = CreateEvent(nullptr, false, false, nullptr);
	mGraphicsCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	mBatchUploader = std::make_unique<ResourceUploadBatch>(g_Device);
	mContext = std::make_unique<RendererContext>(mCmdManager);
	//Allocate 3 desc for IMGUI.Todo:move this gui,make it transparent to renderer.
	g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate(3);
	CreateTextures();
	CreateBuffers();
	CreateRootSignature();
	CreatePipelineState();
	CreateRenderTask();
	CreateSkybox();
	InitPostProcess();
	//GAS::GameScene::sOnNewEntityAdded.push_back(std::bind(&Renderer::BaseRenderer::OnGameSceneUpdated,this,std::placeholders::_1, std::placeholders::_2));
}

Renderer::BaseRenderer::~BaseRenderer()
{
	
}


void Renderer::BaseRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
    mWindow = InWindow;
	mWidth = InWidth;
	mHeight = InHeight;
	mDeviceManager->SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	//Use Reverse Z
	mDefaultCamera = std::make_unique<Gameplay::PerspectCamera>((float)InWidth, (float)InHeight, 0.1f,true);
	mDefaultCamera->LookAt({ 3.0,3.0,2.0 }, { 0.0f,3.0f,0.0f }, { 0.0f,1.0f,0.0f });
	mShadowCamera = std::make_unique<Gameplay::PerspectCamera>((float)InWidth, (float)InHeight, 0.1f,false);
	mShadowCamera->LookAt({ -5,25,0.0 }, { 0.0f,0.0f,0.0f }, { 0.0f,1.0f,0.0f });
	mViewPort = { 0,0,(float)mWidth,(float)mHeight,0.0,1.0 };
	mRect = { 0,0,mWidth,mHeight };
	mContext->CreateWindowDependentResource(InWidth, InHeight);
    CreateGui();
}


void Renderer::BaseRenderer::Update(float delta)
{
	UpdataFrameData();
	mRenderExecution->run(*mRenderFlow).wait();
}
       
void Renderer::BaseRenderer::LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene)
{
	mCurrentScene = InGameScene;
	entt::registry& sceneRegistery = mCurrentScene->GetRegistery();
    mLoadResourceFuture = std::async(std::launch::async, [&]() 
		{
			auto allStaticMeshComponents = sceneRegistery.view<ECS::StaticMeshComponent>();
			allStaticMeshComponents.each([this](auto entity, ECS::StaticMeshComponent& renderComponent) {
				LoadStaticMeshToGpu(renderComponent);
			});
			for (auto [textureName, textureData] : mCurrentScene->GetTextureMap())
			{
				LoadMaterial(textureName, textureData);
			}
		});
    mGui->SetCurrentScene(InGameScene);
}

void Renderer::BaseRenderer::CreateGui() 
{
    mGui = std::make_shared<Gui>(weak_from_this());
    mGui->CreateGui(mWindow);
}

void Renderer::BaseRenderer::CreateRenderTask()
{
	mRenderFlow = std::make_unique<tf::Taskflow>();
	mRenderExecution = std::make_unique<tf::Executor>();
	auto SkyboxPass = [this]()
		{
			//Render Scene
			auto lCurrentBackbufferIndex = mDeviceManager->GetCurrentFrameIndex();
			if (!mHasSkybox)
			{
				return;
			}
			auto frameDataIndex = lCurrentBackbufferIndex % SWAP_CHAIN_BUFFER_COUNT;
			TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetResource(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mGraphicsCmd->SetPipelineState(mSkybox->GetPipelineState());
			mGraphicsCmd->SetGraphicsRootSignature(mSkybox->GetRS());
			mGraphicsCmd->SetGraphicsRootConstantBufferView(0, mFrameDataGPU[frameDataIndex]->RootConstantBufferView());
			std::vector<ID3D12DescriptorHeap*> heaps = { g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap() };
			mGraphicsCmd->SetDescriptorHeaps((UINT)heaps.size(), heaps.data());
			mGraphicsCmd->SetGraphicsRootDescriptorTable(1, mSkyboxTexture->GetSRVGpu());
			mGraphicsCmd->OMSetRenderTargets(1, &mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetRTV(), true, nullptr);
			mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[lCurrentBackbufferIndex].GetRTV(), mColorRGBA, 1, &mRect);
			RenderObject(*mSkybox->GetStaticMeshComponent());
		};

	auto DepthOnlyPass = [this]()
		{
			mDeviceManager->BeginFrame();
			auto lCurrentBackbufferIndex = mDeviceManager->GetCurrentFrameIndex();
			auto frameDataIndex = lCurrentBackbufferIndex % SWAP_CHAIN_BUFFER_COUNT;


			mGraphicsCmdAllocator = mCmdManager->RequestAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, mGraphicsFenceValue);
			mGraphicsCmd->Reset(mGraphicsCmdAllocator, mPipelineStateDepthOnly);
			if (mIsFirstFrame)
			{
				FirstFrame();
				mIsFirstFrame = false;
			}
			//Update Frame Resource
			TransitState(mGraphicsCmd, mFrameDataGPU[frameDataIndex]->GetResource(),
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				D3D12_RESOURCE_STATE_COPY_DEST);
			mGraphicsCmd->CopyResource(mFrameDataGPU[frameDataIndex]->GetResource(),mFrameDataCPU[frameDataIndex]->GetResource());
			TransitState(mGraphicsCmd, mFrameDataGPU[frameDataIndex]->GetResource(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);


			//Setup RenderTarget
			TransitState(mGraphicsCmd, g_DisplayPlane[lCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			mGraphicsCmd->OMSetRenderTargets(0, nullptr, true, &mContext->GetDepthBuffer()->GetDSV());
			mGraphicsCmd->RSSetViewports(1, &mViewPort);
			mGraphicsCmd->RSSetScissorRects(1, &mRect);
			mGraphicsCmd->ClearDepthStencilView(mContext->GetDepthBuffer()->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 1, &mRect);

			//Set Resources
			mGraphicsCmd->SetGraphicsRootSignature(mColorPassRootSignature);
			mGraphicsCmd->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			mGraphicsCmd->SetGraphicsRootConstantBufferView(ROOT_PARA_FRAME_DATA_CBV, mFrameDataGPU[frameDataIndex]->RootConstantBufferView());
			auto vbview = mContext->GetVertexBuffer()->VertexBufferView();
			mGraphicsCmd->IASetVertexBuffers(0, 1, &vbview);
			auto ibview = mContext->GetIndexBuffer()->IndexBufferView();
			mGraphicsCmd->IASetIndexBuffer(&ibview);
			mGraphicsCmd->SetPipelineState(mPipelineStateDepthOnly);
			using namespace ECS;
			if (mCurrentScene && mCurrentScene->IsSceneReady())
			{
                auto& sceneRegistry = mCurrentScene->GetRegistery();
                auto renderEntities = sceneRegistry.view<StaticMeshComponent, TransformComponent>();
                //auto renderEntitiesCount = renderEntities.size();
                renderEntities.each([=](auto entity, auto& renderComponent, auto& transformComponent) {
                    auto modelMatrix = transformComponent.GetModelMatrix();
                    mGraphicsCmd->SetGraphicsRoot32BitConstants(ROOT_PARA_COMPONENT_DATA, 16, &modelMatrix, 0);
                    RenderObject(renderComponent);
                });

				//ShadowMap
				auto shadowMap = mContext->GetShadowMap();
				TransitState(mGraphicsCmd, shadowMap->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				mGraphicsCmd->SetPipelineState(mPipelineStateShadowMap);
				mGraphicsCmd->OMSetRenderTargets(0, nullptr, true, &shadowMap->GetDSV());
				mGraphicsCmd->ClearDepthStencilView(shadowMap->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 1, &mRect);
				//auto renderEntitiesCount = renderEntities.size();
				renderEntities.each([=](auto entity, auto& renderComponent, auto& transformComponent) {
					auto modelMatrix = transformComponent.GetModelMatrix();
					mGraphicsCmd->SetGraphicsRoot32BitConstants(ROOT_PARA_COMPONENT_DATA, 16, &modelMatrix, 0);
					RenderObject(renderComponent);
					});
				TransitState(mGraphicsCmd, shadowMap->GetResource(),D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

				
			}
		};

	auto ComputePass = [this]() 
		{
			
		};

	auto ColorPass = [this]()
		{
			auto lCurrentBackbufferIndex = mDeviceManager->GetCurrentFrameIndex();
			auto frameDataIndex = lCurrentBackbufferIndex % SWAP_CHAIN_BUFFER_COUNT;

			ID3D12CommandAllocator* cmdAllcator = mDeviceManager->GetCmdManager()->RequestAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, mComputeFenceValue);
			mComputeCmd->Reset(cmdAllcator, mLightCullPass);
			mComputeCmd->SetPipelineState(mLightCullPass);
			mComputeCmd->SetComputeRootSignature(mLightCullPassRootSignature);
			mComputeCmd->SetComputeRootConstantBufferView(0, mFrameDataGPU[frameDataIndex]->RootConstantBufferView());
			mComputeCmd->SetComputeRootShaderResourceView(1, mLightBuffer->GetGpuVirtualAddress());
			mComputeCmd->SetComputeRootUnorderedAccessView(2, mClusterBuffer->GetGpuVirtualAddress());
			mComputeCmd->Dispatch(CLUSTER_X, CLUSTER_Y, CLUSTER_Z);
			ID3D12CommandQueue* queue = mDeviceManager->GetCmdManager()->GetQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
			ID3D12CommandList* lCmds = { mComputeCmd };
			mComputeCmd->Close();
			queue->ExecuteCommandLists(1, &lCmds);
			queue->Signal(mComputeFence, mComputeFenceValue);
			mDeviceManager->GetCmdManager()->Discard(D3D12_COMMAND_LIST_TYPE_COMPUTE, cmdAllcator, mComputeFenceValue);
			mComputeFence->SetEventOnCompletion(mComputeFenceValue, mComputeFenceHandle);
			mComputeFenceValue++;

			//Render Scene
			WaitForSingleObject(mComputeFenceHandle, INFINITE);
#if 0
			mGraphicsCmd->SetPipelineState(mColorPassPipelineState);
#endif
			mGraphicsCmd->SetPipelineState(mColorPassPipelineState8XMSAA);
			mGraphicsCmd->SetGraphicsRootSignature(mColorPassRootSignature);
			std::vector<ID3D12DescriptorHeap*> heaps = { g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap() };
			mGraphicsCmd->SetDescriptorHeaps((UINT)heaps.size(), heaps.data());
			mGraphicsCmd->SetGraphicsRootConstantBufferView(ROOT_PARA_FRAME_DATA_CBV, mFrameDataGPU[frameDataIndex]->RootConstantBufferView());
			mGraphicsCmd->SetGraphicsRootDescriptorTable(ROOT_PARA_FRAME_SOURCE_TABLE, mLightBuffer->GetSRVGpu());
			//mGraphicsCmd->SetGraphicsRootConstantBufferView(3, mLightCullViewDataGpu->GetGpuVirtualAddress());
#if 0
			mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[lCurrentBackbufferIndex].GetRTV(), true, &mContext->GetDepthBuffer()->GetDSV_ReadOnly());
#endif
			mGraphicsCmd->OMSetRenderTargets(1, &mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetRTV(), true, &mContext->GetDepthBuffer()->GetDSV_ReadOnly());
			mGraphicsCmd->SetGraphicsRootDescriptorTable(ROOT_PARA_SHADOW_MAP, mContext->GetShadowMap()->GetDepthSRVGPU());
			using namespace ECS;
            if (mCurrentScene && mCurrentScene->IsSceneReady()) 
			{
                auto& sceneRegistry = mCurrentScene->GetRegistery();
                auto renderEntities = sceneRegistry.view<StaticMeshComponent, TransformComponent>();
                renderEntities.each([=](auto entity, auto& renderComponent, auto& transformComponent) {
                    auto modelMatrix = transformComponent.GetModelMatrix();
					
					//if (mTextureMap.find(renderComponent.NormalMap) == mTextureMap.end())
					//{
					//	mGraphicsCmd->SetGraphicsRootDescriptorTable(6, mTextureMap["defaultNormal"]->GetSRVGpu());
					//}
					//else
					//{
					//	mGraphicsCmd->SetGraphicsRootDescriptorTable(6, mTextureMap[renderComponent.NormalMap]->GetSRVGpu());
					//}
					OjbectData objData = {};
					objData.ModelMatrix = modelMatrix;
					objData.DiffuseColor = renderComponent.mBaseColor;
					constexpr int objSize = sizeof(OjbectData) / 4;
                    mGraphicsCmd->SetGraphicsRoot32BitConstants(ROOT_PARA_COMPONENT_DATA, objSize, &objData, 0);
					//Render 
					for (const auto& [matid,subMesh] : renderComponent.mSubMeshes)
					{
						auto subMeshTextureName = renderComponent.mMatTextureName[matid];

						if (mTextureMap.find(subMeshTextureName) == mTextureMap.end())
						{
							mGraphicsCmd->SetGraphicsRootDescriptorTable(ROOT_PARA_DIFFUSE_COLOR_TEXTURE, mTextureMap["defaultTexture"]->GetSRVGpu());
						}
						else
						{
							mGraphicsCmd->SetGraphicsRootDescriptorTable(ROOT_PARA_DIFFUSE_COLOR_TEXTURE, mTextureMap[subMeshTextureName]->GetSRVGpu());
						}

						mGraphicsCmd->DrawIndexedInstanced((UINT)subMesh.TriangleCount * 3, 1, renderComponent.StartIndexLocation + subMesh.IndexOffset,
							renderComponent.BaseVertexLocation, 0);
					}
				});
			}

			if (mUseToneMapping)
			{
				//Resolve MSAA RT to normal RT
				TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
				TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_RESOLVED)->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);
				mGraphicsCmd->ResolveSubresource(mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_RESOLVED)->GetResource(), 0, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetResource(), 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
				TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_RESOLVED)->GetResource(), D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				//Bloom

				auto sceneTex = mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_RESOLVED);
				auto blurHalf = mContext->GetRenderTarget(RenderTarget::BLUR_HALF);
				auto blurQuat = mContext->GetRenderTarget(RenderTarget::BLUR_QUAT);
				auto bloomRes = mContext->GetRenderTarget(RenderTarget::BLOOM_RES);


				// Bloom Pass 1 (scene -> blur1)
				ppBloomExtract->SetSourceTexture(sceneTex->GetSRVGPU(),sceneTex->GetResource());
				ppBloomExtract->SetBloomExtractParameter(mBloomThreshold);
				mGraphicsCmd->OMSetRenderTargets(1, &blurHalf->GetRTV(), FALSE, nullptr);
				SimpleMath::Viewport halfvp(mViewPort);
				halfvp.height /= 2.f;
				halfvp.width /= 2.f;
				mGraphicsCmd->RSSetViewports(1, halfvp.Get12());
				ppBloomExtract->Process(mGraphicsCmd);

				// Pass 2 (blur1 -> blur2)
				{
					auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(blurHalf->GetResource(),
						D3D12_RESOURCE_STATE_RENDER_TARGET,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0);
					mGraphicsCmd->ResourceBarrier(1, &barrier);
				}
				

				ppBloomBlur->SetSourceTexture(blurHalf->GetSRVGPU(), blurHalf->GetResource());
				ppBloomBlur->SetBloomBlurParameters(true, mBloomBlurKernelSize, mBloomBrightness);
				mGraphicsCmd->OMSetRenderTargets(1, &blurQuat->GetRTV(), FALSE, nullptr);
				ppBloomBlur->Process(mGraphicsCmd);


				// Pass 3 (blur2 -> blur1)
				{
					CD3DX12_RESOURCE_BARRIER barriers[2] =
					{
						CD3DX12_RESOURCE_BARRIER::Transition(blurHalf->GetResource(),
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						D3D12_RESOURCE_STATE_RENDER_TARGET, 0),
						CD3DX12_RESOURCE_BARRIER::Transition(blurQuat->GetResource(),
						D3D12_RESOURCE_STATE_RENDER_TARGET,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0),
					};
					mGraphicsCmd->ResourceBarrier(2, barriers);
				}
				ppBloomBlur->SetSourceTexture(blurQuat->GetSRVGPU(), blurQuat->GetResource());
				ppBloomBlur->SetBloomBlurParameters(false, mBloomBlurKernelSize, mBloomBrightness);
				mGraphicsCmd->OMSetRenderTargets(1, &blurHalf->GetRTV(), FALSE, nullptr);
				ppBloomBlur->Process(mGraphicsCmd);


				// Pass 4 (scene+blur1 -> rt)
				{
					auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(blurHalf->GetResource(),
						D3D12_RESOURCE_STATE_RENDER_TARGET,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0);
					mGraphicsCmd->ResourceBarrier(1, &barrier);
				}

				ppBloomCombine->SetSourceTexture(sceneTex->GetSRVGPU());
				ppBloomCombine->SetSourceTexture2(blurHalf->GetSRVGPU());
				ppBloomCombine->SetBloomCombineParameters(mbloomIntensity, mbaseIntensity, mbloomSaturation, mbloomBaseSaturation);
				mGraphicsCmd->OMSetRenderTargets(1, &bloomRes->GetRTV(), FALSE, nullptr);
				mGraphicsCmd->RSSetViewports(1, &mViewPort); 
				ppBloomCombine->Process(mGraphicsCmd);

				{
					CD3DX12_RESOURCE_BARRIER barriers[3] =
					{
						CD3DX12_RESOURCE_BARRIER::Transition(blurHalf->GetResource(),
							D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
							D3D12_RESOURCE_STATE_RENDER_TARGET, 0),
						CD3DX12_RESOURCE_BARRIER::Transition(blurQuat->GetResource(),
							D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
							D3D12_RESOURCE_STATE_RENDER_TARGET, 0),
						CD3DX12_RESOURCE_BARRIER::Transition(bloomRes->GetResource(),
							D3D12_RESOURCE_STATE_RENDER_TARGET,
							D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0)
					};
					mGraphicsCmd->ResourceBarrier(3, barriers);
				}




				//Tone Mapping
				ppToneMap->SetHDRSourceTexture(bloomRes->GetSRVGPU());
				mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[lCurrentBackbufferIndex].GetRTV(), true, nullptr);
				ppToneMap->SetExposure(mExposure);
				ppToneMap->Process(mGraphicsCmd);

				{
					auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(bloomRes->GetResource(),
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						D3D12_RESOURCE_STATE_RENDER_TARGET, 0);
					mGraphicsCmd->ResourceBarrier(1, &barrier);

				}
				

			}
			else
			{
				//Resolve MSAA RT to swap chain back buffer
				TransitState(mGraphicsCmd, g_DisplayPlane[lCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_DEST);
				TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
				mGraphicsCmd->ResolveSubresource(g_DisplayPlane[lCurrentBackbufferIndex].GetResource(), 0, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetResource(), 0, DXGI_FORMAT_R16G16B16A16_FLOAT);
				TransitState(mGraphicsCmd, g_DisplayPlane[lCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);
				TransitState(mGraphicsCmd, g_DisplayPlane[lCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
			}
			
		};

	auto GuiPass = [=] {
		auto lCurrentBackbufferIndex = mDeviceManager->GetCurrentFrameIndex();
		mGraphicsCmd->SetPipelineState(mColorPassPipelineState);
		mGraphicsCmd->OMSetRenderTargets(1, &g_DisplayPlane[lCurrentBackbufferIndex].GetRTV(), true, nullptr);
		mGui->BeginGui();
		mGui->Render();
		mGui->EndGui(mGraphicsCmd);
    };

	auto PostRender = [this]()
		{
			//Flush CmdList
			auto lCurrentBackbufferIndex = mDeviceManager->GetCurrentFrameIndex();
			TransitState(mGraphicsCmd, g_DisplayPlane[lCurrentBackbufferIndex].GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			mGraphicsCmd->Close();
			ID3D12CommandList* cmds[] = { mGraphicsCmd };
			ID3D12CommandQueue* graphicsQueue = mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
			graphicsQueue->ExecuteCommandLists(1, cmds);
			//Wait For cmd finished to reclaim cmd
			mCmdManager->Discard(D3D12_COMMAND_LIST_TYPE_DIRECT, mGraphicsCmdAllocator, mGraphicsFenceValue);
			mGraphicsFenceValue++;
			mDeviceManager->EndFrame();
		};
    auto [depthOnlyPass, skyboxpass, colorPass, postRender, computePass, guiPass ] =
            mRenderFlow->emplace(DepthOnlyPass, SkyboxPass, ColorPass, PostRender, ComputePass,GuiPass);
	skyboxpass.succeed(depthOnlyPass, computePass);
	colorPass.succeed(skyboxpass);
	guiPass.succeed(colorPass);
	postRender.succeed(guiPass);
}



void Renderer::BaseRenderer::CreateBuffers()
{

	for (auto i = 0 ; i < SWAP_CHAIN_BUFFER_COUNT ; ++i)
	{
		mFrameDataCPU[i] = std::make_shared<Resource::UploadBuffer>();
		mFrameDataCPU[i]->Create(L"FrameData", sizeof(FrameData));

		mFrameDataGPU[i] = std::make_unique<Resource::VertexBuffer>();
		mFrameDataGPU[i]->Create(L"FrameData", 1, sizeof(FrameData),D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		mFrameData[i].DirectionalLightColor = SimpleMath::Vector4(1.0f, 1.0f, 1.0f, 1.0);
		mFrameData[i].DirectionalLightDir = SimpleMath::Vector3(1.0, 10.0, 12.0);
		mFrameData[i].SunLightIntensity = 1.0;

	}

	mLightUploadBuffer = std::make_shared<Resource::UploadBuffer>();
	mLightUploadBuffer->Create(L"LightUploadBuffer", sizeof(ECS::LigthData) * mLights.size());

	float size = 65.0f;
	
	for (auto& light : mLights)
	{
		light.pos.x = ((float(rand()) / RAND_MAX) - 0.5f) * 2.0f;
		light.pos.y = float(rand()) / RAND_MAX;
		light.pos.z = ((float(rand()) / RAND_MAX) - 0.5f) * 2.0f;
	
		light.pos.x *= size;
		light.pos.y *= 15.0;
		light.pos.z *= size;
		light.pos.w = 1.0f;
	
		light.color.x = float(rand()) / RAND_MAX;
		light.color.y = float(rand()) / RAND_MAX;
		light.color.z = float(rand()) / RAND_MAX;
		light.radius_attenu.x = 55.0f;
		//light.radius_attenu.y = float(rand()) * 1.2f / RAND_MAX;
		light.radius_attenu.z = float(rand()) * 1.2f / RAND_MAX;
		light.radius_attenu.w = float(rand()) * 1.2f / RAND_MAX;
	}

	//mLights[0].pos = { 0.0, 1.0, 0.0, 1.0f };
	//mLights[0].radius_attenu = { 20.0, 0.0, 0.0, 1.0f };
	//mLights[0].color = {1.0f,0.0f,0.0f,1.0f};

	//mLights[1].pos = { -5.0, 0.0, 0.0, 1.0f };
	//mLights[1].radius_attenu = { 200.0, 0.0, 0.0, 1.0f };
	//mLights[1].color = { 0.0f,1.0f,0.0f,1.0f };
	//
	//mLights[3].pos = { 0.0, 0.0, 5.0, 1.0f };
	//mLights[3].radius_attenu = { 50.0, 0.0, 0.0, 1.0f };
	//mLights[3].color = { 1.0f,1.0f,0.0f,1.0f };
	//
	//mLights[2].pos = { 5.0, 0.0, 0.0, 1.0f };
	//mLights[2].radius_attenu = { 10.0, 0.0, 0.0, 1.0f };
	//mLights[2].color = { 1.0f,1.0f,1.0f,1.0f };
	

	//
	mLightUploadBuffer->UploadData<ECS::LigthData>(mLights);

	//mDummyBuffer = std::make_unique<Resource::StructuredBuffer>();
	//mDummyBuffer->Create(L"Dummy", 1, sizeof(uint8_t), nullptr);
	//mDummyBuffer->CreateSRV();

	//FrameResource Table
	mLightBuffer = std::make_unique<Resource::StructuredBuffer>();
	mLightBuffer->Create(L"LightBuffer", (UINT32)mLights.size(), sizeof(ECS::LigthData));

	mClusterBuffer = std::make_unique<Resource::StructuredBuffer>();
	mCLusters.resize(CLUSTER_X * CLUSTER_Y * CLUSTER_Z);
	mClusterBuffer->Create(L"ClusterBuffer", (UINT32)mCLusters.size(), sizeof(Cluster));
}

void Renderer::BaseRenderer::CreateTextures()
{
	std::shared_ptr<Resource::Texture> defaultTexture =  LoadMaterial("uvmap.png", "defaultTexture",L"Diffuse");
	std::shared_ptr<Resource::Texture> defaultNormalTexture = LoadMaterial("T_Flat_Normal.PNG", "defaultNormal",L"Normal");

}

std::shared_ptr<Renderer::Resource::Texture> Renderer::BaseRenderer::LoadMaterial(std::string_view InTextureName, std::string_view InMatName,const std::wstring& InDebugName)
{
	auto newTextureData = AssetLoader::gStbTextureLoader->LoadTextureFromFile(InTextureName);
	if (!newTextureData.has_value())
	{
		return nullptr;
	}

	std::shared_ptr<Resource::Texture> newTexture = std::make_shared<Resource::Texture>();
	auto RowPitchBytes = newTextureData.value()->mWidth * newTextureData.value()->mComponent;
	newTexture->Create2D(RowPitchBytes, newTextureData.value()->mWidth, newTextureData.value()->mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr);
	mBatchUploader->Begin(D3D12_COMMAND_LIST_TYPE_COPY);
	D3D12_SUBRESOURCE_DATA sourceData = {};
	sourceData.pData = newTextureData.value()->mdata;
	sourceData.RowPitch = RowPitchBytes;
	sourceData.SlicePitch = RowPitchBytes * newTextureData.value()->mHeight;
	mBatchUploader->Upload(newTexture->GetResource(), 0, &sourceData, 1);
	mBatchUploader->Transition(newTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	mBatchUploader->End(mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY));
	newTexture->GetResource()->SetName(InDebugName.c_str());
	if (!InMatName.empty())
	{
		mTextureMap[std::string(InMatName)] = newTexture;
	}
	else
	{                                                                                                                                                                                                                                           
		mTextureMap[std::filesystem::path(InTextureName).filename().string()] = newTexture;
	}
	return newTexture;

}

std::shared_ptr<Renderer::Resource::Texture> Renderer::BaseRenderer::LoadMaterial(std::string_view InTextureName, AssetLoader::TextureData* textureData, const std::wstring& InDebugName /*= L""*/)
{
	if (mTextureMap.find(InTextureName.data()) != mTextureMap.end())
	{
		return mTextureMap[InTextureName.data()];
	}
	std::shared_ptr<Resource::Texture> newTexture = std::make_shared<Resource::Texture>();

	//Empty Data,load dds texture in renderer
	if (!textureData->mdata)
	{
		//load and upload
		newTexture->CreateDDSFromFile(textureData->mFilePath,mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT), false);
	}
	else
	{
		auto RowPitchBytes = textureData->mWidth * textureData->mComponent;
		newTexture->Create2D(RowPitchBytes, textureData->mWidth, textureData->mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr);
		mBatchUploader->Begin(D3D12_COMMAND_LIST_TYPE_COPY);
		D3D12_SUBRESOURCE_DATA sourceData = {};
		sourceData.pData = textureData->mdata;
		sourceData.RowPitch = RowPitchBytes;
		sourceData.SlicePitch = RowPitchBytes * textureData->mHeight;
		mBatchUploader->Upload(newTexture->GetResource(), 0, &sourceData, 1);
		mBatchUploader->Transition(newTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mBatchUploader->End(mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY));
	}
	
	newTexture->GetResource()->SetName(InDebugName.c_str());
	if (!InTextureName.empty())
	{
		mTextureMap[std::string(InTextureName)] = newTexture;
	}
	else
	{
		mTextureMap[std::filesystem::path(InTextureName).filename().string()] = newTexture;
	}
	return newTexture;
}

std::unordered_map<std::string, std::shared_ptr<Renderer::Resource::Texture>>& Renderer::BaseRenderer::GetSceneTextureMap()
{
	return mTextureMap;
}

void Renderer::BaseRenderer::DepthOnlyPass(const ECS::StaticMeshComponent& InAsset)
{
	

}

void Renderer::BaseRenderer::CreateSkybox()
{
	mSkybox = std::make_unique<Skybox>();
	LoadStaticMeshToGpu(*mSkybox->GetStaticMeshComponent());

	mSkyboxTexture = std::make_shared<Resource::Texture>();
	auto skybox_bottom = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/bottom.jpg");
	auto skybox_top = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/top.jpg");
	auto skybox_front = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/front.jpg");
	auto skybox_back = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/back.jpg");
	auto skybox_left = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/left.jpg");
	auto skybox_right = AssetLoader::gStbTextureLoader->LoadTextureFromFile("skybox/right.jpg");

	Ensures(skybox_bottom.has_value());
	Ensures(skybox_top.has_value());
	Ensures(skybox_front.has_value());
	Ensures(skybox_back.has_value());
	Ensures(skybox_left.has_value());
	Ensures(skybox_right.has_value());
	std::vector<AssetLoader::TextureData*> textures =
	{
		skybox_right.value(),
		skybox_left.value(),
		skybox_top.value(),
		skybox_bottom.value(),
		skybox_front.value(),
		skybox_back.value()
	};

	auto RowPitchBytes = textures[0]->mWidth * textures[0]->mComponent;
	mSkyboxTexture->CreateCube(RowPitchBytes, textures[0]->mWidth, textures[0]->mHeight, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr);
	mBatchUploader->Begin(D3D12_COMMAND_LIST_TYPE_COPY);
	int i = 0;
	std::array<D3D12_SUBRESOURCE_DATA, 6> subResourceData;
	for (AssetLoader::TextureData* newTexture : textures)
	{
		D3D12_SUBRESOURCE_DATA& sourceData = subResourceData[i];
		sourceData.pData = newTexture->mdata;
		sourceData.RowPitch = RowPitchBytes;
		sourceData.SlicePitch = RowPitchBytes * newTexture->mHeight;
		++i;
	}
	mBatchUploader->Upload(mSkyboxTexture->GetResource(), 0, subResourceData.data(), subResourceData.size());
	//mBatchUploader->Transition(mSkyboxTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	mBatchUploader->End(mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_COPY));
}

void Renderer::BaseRenderer::InitPostProcess()
{
	using namespace DirectX::DX12;

	mGPUMemory = std::make_unique<GraphicsMemory>(g_Device);

	DirectX::RenderTargetState bloomState(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);

	ppBloomExtract = std::make_unique<BasicPostProcess>(g_Device, bloomState,
		BasicPostProcess::BloomExtract);

	ppBloomBlur = std::make_unique<BasicPostProcess>(g_Device, bloomState,
		BasicPostProcess::BloomBlur);

	ppBloomCombine = std::make_unique<DualPostProcess>(g_Device, bloomState,
		DualPostProcess::BloomCombine);

	RenderTargetState toneMapRTState(g_DisplayFormat,
		DXGI_FORMAT_UNKNOWN);
	ppToneMap = std::make_unique<ToneMapPostProcess>(g_Device, toneMapRTState,
		ToneMapPostProcess::ACESFilmic,
		ToneMapPostProcess::Linear);

	//RenderTargetState imageBlit(g_DisplayFormat,
	//	DXGI_FORMAT_D32_FLOAT);
	//mImageBlit = std::make_unique<BasicPostProcess>(g_Device, imageBlit, BasicPostProcess::Copy);
}

void Renderer::BaseRenderer::FirstFrame()
{
	TransitState(mGraphicsCmd, mContext->GetShadowMap()->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	TransitState(mGraphicsCmd, mLightUploadBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitState(mGraphicsCmd, mLightBuffer->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	mGraphicsCmd->CopyResource(mLightBuffer->GetResource(), mLightUploadBuffer->GetResource());
	TransitState(mGraphicsCmd, mLightBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	TransitState(mGraphicsCmd, mContext->GetDepthBuffer()->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_RESOLVED)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::BLUR_HALF)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::BLUR_QUAT)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	TransitState(mGraphicsCmd, mContext->GetRenderTarget(RenderTarget::BLOOM_RES)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

	for (size_t cubeFace = 0; cubeFace < 6; cubeFace++)
	{
		TransitState(mGraphicsCmd, mSkyboxTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, cubeFace);
	}

	for (auto& texture : mTextureMap)
	{
		TransitState(mGraphicsCmd, texture.second->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	auto gridParas = Utils::GetLightGridZParams(mDefaultCamera->GetFar(), mDefaultCamera->GetNear());
	mFrameData[0].LightGridZParams = SimpleMath::Vector4(gridParas.x,gridParas.y, gridParas.z,0.0f);
	mFrameData[0].ClipToView = mDefaultCamera->GetClipToView();
	mFrameData[0].ViewMatrix = mDefaultCamera->GetView();
	mFrameData[0].InvDeviceZToWorldZTransform = Utils::CreateInvDeviceZToWorldZTransform(mDefaultCamera->GetPrj(false));
}

void Renderer::BaseRenderer::PreRender()
{

}

void Renderer::BaseRenderer::PostRender()
{

}

void Renderer::BaseRenderer::CreatePipelineState()
{
	//Color Pass pipeline state
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lDesc = {};
	lDesc.pRootSignature = mColorPassRootSignature;
	lDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	lDesc.VS = Utils::ReadShader(L"ForwardVS.cso");
	lDesc.PS = Utils::ReadShader(L"ForwardPS.cso");
	lDesc.SampleMask = UINT_MAX;
	std::vector<D3D12_INPUT_ELEMENT_DESC> elements =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,	   0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

	};
	lDesc.InputLayout.NumElements = static_cast<UINT>(elements.size());
	lDesc.InputLayout.pInputElementDescs = elements.data();
	lDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	lDesc.NumRenderTargets = 1;
	lDesc.RTVFormats[0] = g_ColorBufferFormat;
	lDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	lDesc.SampleDesc.Count = 1;
	lDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	
	lDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	lDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mColorPassPipelineState));
	mColorPassPipelineState->SetName(L"mColorPassPipelineState");
	
	lDesc.RasterizerState.MultisampleEnable = true;
	lDesc.SampleDesc.Count = 8;
	lDesc.SampleDesc.Quality = 0;
	lDesc.RTVFormats[0] = g_ColorBufferFormat;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mColorPassPipelineState8XMSAA));
	mColorPassPipelineState8XMSAA->SetName(L"mColorPassPipelineState8XMSAA");

	//Depth Only pipeline state
	lDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	lDesc.NumRenderTargets = 0;
	lDesc.PS = {};
	lDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mPipelineStateDepthOnly));
	mPipelineStateDepthOnly->SetName(L"mPipelineStateDepthOnly");


	lDesc.VS = Utils::ReadShader(L"ShadowMap.cso");
	lDesc.SampleDesc.Count = 1;
	lDesc.RasterizerState.MultisampleEnable = false;
	lDesc.RasterizerState.DepthBias = -100;
	lDesc.RasterizerState.SlopeScaledDepthBias = -1.5f;
	lDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mPipelineStateShadowMap));
	mPipelineStateShadowMap->SetName(L"mPipelineStateShadowMap");

	//Compute:Light Cull Pass
	D3D12_COMPUTE_PIPELINE_STATE_DESC lightCullPassDesc = {};
	lightCullPassDesc.CS = Utils::ReadShader(L"LightCull.cso");
	lightCullPassDesc.pRootSignature = mLightCullPassRootSignature;
	g_Device->CreateComputePipelineState(&lightCullPassDesc, IID_PPV_ARGS(&mLightCullPass));
	mLightCullPass->SetName(L"mLightCullPass");
}

void Renderer::BaseRenderer::CreateRootSignature()
{
	//Color Pass RS
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

		D3D12_ROOT_PARAMETER frameDataCBV = {};
		frameDataCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		frameDataCBV.Descriptor.RegisterSpace = 0;
		frameDataCBV.Descriptor.ShaderRegister = 0;
		frameDataCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER frameResourceTable = {};
		frameResourceTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

		D3D12_DESCRIPTOR_RANGE light_buffer_range = {};
		light_buffer_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		light_buffer_range.NumDescriptors = 1;
		light_buffer_range.BaseShaderRegister = 1;
		light_buffer_range.RegisterSpace = 0;
		light_buffer_range.OffsetInDescriptorsFromTableStart = 0;

		D3D12_DESCRIPTOR_RANGE cluster_range = {};
		cluster_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		cluster_range.NumDescriptors = 1;
		cluster_range.BaseShaderRegister = 2;
		cluster_range.RegisterSpace = 0;
		//warning !!! skip two descriptors than used by counter buffer within uav buffer
		//offset is 3 = 2(counter buffer desc) + 1(light buffer desc)
		auto cluster_offset = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->CalcHandleOffset(mClusterBuffer->GetUAV());
		auto light_offset = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->CalcHandleOffset(mLightBuffer->GetSRV());
		cluster_range.OffsetInDescriptorsFromTableStart = cluster_offset - light_offset;
		

		std::vector<D3D12_DESCRIPTOR_RANGE> frame_resource_ranges = { light_buffer_range,cluster_range };
		frameResourceTable.DescriptorTable.NumDescriptorRanges = frame_resource_ranges.size();
		frameResourceTable.DescriptorTable.pDescriptorRanges = frame_resource_ranges.data();
		frameResourceTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER componentData = {};
        componentData.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        componentData.Constants.RegisterSpace = 0;
        componentData.Constants.ShaderRegister = 4;
		//RTS matrix
		constexpr int objDataSize = sizeof(OjbectData) / 4;
        componentData.Constants.Num32BitValues = objDataSize;
        componentData.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;



		D3D12_ROOT_PARAMETER diffuseColorTexture = {};
		diffuseColorTexture.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		D3D12_DESCRIPTOR_RANGE diffuseRange = {};
		//Texture table range
		diffuseRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		diffuseRange.NumDescriptors = 1;
		diffuseRange.BaseShaderRegister = 5;
		diffuseRange.RegisterSpace = 0;
		diffuseRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_DESCRIPTOR_RANGE normalRange = {};
		normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		normalRange.NumDescriptors = 1;
		normalRange.BaseShaderRegister = 6;
		normalRange.RegisterSpace = 0;
		normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_DESCRIPTOR_RANGE roughnessRange = {};
		roughnessRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		roughnessRange.NumDescriptors = 1;
		roughnessRange.BaseShaderRegister = 7;
		roughnessRange.RegisterSpace = 0;
		roughnessRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		std::vector<D3D12_DESCRIPTOR_RANGE> ranges = { diffuseRange,normalRange,roughnessRange };
		diffuseColorTexture.DescriptorTable.NumDescriptorRanges = (UINT)ranges.size();
		diffuseColorTexture.DescriptorTable.pDescriptorRanges = ranges.data();
		diffuseColorTexture.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_PARAMETER shadowMap = {};
		D3D12_DESCRIPTOR_RANGE shadowMapRange = {};
		shadowMap.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		shadowMapRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		shadowMapRange.NumDescriptors = 1;
		shadowMapRange.BaseShaderRegister = 8;
		shadowMapRange.RegisterSpace = 0;
		shadowMapRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		std::vector<D3D12_DESCRIPTOR_RANGE> shadowmapranges = { shadowMapRange };
		shadowMap.DescriptorTable.NumDescriptorRanges = (UINT)shadowmapranges.size();
		shadowMap.DescriptorTable.pDescriptorRanges = shadowmapranges.data();
		shadowMap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		std::vector<D3D12_ROOT_PARAMETER> parameters =
		{
			frameDataCBV,//0
			frameResourceTable,//1
            componentData,//2
			diffuseColorTexture,//3
			shadowMap//4
		};

		//Samplers
		D3D12_STATIC_SAMPLER_DESC texture2DSampler = CD3DX12_STATIC_SAMPLER_DESC(0);
		texture2DSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC shadowSampler = CD3DX12_STATIC_SAMPLER_DESC(0);
		shadowSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		shadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		shadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		shadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		shadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		shadowSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		shadowSampler.ShaderRegister = 1;

		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = { texture2DSampler,shadowSampler };
		//rootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		rootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), (UINT)samplers.size(), samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob* signature;
		ID3DBlob* error;
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mColorPassRootSignature));
	}
	
	//Light Cull Pass RS
	{
		CD3DX12_ROOT_SIGNATURE_DESC lightCullRootSignatureDesc;
		D3D12_ROOT_PARAMETER lightBuffer = {};
		lightBuffer.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		lightBuffer.Descriptor.RegisterSpace = 0;
		lightBuffer.Descriptor.ShaderRegister = 1;
		lightBuffer.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER clusterBuffer = {};
		clusterBuffer.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		clusterBuffer.Descriptor.RegisterSpace = 0;
		clusterBuffer.Descriptor.ShaderRegister = 2;
		clusterBuffer.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER lightCullViewData = {};
		lightCullViewData.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		lightCullViewData.Descriptor.RegisterSpace = 0;
		lightCullViewData.Descriptor.ShaderRegister = 0;
		lightCullViewData.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		std::vector<D3D12_ROOT_PARAMETER> parameters =
		{
			lightCullViewData,lightBuffer,clusterBuffer
		};
		lightCullRootSignatureDesc.Init((UINT)parameters.size(), parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
		
		ID3DBlob* signature;
		ID3DBlob* error;
		D3D12SerializeRootSignature(&lightCullRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		g_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mLightCullPassRootSignature));
	}
	
}

void Renderer::BaseRenderer::RenderObject(const ECS::StaticMeshComponent& InAsset)
{
	//Render 
	for (const auto& subMesh : InAsset.mSubMeshes)
	{
        mGraphicsCmd->DrawIndexedInstanced((UINT)subMesh.second.TriangleCount * 3, 1, InAsset.StartIndexLocation + subMesh.second.IndexOffset,
                                           InAsset.BaseVertexLocation, 0);
	}
}

void Renderer::BaseRenderer::TransitState(ID3D12GraphicsCommandList* InCmd, ID3D12Resource* InResource, D3D12_RESOURCE_STATES InBefore, D3D12_RESOURCE_STATES InAfter, UINT InSubResource)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.Subresource = InSubResource;
	barrier.Transition.StateBefore = InBefore;
	barrier.Transition.StateAfter = InAfter;
	barrier.Transition.pResource = InResource;
	InCmd->ResourceBarrier(1, &barrier);
}

void Renderer::BaseRenderer::UpdataFrameData()
{
	auto frameDataCpuIndex = mFrameIndexCpu % SWAP_CHAIN_BUFFER_COUNT;

	auto gridParas = Utils::GetLightGridZParams(mDefaultCamera->GetNear(), mDefaultCamera->GetFar());
	mFrameData[frameDataCpuIndex].PrjView = mDefaultCamera->GetPrjView();
	mFrameData[frameDataCpuIndex].View = mDefaultCamera->GetView();
	mFrameData[frameDataCpuIndex].Prj = mDefaultCamera->GetPrj();
	mFrameData[frameDataCpuIndex].ShadowViewMatrix = mShadowCamera->GetView();
	mFrameData[frameDataCpuIndex].ShadowViewPrjMatrix = mShadowCamera->GetPrjView();
	mFrameData[frameDataCpuIndex].NormalMatrix = mDefaultCamera->GetNormalMatrix();
	mFrameData[frameDataCpuIndex].DirectionalLightDir.x = mSunLightDir[0];
	mFrameData[frameDataCpuIndex].DirectionalLightDir.y = mSunLightDir[1];
	mFrameData[frameDataCpuIndex].DirectionalLightDir.z = mSunLightDir[2];
	mFrameData[frameDataCpuIndex].SunLightIntensity = mSunLightIntensity;
	mFrameData[frameDataCpuIndex].DirectionalLightDir.Normalize();
	mFrameData[frameDataCpuIndex].LightGridZParams.x = gridParas.x;
	mFrameData[frameDataCpuIndex].LightGridZParams.y = gridParas.y;
	mFrameData[frameDataCpuIndex].LightGridZParams.z = gridParas.z;
	mFrameData[frameDataCpuIndex].ViewSizeAndInvSize = SimpleMath::Vector4((float)mWidth, (float)mHeight, 1.0f / (float)mWidth, 1.0f / (float)mHeight);
	mFrameData[frameDataCpuIndex].ClipToView = mDefaultCamera->GetClipToView();
	mFrameData[frameDataCpuIndex].ViewMatrix = mDefaultCamera->GetView();
	mFrameData[frameDataCpuIndex].InvDeviceZToWorldZTransform = Utils::CreateInvDeviceZToWorldZTransform(mDefaultCamera->GetPrj(false));
	mFrameDataCPU[frameDataCpuIndex]->UpdataData<FrameData>(mFrameData[frameDataCpuIndex]);
	//Advance CPU Frame Index
	mFrameIndexCpu++;
}

void Renderer::BaseRenderer::OnGameSceneUpdated(std::shared_ptr<GAS::GameScene> InScene, std::span<entt::entity> InNewEntities)
{
	if (InScene == mCurrentScene)
	{
		auto& textureMap = mCurrentScene->GetTextureMap();
		for (auto [textureName,textureData] : textureMap)
		{
			LoadMaterial(textureName, textureData);
		}
	}
}

void Renderer::BaseRenderer::LoadStaticMeshToGpu(ECS::StaticMeshComponent& InComponent)
{
	auto& vertices = InComponent.mVertices;
	auto& indices = InComponent.mIndices;
	InComponent.BaseVertexLocation = mContext->GetVertexBufferCpu()->GetOffset();
	InComponent.StartIndexLocation = mContext->GetIndexBufferCpu()->GetOffset();
	mContext->UpdateDataToVertexBuffer(vertices);
	mContext->UpdateDataToIndexBuffer(indices);
}



