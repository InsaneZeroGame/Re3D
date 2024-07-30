#include "components.h"

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

 ECS::TransformComponent::TransformComponent(StaticMesh&& InMesh) : 
	mScale(InMesh.Scale), mTranslation(InMesh.Translation)
 {
    Translate(mTranslation);
    Scale(mScale);
    Rotate(InMesh.Rotation);
 }

const DirectX::SimpleMath::Matrix& ECS::TransformComponent::GetModelMatrix(bool UploadToGpu /*= true*/) 
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
