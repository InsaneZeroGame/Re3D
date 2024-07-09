#pragma once
#include "asset_loader.h"
#include <mutex>

namespace AssetLoader 
{




class FbxLoader final : public ModelAssetLoader {
public:
    ~FbxLoader();
    FbxLoader();
    
    //NoCopy
    //FbxLoader(const FbxLoader&) = delete;
    //FbxLoader(FbxLoader&&) = delete;
    //FbxLoader& operator=(const FbxLoader&) = delete;
    //FbxLoader& operator=(FbxLoader&&) = delete;

    std::vector<ECS::StaticMesh>& LoadAssetFromFile(std::string_view InFileName) override;
    //static FbxLoader& GetInstance();

private:
    class FbxManager* lSdkManager = NULL;
    class FbxScene* lScene = NULL;
    bool lResult;
    void DisplayContent(FbxScene* pScene);
    void DisplayContent(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxAMatrix& pParentGlobalPosition,
                        FbxPose* pPose);
    void DisplayMesh(FbxNode* pNode,const FbxAMatrix& globalPos);
    void DisplayPolygons(FbxMesh* pMesh);
    //inline static std::once_flag mCallOnce;
    //inline static FbxLoader* sLoader = nullptr;
    //ECS::StaticMesh mCurrentMesh;
    std::mutex mMeshMutex;
	std::mutex mTextureMapMutext;
    bool LoadStaticMesh(const FbxMesh* pMesh);
    void GetNodeGeometricTransform(FbxNode* pNode);
    FbxAMatrix GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, FbxPose* pPose,
                                 FbxAMatrix* pParentGlobalPosition);
    void LoadTextureMaterial(FbxScene* InScene, const std::string_view InFileName);

    };
}



