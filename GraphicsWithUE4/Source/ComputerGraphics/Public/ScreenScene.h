// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObjectInterface.h"
#include "ScreenScene.generated.h"

UCLASS()
class COMPUTERGRAPHICS_API AScreenScene : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* StaticMesh;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UDynamicTextureComponent* Texture;

public:	
	// Sets default values for this actor's properties
	AScreenScene();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FOV = 90;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Spp = 1;

	UPROPERTY(EditAnywhere)
	bool ShowTree;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TreeDepth = 0;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	class UBVHTree* BvhTree;

	int32 CurrentDrawingX;
	int32 CurrentDrawingY;

	bool bEnableDrawFrame;

	UPROPERTY(BlueprintReadOnly)
	bool bEnableDrawPath;

	virtual void DrawOnePixel();

	virtual void DrawOneLinePixel();

	virtual FLinearColor CastRayWithSpp(const FLightRay& Ray, int32 Depth);

	TArray<class ATriangleMesh*> TriangleMeshes;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	virtual void BeginDraw();

	UFUNCTION(BlueprintCallable)
	void BeginPath(FVector Location);

	UFUNCTION(BlueprintCallable)
	void BuildTree();

	UPROPERTY()
	TArray<FVector> Lights;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FLinearColor CastRay(const FLightRay& Ray, int32 Depth);
	FLinearColor CastRay_Implementation(const FLightRay& Ray, int32 Depth);

	UFUNCTION(BlueprintCallable)
	void SimpleLight(FIntersection& Position, float& Pdf) const;

	UFUNCTION(BlueprintCallable)
	FLightRay MakeRay(const FVector& Origin, const FVector& Direction) const {
		return FLightRay(Origin, Direction);
	}
};
