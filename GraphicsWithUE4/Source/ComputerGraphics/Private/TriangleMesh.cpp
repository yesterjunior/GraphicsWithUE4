// Fill out your copyright notice in the Description page of Project Settings.


#include "TriangleMesh.h"
#include "StaticMeshDataComponent.h"
#include "ProceduralMeshComponent.h"
#include "BVHTree.h"

FBounds3 UTriangle::GetBounds() const
{
	if (ensure(MeshData))
	{
		FVector Min = MeshData->Vertices[MeshData->Indices[FirstIndex]] + MeshDataLocationOffset;
		FVector Max = Min;
		Min = Min.ComponentMin(p1);
		Min = Min.ComponentMin(p2);
		Max = Max.ComponentMax(p1);
		Max = Max.ComponentMax(p2);
		return FBounds3(Min, Max);
	}
	return FBounds3();
}

FIntersection UTriangle::GetIntersection(const FLightRay& Ray, bool bDraw) const
{
	if (ensure(MeshData))
	{
		FIntersection HitResult;
		if (FVector::DotProduct(Ray.Direction, Normal) > 0)
		{
			return HitResult;
		}
		float u, v, t_tmp = 0;
		FVector s1 = FVector::CrossProduct(Ray.Direction, e2);
		float det = FVector::DotProduct(e1, s1);
		if (FMath::Abs(det) < EPSILON)
		{
			return HitResult;
		}

		float det_inv = 1. / det;
		FVector s0 = Ray.Origin - p0;
		u = FVector::DotProduct(s0, s1) * det_inv;
		if (u < 0 || u > 1)
		{
			return HitResult;
		}
		FVector s2 = FVector::CrossProduct(s0, e1);
		v = FVector::DotProduct(Ray.Direction, s2) * det_inv;
		if (v < 0 || u + v > 1)
		{
			return HitResult;
		}
		t_tmp = FVector::DotProduct(e2, s2) * det_inv;

		if (t_tmp >= 0 && IsValid(TriangleMesh))
		{
			HitResult.bBlockingHit = true;
			HitResult.Coords = p0 * (1 - u - v) + p1 * u + p2 * v;
			HitResult.Normal = Normal;
			HitResult.Distance = FMath::Sqrt(FVector::DotProduct(Ray(t_tmp) - Ray.Origin, Ray(t_tmp) - Ray.Origin));
			HitResult.Emit = GetEmit();
			HitResult.Kd = TriangleMesh->Kd;
			HitResult.Object.SetObject(const_cast<UTriangle *>(this));
		}
		return HitResult;
	}
	return IObjectInterface::GetIntersection(Ray);
}

FLinearColor UTriangle::GetColor() const
{
	return FLinearColor::Gray;
}

FVector UTriangle::GetEmit() const
{
	if (IsValid(TriangleMesh))
		return TriangleMesh->GetEmit();
	else
		return FVector::ZeroVector;
}

void UTriangle::Init(ATriangleMesh* Mesh, class UStaticMeshDataComponent* Data, FVector LocationOffset, int32 First)
{
	TriangleMesh = Mesh;
	MeshData = Data;
	MeshDataLocationOffset = LocationOffset;
	FirstIndex = First;

	p0 = MeshData->Vertices[MeshData->Indices[FirstIndex]] + MeshDataLocationOffset;
	p1 = MeshData->Vertices[MeshData->Indices[FirstIndex + 1]] + MeshDataLocationOffset;
	p2 = MeshData->Vertices[MeshData->Indices[FirstIndex + 2]] + MeshDataLocationOffset;

	e1 = p1 - p0;
	e2 = p2 - p0;
	Normal = FVector::CrossProduct(e2, e1).GetSafeNormal();
	Area = FVector::CrossProduct(e1, e2).Size() * 0.5f;
}

void UTriangle::Sample(FIntersection& Position, float& Pdf)
{
	float x = FMath::Sqrt(FMath::FRand());
	float y = FMath::FRand();
	Position.Coords = p0 * (1.0f - x) + p1 * (x * (1.0f - y)) + p2 * (x * y);
	Position.Normal = Normal;
	Pdf = 1.0f / Area;
}

void UTriangle::SetColor(FLinearColor Color) const
{
	if (ensure(MeshData))
	{
		MeshData->Colors[MeshData->Indices[FirstIndex]] = Color;
		MeshData->Colors[MeshData->Indices[FirstIndex + 1]] = Color;
		MeshData->Colors[MeshData->Indices[FirstIndex + 2]] = Color;
	}
}

// Sets default values
ATriangleMesh::ATriangleMesh()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RenderMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RenderMesh"));
	RenderMesh->SetupAttachment(RootComponent);
	MeshData = CreateDefaultSubobject<UStaticMeshDataComponent>(TEXT("MeshData"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshObject(TEXT("StaticMesh'/Game/HW06/bunny.bunny'"));
	if (StaticMeshObject.Succeeded()) {
		MeshData->Mesh = StaticMeshObject.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialObject(TEXT("Material'/Game/HW06/BunnyMaterial.BunnyMaterial'"));
	if (MaterialObject.Succeeded()) {
		MeshData->Material = MaterialObject.Object;
	}
}

// Called when the game starts or when spawned
void ATriangleMesh::BeginPlay()
{
	Super::BeginPlay();
	MeshData->OnMeshReady.AddDynamic(this, &ATriangleMesh::BuildTree);
	MeshData->RenderMesh(RenderMesh);
}

void ATriangleMesh::BuildTree()
{
	BvhTree = NewObject<UBVHTree>();
	if (ensure(MeshData->Vertices.Num()))
	{
		TArray<IObjectInterface*> Objects;
		Area = 0;
		for (int32 i = 0; i < MeshData->Indices.Num(); i += 3)
		{
			UTriangle* Triangle = NewObject<UTriangle>();
			Triangle->Init(this, MeshData, RenderMesh->GetComponentLocation(), i);
			Area += Triangle->GetArea();
			Triangles.Add(Triangle);
			Objects.Add(Cast<IObjectInterface>(Triangle));
		}
		BvhTree->BuildTree(Objects);
	}
}

FBounds3 ATriangleMesh::GetBounds() const
{
	if (ensure(MeshData->IsReady()))
	{
		return FBounds3(MeshData->Min + RenderMesh->GetComponentLocation(), MeshData->Max + RenderMesh->GetComponentLocation());
	}
	return FBounds3();
}

FIntersection ATriangleMesh::GetIntersection(const FLightRay& Ray, bool bDraw) const
{
	bool IsNeg[] = { Ray.Direction.X > 0, Ray.Direction.Y > 0, Ray.Direction.Z > 0 };
	if (GetBounds().IntersectP(Ray, Ray.DirectionInv, IsNeg) && BvhTree)
	{
		FIntersection Intersection = BvhTree->Intersect(Ray, bDraw);
		if (bDraw)
		{
			MeshData->RenderMesh();
		}
		return Intersection;
	}
	return FIntersection();
}

void ATriangleMesh::ClearTriangleColor()
{
	if (IsValid(BvhTree))
		BvhTree->ClearTriangleColor();
}

FVector ATriangleMesh::GetEmit_Implementation() const
{
	return Emit;
}

void ATriangleMesh::SetMeshColor(FLinearColor Color) const
{
	for (UTriangle* Triangle : Triangles)
	{
		Triangle->SetColor(Color);
	}
	MeshData->RenderMesh();
}

void ATriangleMesh::Sample(FIntersection& Position, float& Pdf)
{
	if (IsValid(BvhTree))
	{
		BvhTree->Sample(Position, Pdf);
		Position.Emit = GetEmit();
	}
}
