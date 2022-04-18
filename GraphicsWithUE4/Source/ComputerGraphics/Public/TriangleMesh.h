// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObjectInterface.h"
#include "TriangleMesh.generated.h"

UCLASS()
class UTriangle : public UObject, public IObjectInterface
{
	GENERATED_BODY()
private:
	class UStaticMeshDataComponent* MeshData;
	class ATriangleMesh* TriangleMesh;

	int32 FirstIndex;
	FVector MeshDataLocationOffset;

	// 三角形顶点
	FVector p0, p1, p2;
	// 法线
	FVector Normal;
	FVector e1, e2;
public:
	virtual FBounds3 GetBounds() const override;

	virtual FIntersection GetIntersection(const FLightRay& Ray, bool bDraw = false) const override;

	virtual void SetColor(FLinearColor Color) const override;

	FLinearColor GetColor() const;

	FVector GetEmit() const;
	
	float Area;

	void Init(ATriangleMesh* Mesh, class UStaticMeshDataComponent* Data, FVector LocationOffset, int32 First);

	FORCEINLINE float GetArea() const { return Area; };

	void Sample(UPARAM(ref) FIntersection& Position, UPARAM(ref) float& Pdf);
};

UCLASS()
class COMPUTERGRAPHICS_API ATriangleMesh : public AActor, public IObjectInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshDataComponent* MeshData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UProceduralMeshComponent* RenderMesh;

public:	
	// Sets default values for this actor's properties
	ATriangleMesh();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FLinearColor Emit; //发光强度

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector Kd;

	float Area;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	class UBVHTree* BvhTree;

	UPROPERTY()
	TArray<UTriangle*> Triangles;
public:
	UFUNCTION()
	void BuildTree();

	virtual FBounds3 GetBounds() const override;

	virtual FIntersection GetIntersection(const FLightRay& Ray, bool bDraw = false) const override;

	virtual void SetColor(FLinearColor Color) const {};

	virtual void ClearTriangleColor();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FVector GetEmit() const;
	virtual FVector GetEmit_Implementation() const override;

	FORCEINLINE float GetArea() const { return Area; };

	void Sample(UPARAM(ref) FIntersection& Position, UPARAM(ref) float& Pdf);

	UFUNCTION(BlueprintCallable)
	void SetMeshColor(FLinearColor Color) const;
};
