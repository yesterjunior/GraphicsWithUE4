// Fill out your copyright notice in the Description page of Project Settings.


#include "StaticMeshDataComponent.h"
#include "RawMesh/Public/RawMesh.h"
#include "ProceduralMeshComponent.h"

// Sets default values for this component's properties
UStaticMeshDataComponent::UStaticMeshDataComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	Min = FVector(TNumericLimits<float>::Max());
	Max = FVector(-TNumericLimits<float>::Max());
	bReady = false;
	// ...
}

void UStaticMeshDataComponent::RenderMesh(UProceduralMeshComponent* InMesh)
{
	ProceduralMesh = InMesh;
	RenderMesh();
}

void UStaticMeshDataComponent::RenderMesh()
{
	if (bReady)
	{
		TArray<FVector> Normals;
		TArray<FVector2D> UV0;
		TArray<FProcMeshTangent> Tangents;
		ProceduralMesh->UpdateMeshSection_LinearColor(0, Vertices, Normals, UV0, Colors, Tangents);
	}
	else if (IsValid(ProceduralMesh) && Indices.Num() > 0)
	{
		TArray<FVector> Normals;
		TArray<FVector2D> UV0;
		TArray<FProcMeshTangent> Tangents;
		ProceduralMesh->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UV0, Colors, Tangents, false);
		if (Material)
		{
			ProceduralMesh->SetMaterial(0, Material);
		}
		bReady = true;
		OnMeshReady.Broadcast();
	}
}

void UStaticMeshDataComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Mesh))
	{
#if WITH_EDITOR
		FStaticMeshLODResources& Resource = Mesh->RenderData->LODResources[0];
		Vertices.AddUninitialized(Resource.VertexBuffers.PositionVertexBuffer.GetNumVertices());
		Colors.AddUninitialized(Vertices.Num());
		TArray<uint32> _Indices;
		Resource.IndexBuffer.GetCopy(_Indices);
		Indices.Append(_Indices);
		for (int32 i = 0; i < Indices.Num(); i += 3)
		{
			Vertices[Indices[i]] = Resource.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[i]);
			Colors[Indices[i]] = FLinearColor::Gray;
			Min = Min.ComponentMin(Vertices[Indices[i]]);
			Max = Max.ComponentMax(Vertices[Indices[i]]);
			Vertices[Indices[i + 1]] = Resource.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[i + 1]);
			Colors[Indices[i + 1]] = FLinearColor::Gray;
			Min = Min.ComponentMin(Vertices[Indices[i + 1]]);
			Max = Max.ComponentMax(Vertices[Indices[i + 1]]);
			Vertices[Indices[i + 2]] = Resource.VertexBuffers.PositionVertexBuffer.VertexPosition(Indices[i + 2]);
			Colors[Indices[i + 2]] = FLinearColor::Gray;
			Min = Min.ComponentMin(Vertices[Indices[i + 2]]);
			Max = Max.ComponentMax(Vertices[Indices[i + 2]]);
		}
		RenderMesh();
#endif
	}
}