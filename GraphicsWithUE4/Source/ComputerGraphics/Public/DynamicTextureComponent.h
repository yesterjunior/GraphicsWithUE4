// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DynamicTextureComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMPUTERGRAPHICS_API UDynamicTextureComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDynamicTextureComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	UTexture2D* Texture;

	bool bDirty;

	UStaticMeshComponent* Mesh;

	FColor* DataLocked;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE UTexture2D* GetTexture() const { return Texture; }

	UFUNCTION(BlueprintCallable)
	bool SetPixel(int X, int Y, FColor Color);

	bool SetPixel(int X, int Y, FLinearColor Color);

	bool SetPixelWithoutLock(int X, int Y, FColor Color);

	bool SetPixelWithoutLock(int X, int Y, FLinearColor Color);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Width;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Height;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FColor BackGroundColor;

	friend struct FDynamicTextureLock;
};

struct FDynamicTextureLock
{
	FDynamicTextureLock(UDynamicTextureComponent* LockTarget)
	{
		_LockTarget = nullptr;
		if (LockTarget && LockTarget->Texture && !LockTarget->DataLocked)
		{
			FTexture2DMipMap& Mip = LockTarget->Texture->PlatformData->Mips[0];
			LockTarget->DataLocked = (FColor*)Mip.BulkData.Lock(LOCK_READ_WRITE);
			_LockTarget = LockTarget;
		}
	}
	~FDynamicTextureLock()
	{
		if (_LockTarget)
		{
			FTexture2DMipMap& Mip = _LockTarget->Texture->PlatformData->Mips[0];
			Mip.BulkData.Unlock();
			_LockTarget->DataLocked = nullptr;
			_LockTarget->bDirty = true;
		}
	}
	UDynamicTextureComponent* _LockTarget;
};