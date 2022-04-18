// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaticMeshDataComponent.generated.h"

class UStaticMeshDataComponent;
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE(FOnMeshReadySignature, UStaticMeshDataComponent, OnMeshReady);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMPUTERGRAPHICS_API UStaticMeshDataComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStaticMeshDataComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UStaticMesh* Mesh;

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> Indices;

	UPROPERTY(BlueprintReadOnly)
	TArray<FLinearColor> Colors;

	UPROPERTY(BlueprintReadOnly)
	FVector Min;

	UPROPERTY(BlueprintReadOnly)
	FVector Max;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UMaterial* Material;

	void RenderMesh();

	void RenderMesh(class UProceduralMeshComponent* InMesh);

	FORCEINLINE bool IsReady() const { return bReady; }

	FOnMeshReadySignature OnMeshReady;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	class UProceduralMeshComponent* ProceduralMesh;

	bool bReady;
};
