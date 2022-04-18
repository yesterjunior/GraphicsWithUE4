// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicTextureComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values for this component's properties
UDynamicTextureComponent::UDynamicTextureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	Width = 1920;
	Height = 1080;
	DataLocked = nullptr;
	// ...
}

// Called when the game starts
void UDynamicTextureComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Width > 0 && Height > 0)
	{
		Texture = UTexture2D::CreateTransient(Width, Height);
		FTexture2DMipMap& Mip = Texture->PlatformData->Mips[0];
		FColor* Data = (FColor*)Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memzero(Data, Width * Height * sizeof(FColor));
		Mip.BulkData.Unlock();
		bDirty = true;
		Mesh = Cast<UStaticMeshComponent>(GetOwner()->GetComponentByClass(UStaticMeshComponent::StaticClass()));
		if (ensure(Mesh))
		{
			Mesh->CreateDynamicMaterialInstance(0)->SetTextureParameterValue(TEXT("Tex"), Texture);
			Mesh->SetRelativeScale3D(FVector(Width, Height, 1));
		}
	}
}


// Called every frame
void UDynamicTextureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bDirty)
	{ 
		Texture->UpdateResource();
		bDirty = false;
	}
}

// https://www.ue4community.wiki/legacy/dynamic-textures-a5iczbuy
bool UDynamicTextureComponent::SetPixel(int X, int Y, FColor Color)
{
	if (Texture && ensure(X < Width && Y < Height))
	{
		FTexture2DMipMap& Mip = Texture->PlatformData->Mips[0];
		FColor* Data = (FColor*)Mip.BulkData.Lock(LOCK_READ_WRITE);
		Data[Y * Texture->GetSizeX() + X] = Color;
		Mip.BulkData.Unlock();
		bDirty = true;
		return true;
	}
	return false;
}

bool UDynamicTextureComponent::SetPixel(int X, int Y, FLinearColor Color)
{
	return SetPixel(X, Y, Color.ToFColor(true));
}

bool UDynamicTextureComponent::SetPixelWithoutLock(int X, int Y, FColor Color)
{
	if (Texture && ensure(X < Width && Y < Height) && DataLocked)
	{
		DataLocked[Y * Texture->GetSizeX() + X] = Color;
		return true;
	}
	return false;
}

bool UDynamicTextureComponent::SetPixelWithoutLock(int X, int Y, FLinearColor Color)
{
	return SetPixelWithoutLock(X, Y, Color.ToFColor(true));
}

