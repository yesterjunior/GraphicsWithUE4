// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ScreenScene.h"
#include "HAL/Runnable.h"
#include "ScreenSceneMultiThread.generated.h"

class FDrawTask : public FRunnable
{
public:
	FDrawTask(int32 Id, class AScreenSceneMultiThread* Actor) :ThreadId(Id), Target(Actor) {}
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;
private:
	int32 ThreadId;
	class AScreenSceneMultiThread* Target;
	static FCriticalSection CriticalSection;
};

struct FPointToCompute
{
	int32 X;
	int32 Y;
public:
	FORCEINLINE FPointToCompute() : X(0), Y(0) { }
	FORCEINLINE FPointToCompute(int32 x, int32 y) : X(x), Y(y) { }
};

struct FPointToDraw
{
	FPointToCompute Point;
	FLinearColor Color;
public:
	FORCEINLINE FPointToDraw(FPointToCompute point = FPointToCompute(), FLinearColor InColor = FLinearColor::Black) : Point(point), Color(InColor) { }
	FORCEINLINE FPointToDraw(int32 x, int32 y, FLinearColor InColor = FLinearColor::Black) : Point(x, y), Color(InColor){ }
};

/**
 * 
 */
UCLASS()
class COMPUTERGRAPHICS_API AScreenSceneMultiThread : public AScreenScene
{
	GENERATED_BODY()
	
	friend class FDrawTask;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void DrawOnePixel() override;

	virtual void DrawOneLinePixel() override;

	virtual FLinearColor CastRayWithSpp(const FLightRay& Ray, int32 Depth) override;

	FLinearColor CastRayWithMultiThread(const FLightRay& Ray, int32 Depth);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void BeginDraw() override;

	UPROPERTY(EditAnywhere)
	int32 MaxWorkCountPerTick = 4096;

	UFUNCTION(BlueprintNativeEvent)
	FVector Sample(const FVector& Normal) const;
	FVector Sample_Implementation(const FVector& Normal) const;

private:
	TQueue<FPointToCompute> WorkQueue;
	TQueue<FPointToDraw, EQueueMode::Mpsc> DrawQueue;
	int32 CurrentCompute;
	int32 CurrentDraw;
};
