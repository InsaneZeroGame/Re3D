#include "components.h"
#include <span>
#include <d3d12.h>

ECS::System::System()
{

}

ECS::System::~System()
{

}

ECS::StaticMeshComponent::StaticMeshComponent(StaticMesh&& InMesh):
mVertices(InMesh.mVertices),
mIndices(InMesh.mIndices),
mIndexCount(InMesh.mIndices.size()),
mVertexCount(InMesh.mVertices.size()),
StartIndexLocation(0),
BaseVertexLocation(0),
mSubMeshes(InMesh.mSubmeshMap),
mName(InMesh.mName),
mMatBaseColorName(InMesh.mMatBaseColorName),
mMatNormalMapName(InMesh.mMatNormalMapName),
mBaseColor(InMesh.mDiffuseColor)
{
	
}

HRESULT ECS::StaticMeshComponent::ConvertToMeshlets(size_t maxVerticesPerMeshlet, size_t maxIndicesPerMeshlet)
{
    // Convert vertices to DirectXMesh format
    mMeshletsVerticesPosition.resize(mVertices.size());
    for (size_t i = 0; i < mVertices.size(); ++i) {
        mMeshletsVerticesPosition[i].x = mVertices[i].pos[0];
        mMeshletsVerticesPosition[i].y = mVertices[i].pos[1];
        mMeshletsVerticesPosition[i].z = mVertices[i].pos[2];
    }

    // Generate meshlets using DirectXMesh
    std::vector<uint8_t> uniqueVertexIB;
    
    std::vector<DirectX::Meshlet> lMeshlets;


    HRESULT hr = DirectX::ComputeMeshlets(
        mIndices.data(),
        mIndices.size()/3,
        mMeshletsVerticesPosition.data(),
        mMeshletsVerticesPosition.size(),
        nullptr,
        lMeshlets,
        uniqueVertexIB,
        mMeshletPrimditives,
        maxVerticesPerMeshlet,
        maxIndicesPerMeshlet
    );
	auto meshlet_thread_group_size = lMeshlets.size()  / MAX_MESHLET_PER_THREAD_GROUP + 1;
	mMeshlets.resize(meshlet_thread_group_size);
    for (size_t i = 0; i < meshlet_thread_group_size; i++)
    {
        for (size_t j = 0; j < MAX_MESHLET_PER_THREAD_GROUP; j++)
        {
            if (i * MAX_MESHLET_PER_THREAD_GROUP + j < lMeshlets.size())
            {
                mMeshlets[i][j] = lMeshlets[i * MAX_MESHLET_PER_THREAD_GROUP + j];
            }
        }
    }

    size_t vertIndices = uniqueVertexIB.size() / sizeof(uint32_t);
    mMeshletsIndices = std::vector<uint32_t>(reinterpret_cast<uint32_t*>(uniqueVertexIB.data()),
        reinterpret_cast<uint32_t*>(uniqueVertexIB.data() + uniqueVertexIB.size()));

    if (FAILED(hr)) {
        // Handle error
        return hr;
    }
    return S_OK;
}

 ECS::TransformComponent::TransformComponent(StaticMesh&& InMesh) : 
	mScale(InMesh.Scale), mTranslation(InMesh.Translation)
 {
    Translate(mTranslation);
    Scale(mScale);
    Rotate(InMesh.Rotation);
 }

const DirectX::SimpleMath::Matrix ECS::TransformComponent::GetModelMatrix(bool UploadToGpu /*= true*/) 
{
    //Todo::Move calc to update data.
    mMat =
		DirectX::SimpleMath::Matrix::CreateScale(mScale)*
        DirectX::SimpleMath::Matrix::CreateFromAxisAngle(X_AXIS, mRotation.x)*
        DirectX::SimpleMath::Matrix::CreateFromAxisAngle(Y_AXIS, mRotation.y)*
        DirectX::SimpleMath::Matrix::CreateFromAxisAngle(Z_AXIS, mRotation.z)*
        DirectX::SimpleMath::Matrix::CreateTranslation(mTranslation);

    return UploadToGpu ? mMat.Transpose() : mMat;
}

 void ECS::TransformComponent::Translate(const DirectX::SimpleMath::Vector3& InTranslate) {

    mTranslation = InTranslate;
}

void ECS::TransformComponent::Scale(const DirectX::SimpleMath::Vector3& InScale) {
    mScale = InScale;
}

void ECS::TransformComponent::Rotate(const DirectX::SimpleMath::Vector3& InAngles) {
    mRotation = DirectX::SimpleMath::Vector3(InAngles.x, InAngles.y, InAngles.z) * DirectX::XM_PI / 180.0f;
}

DirectX::SimpleMath::Vector3& ECS::TransformComponent::GetTranslate() 
{
    return  mTranslation;
}

DirectX::SimpleMath::Vector3& ECS::TransformComponent::GetScale() {
    return mScale;
}

DirectX::SimpleMath::Vector3& ECS::TransformComponent::GetRotation() {
    return mRotation;
}
