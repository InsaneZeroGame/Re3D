#include "renderer.h"
#include "renderer_context.h"
#include "obj_model_loader.h"
#include "utility.h"
#include <camera.h>
#include "components.h"
#include "renderer_context.h"
#include "PostProcess.h"
#include "GraphicsMemory.h"
#include "gui.h"

Renderer::ClusterForwardRenderer::ClusterForwardRenderer():
	BaseRenderer(),
	mIsFirstFrame(true),
	mComputeFence(nullptr),
	mGraphicsCmd(nullptr),
	mSkyboxPass(nullptr),
	mComputeFenceValue(1),
	mContext(std::make_shared<RendererContext>(mCmdManager))
{
	Ensures(AssetLoader::gStbTextureLoader);
	Ensures(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mComputeFence)) == S_OK);
	mComputeFenceHandle = CreateEvent(nullptr, false, false, nullptr);
	mGraphicsCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	mComputeCmd = mCmdManager->AllocateCmdList(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	
	CreateTextures();
	CreateBuffers();
	CreateRootSignature();
	CreatePipelineState();
	CreateRenderTask();
	mSkyboxPass = std::make_unique<SkyboxPass>(mContext);
	mLightCullPass = std::make_unique<LightCullPass>(mContext);
	InitPostProcess();
}

Renderer::ClusterForwardRenderer::~ClusterForwardRenderer()
{
	
}


void Renderer::ClusterForwardRenderer::SetTargetWindowAndCreateSwapChain(HWND InWindow, int InWidth, int InHeight)
{
	BaseRenderer::SetTargetWindowAndCreateSwapChain(InWindow, InWidth, InHeight);
	mContext->CreateWindowDependentResource(InWidth, InHeight);
	mGui = std::make_shared<Gui>(weak_from_this());
	mGui->CreateGui(mWindow);
	PrepairForRendering();
}


void Renderer::ClusterForwardRenderer::Update(float delta)
{
	UpdataFrameData();
	mRenderExecution->run(*mRenderFlow).wait();
}
       
void Renderer::ClusterForwardRenderer::LoadGameScene(std::shared_ptr<GAS::GameScene> InGameScene)
{
	BaseRenderer::LoadGameScene(InGameScene);
    mGui->SetCurrentScene(InGameScene);
}

std::shared_ptr<Renderer::RendererContext> Renderer::ClusterForwardRenderer::GetContext()
{
	return mContext;
}

void Renderer::ClusterForwardRenderer::CreateRenderTask()
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
			mSkyboxPass->SetRenderPassStates(mGraphicsCmd);
			mGraphicsCmd->SetGraphicsRootConstantBufferView(0, mFrameDataGPU[frameDataIndex]->RootConstantBufferView());
			std::vector<ID3D12DescriptorHeap*> heaps = { g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetDescHeap() };
			mGraphicsCmd->SetDescriptorHeaps((UINT)heaps.size(), heaps.data());
			mGraphicsCmd->OMSetRenderTargets(1, &mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetRTV(), true, nullptr);
			mGraphicsCmd->ClearRenderTargetView(g_DisplayPlane[lCurrentBackbufferIndex].GetRTV(), mColorRGBA, 1, &mRect);
			mSkyboxPass->RenderScene(mGraphicsCmd);
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
                    DrawObject(renderComponent);
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
					DrawObject(renderComponent);
					});
				TransitState(mGraphicsCmd, shadowMap->GetResource(),D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

				
			}
		};

	auto ColorPass = [this]()
		{
			auto lCurrentBackbufferIndex = mDeviceManager->GetCurrentFrameIndex();
			auto frameDataIndex = lCurrentBackbufferIndex % SWAP_CHAIN_BUFFER_COUNT;

			ID3D12CommandAllocator* cmdAllcator = mDeviceManager->GetCmdManager()->RequestAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, mComputeFenceValue);
			mComputeCmd->Reset(cmdAllcator, nullptr);
			mLightCullPass->SetRenderPassStates(mComputeCmd);
			mComputeCmd->SetComputeRootConstantBufferView(0, mFrameDataGPU[frameDataIndex]->RootConstantBufferView());
			mComputeCmd->SetComputeRootShaderResourceView(1, mLightBuffer->GetGpuVirtualAddress());
			mComputeCmd->SetComputeRootUnorderedAccessView(2, mClusterBuffer->GetGpuVirtualAddress());
			mLightCullPass->RenderScene(mComputeCmd);
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
					
					
					OjbectData objData = {};
					objData.ModelMatrix = modelMatrix;
					objData.DiffuseColor = renderComponent.mBaseColor;
					constexpr int objSize = sizeof(OjbectData) / 4;
                    mGraphicsCmd->SetGraphicsRoot32BitConstants(ROOT_PARA_COMPONENT_DATA, objSize, &objData, 0);
					//Render 
					for (const auto& [matid,subMesh] : renderComponent.mSubMeshes)
					{
						auto subMeshTextureName = renderComponent.mMatBaseColorName[matid];
						auto subMeshNormalMapName = renderComponent.mMatNormalMapName[matid];

						if (mTextureMap.find(subMeshTextureName) == mTextureMap.end())
						{
							mGraphicsCmd->SetGraphicsRootDescriptorTable(ROOT_PARA_DIFFUSE_COLOR_TEXTURE, mTextureMap["defaultTexture"]->GetSRVGpu());
						}
						else
						{
							mGraphicsCmd->SetGraphicsRootDescriptorTable(ROOT_PARA_DIFFUSE_COLOR_TEXTURE, mTextureMap[subMeshTextureName]->GetSRVGpu());
						}

						if (mTextureMap.find(subMeshNormalMapName) == mTextureMap.end())
						{
							mGraphicsCmd->SetGraphicsRootDescriptorTable(ROOT_PARA_NORMAL_MAP_TEXTURE, mTextureMap["defaultNormal"]->GetSRVGpu());
						}
						else
						{
							mGraphicsCmd->SetGraphicsRootDescriptorTable(ROOT_PARA_NORMAL_MAP_TEXTURE, mTextureMap[subMeshNormalMapName]->GetSRVGpu());
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
    auto [depthOnlyPass, skyboxpass, colorPass, postRender, guiPass ] =
            mRenderFlow->emplace(DepthOnlyPass, SkyboxPass, ColorPass, PostRender,GuiPass);
	skyboxpass.succeed(depthOnlyPass);
	colorPass.succeed(skyboxpass);
	guiPass.succeed(colorPass);
	postRender.succeed(guiPass);
}



void Renderer::ClusterForwardRenderer::CreateBuffers()
{
	BaseRenderer::CreateBuffers();
	mClusterBuffer = std::make_unique<Resource::StructuredBuffer>();
	mCLusters.resize(CLUSTER_X * CLUSTER_Y * CLUSTER_Z);
	mClusterBuffer->Create(L"ClusterBuffer", (UINT32)mCLusters.size(), sizeof(Cluster));

}

void Renderer::ClusterForwardRenderer::CreateTextures()
{
	std::shared_ptr<Resource::Texture> defaultTexture =  LoadMaterial("uvmap.png", "defaultTexture",L"Diffuse");
	std::shared_ptr<Resource::Texture> defaultNormalTexture = LoadMaterial("uvmap.png", "defaultNormal",L"Normal");
}


void Renderer::ClusterForwardRenderer::DepthOnlyPass(const ECS::StaticMeshComponent& InAsset)
{
	

}

void Renderer::ClusterForwardRenderer::InitPostProcess()
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

void Renderer::ClusterForwardRenderer::FirstFrame()
{
	BaseRenderer::FirstFrame();
}

void Renderer::ClusterForwardRenderer::PreRender()
{

}

void Renderer::ClusterForwardRenderer::PostRender()
{

}

void Renderer::ClusterForwardRenderer::CreatePipelineState()
{
	//Color Pass pipeline state
	D3D12_GRAPHICS_PIPELINE_STATE_DESC lDesc = {};
	lDesc.pRootSignature = mColorPassRootSignature;
	lDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	lDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	lDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	lDesc.VS = Utils::ReadShader("ForwardVS.hlsl","main","vs_6_5");
	lDesc.PS = Utils::ReadShader("ForwardPS.hlsl", "main", "ps_6_5");
	lDesc.SampleMask = UINT_MAX;
	std::vector<D3D12_INPUT_ELEMENT_DESC> elements =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,	   0, 52, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

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


	lDesc.VS = Utils::ReadShader("ShadowMap.hlsl", "main", "vs_6_5");
	lDesc.SampleDesc.Count = 1;
	lDesc.RasterizerState.MultisampleEnable = false;
	lDesc.RasterizerState.DepthBias = -100;
	lDesc.RasterizerState.SlopeScaledDepthBias = -1.5f;
	lDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	g_Device->CreateGraphicsPipelineState(&lDesc, IID_PPV_ARGS(&mPipelineStateShadowMap));
	mPipelineStateShadowMap->SetName(L"mPipelineStateShadowMap");
}

void Renderer::ClusterForwardRenderer::CreateRootSignature()
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

		D3D12_DESCRIPTOR_RANGE roughnessRange = {};
		roughnessRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		roughnessRange.NumDescriptors = 1;
		roughnessRange.BaseShaderRegister = 7;
		roughnessRange.RegisterSpace = 0;
		roughnessRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		std::vector<D3D12_DESCRIPTOR_RANGE> diffuseRanges = { diffuseRange,roughnessRange };
		diffuseColorTexture.DescriptorTable.NumDescriptorRanges = (UINT)diffuseRanges.size();
		diffuseColorTexture.DescriptorTable.pDescriptorRanges = diffuseRanges.data();
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

		D3D12_ROOT_PARAMETER normalMapTexture = {};
		normalMapTexture.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		D3D12_DESCRIPTOR_RANGE normalRange = {};
		normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		normalRange.NumDescriptors = 1;
		normalRange.BaseShaderRegister = 6;
		normalRange.RegisterSpace = 0;
		normalRange.OffsetInDescriptorsFromTableStart = 0;
		std::vector<D3D12_DESCRIPTOR_RANGE> normalMapRanges = { normalRange };
		normalMapTexture.DescriptorTable.NumDescriptorRanges = (UINT)normalMapRanges.size();
		normalMapTexture.DescriptorTable.pDescriptorRanges = normalMapRanges.data();
		normalMapTexture.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


		std::vector<D3D12_ROOT_PARAMETER> parameters =
		{
			frameDataCBV,//0
			frameResourceTable,//1
            componentData,//2
			diffuseColorTexture,//3
			shadowMap,//4,
			normalMapTexture,
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
}



void Renderer::ClusterForwardRenderer::UpdataFrameData()
{
	BaseRenderer::UpdataFrameData();
}

void Renderer::ClusterForwardRenderer::OnGameSceneUpdated(std::shared_ptr<GAS::GameScene> InScene, std::span<entt::entity> InNewEntities)
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

void Renderer::ClusterForwardRenderer::DrawObject(const ECS::StaticMeshComponent& InAsset)
{
	//Render 
	for (const auto& subMesh : InAsset.mSubMeshes)
	{
		mGraphicsCmd->DrawIndexedInstanced((UINT)subMesh.second.TriangleCount * 3, 1, InAsset.StartIndexLocation + subMesh.second.IndexOffset,
			InAsset.BaseVertexLocation, 0);
	}
}

void Renderer::ClusterForwardRenderer::PrepairForRendering()
{
	BaseRenderer::PrepairForRendering();
	mCmdManager->AllocateCmdAndFlush(D3D12_COMMAND_LIST_TYPE_DIRECT, [=](ID3D12GraphicsCommandList* lcmd,ID3D12CommandAllocator* allocator)
		{
			TransitState(lcmd, mContext->GetShadowMap()->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			TransitState(lcmd, mContext->GetDepthBuffer()->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			TransitState(lcmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_MSAA)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
			TransitState(lcmd, mContext->GetRenderTarget(RenderTarget::COLOR_OUTPUT_RESOLVED)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			TransitState(lcmd, mContext->GetRenderTarget(RenderTarget::BLUR_HALF)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			TransitState(lcmd, mContext->GetRenderTarget(RenderTarget::BLUR_QUAT)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			TransitState(lcmd, mContext->GetRenderTarget(RenderTarget::BLOOM_RES)->GetResource(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

			for (size_t cubeFace = 0; cubeFace < 6; cubeFace++)
			{
				TransitState(lcmd, mSkyboxPass->GetSkyBoxTexture(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, cubeFace);
			}

			for (auto& texture : mTextureMap)
			{
				TransitState(lcmd, texture.second->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
		});


}


