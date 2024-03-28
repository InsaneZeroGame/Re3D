#include "components.h"

ECS::System::System()
{

}

ECS::System::~System()
{

}

ECS::StaticMeshComponent::StaticMeshComponent(std::vector<Renderer::Vertex>&& Vertices, std::vector<int>&& Indices):
	mVertices(Vertices),
	mIndices(Indices),
	mIndexCount(Indices.size()),
	mVertexCount(Vertices.size()),
	StartIndexLocation(0),
	BaseVertexLocation(0)
{

}

ECS::StaticMeshComponent::StaticMeshComponent(StaticMesh&& InMesh):
mVertices(InMesh.mVertices),
mIndices(InMesh.mIndices),
mIndexCount(InMesh.mIndices.size()),
mVertexCount(InMesh.mVertices.size()),
StartIndexLocation(0),
BaseVertexLocation(0)
{

}
