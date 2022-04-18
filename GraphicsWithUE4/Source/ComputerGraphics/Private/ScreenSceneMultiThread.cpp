// Fill out your copyright notice in the Description page of Project Settings.


#include "ScreenSceneMultiThread.h"
#include "HAL/RunnableThread.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BVHTree.h"
#include "DynamicTextureComponent.h"

// Called when the game starts or when spawned
void AScreenSceneMultiThread::BeginPlay()
{
    Super::BeginPlay();
	uint32 i = 0;
	for (; i < FPlatformProcess::GetCurrentCoreNumber(); ++i)
	{
		FRunnableThread::Create(new FDrawTask(i, this), *FString::Printf(TEXT("%s%d"), *StaticClass()->GetFName().ToString(), i));
	}
	UE_LOG(LogTemp, Log, TEXT(__FUNCTION__" %d:%d threads created."), __LINE__, i);
}

void AScreenSceneMultiThread::DrawOnePixel()
{
	if (CurrentDrawingY < Texture->Height)
	{
		ensure(WorkQueue.Enqueue(FPointToCompute(CurrentDrawingX, CurrentDrawingY)));
		++CurrentCompute;
		++CurrentDrawingX;
		if (CurrentDrawingX >= Texture->Width)
		{
			++CurrentDrawingY;
			CurrentDrawingX = 0;
		}
	}
	else
	{
		bEnableDrawFrame = false;
		CurrentDrawingY = 0;
	}
}

void AScreenSceneMultiThread::DrawOneLinePixel()
{
	if (CurrentCompute - CurrentDraw > MaxWorkCountPerTick)
		return;
	for (int32 i = 0; i < Texture->Width; ++i)
	{
		DrawOnePixel();
	}
}

FLinearColor AScreenSceneMultiThread::CastRayWithSpp(const FLightRay& Ray, int32 Depth)
{
	FLinearColor Result(0, 0, 0, 0);
	for (int32 i = 0; i < Spp; ++i)
	{
		Result += CastRayWithMultiThread(Ray, Depth);
	}
	Result = Result / Spp;
	return FLinearColor(Result.R, Result.G, Result.B, 1);
}

FLinearColor AScreenSceneMultiThread::CastRayWithMultiThread(const FLightRay& Ray, int32 Depth)
{
	if (!IsValid(BvhTree))
	{
		return FLinearColor::Black;
	}
	FIntersection Intersection = BvhTree->Intersect(Ray);
	FVector LDir(FVector::ZeroVector), LInder(FVector::ZeroVector);
	if (Intersection.bBlockingHit)
	{
		if (Intersection.Emit.Size() > 0)
		{
			// 打中光源
			if (Depth == 0)
			{
				AsyncTask(ENamedThreads::GameThread,
					[=]()
					{
						UKismetSystemLibrary::DrawDebugLine(GetWorld(), Ray.Origin, Intersection.Coords, Intersection.Emit, 0.1f, 5);
					}
				);
				return FLinearColor(Intersection.Emit);
			}
			else
			{
				// 多次弹射击中光源
				return FLinearColor::Black;
			}
		}
		else
		{
			FIntersection IntersectionLight;
			float PdfLight = .0f;
			SimpleLight(IntersectionLight, PdfLight);
			FVector WS = (IntersectionLight.Coords - Intersection.Coords).GetSafeNormal();
			FIntersection IntersectionCheckBlock = BvhTree->Intersect(FLightRay(IntersectionLight.Coords, WS));
			/*Shoot a ray from p to x
				If the ray is not blocked in the middle*/
			if (IntersectionCheckBlock.bBlockingHit && IntersectionCheckBlock.Emit.Size() > 0)
			{
				// L_dir = emit * eval(wo, ws, N) * dot(ws, N) * dot(ws, NN) / ((x - p) * (x - p)) / pdf_light;
				LDir = IntersectionLight.Emit; // emit
				float Product = FVector::DotProduct(Intersection.Normal, WS);				
				LDir *= ((Product > 0) ? Intersection.Kd / PI : FVector::ZeroVector); // eval(wo,ws,N)
				LDir *= Product; // dot(ws,N)
				LDir *= FVector::DotProduct(-WS, IntersectionLight.Normal); // dot(ws, NN)
				LDir /= FVector::DistSquared(IntersectionLight.Coords, Intersection.Coords); // ((x - p) * (x - p))
				LDir /= PdfLight; // pdf_light
			}
		}
	}
	else
	{
		// 啥也没打着
		return FLinearColor::Black;
	}

	float RussianRoulette = .8f;
	if (FMath::FRandRange(0.0f, 1.0f) > RussianRoulette)
	{
		if (Depth == 0)
		{
			AsyncTask(ENamedThreads::GameThread,
				[=]()
				{
					UKismetSystemLibrary::DrawDebugLine(GetWorld(), Ray.Origin, Intersection.Coords, LDir, 0.1f, 5);
				}
			);
		}
		return FLinearColor(LDir);
	}

	FVector WI = -Ray.Direction;
	FVector WO = Sample(Intersection.Normal);
	FLightRay TmpRay = FLightRay(Intersection.Coords, WO);
	FIntersection IntersectionNoEmit = BvhTree->Intersect(TmpRay);
	// 非光源
	if (IntersectionNoEmit.bBlockingHit && IntersectionNoEmit.Emit.IsNearlyZero())
	{
		// L_inder = shade(q, wi) * eval(wo, wi, N) * dot(wi, N) / pdf(wo, wi, N) / RussianRoulette
		LInder = FVector(CastRayWithMultiThread(TmpRay, Depth + 1)); // shade(q, wi)
		float Product = FVector::DotProduct(Intersection.Normal, WO);
		LInder *= Product > 0 ? (Intersection.Kd / PI) : FVector::ZeroVector; // eval(wo, wi, N)
		LInder *= FVector::DotProduct(Intersection.Normal, WI); // dot(wi, N)
		LInder /= Product > 0 ? .5f / PI : 0; // pdf(wo, wi, N)
		LInder /= RussianRoulette; // RussianRoulette
	}

	if (Depth == 0)
	{
		AsyncTask(ENamedThreads::GameThread,
			[=]()
			{
				UKismetSystemLibrary::DrawDebugLine(GetWorld(), Ray.Origin, Intersection.Coords, LDir + LInder, 0.1f, 5);
			}
		);
	}

	return FLinearColor(LDir + LInder);
}

void AScreenSceneMultiThread::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!DrawQueue.IsEmpty())
	{
		struct FDynamicTextureLock Lock(Texture);
		int32 WorkCount = 0;
		FPointToDraw Point;

		while (WorkCount++ < MaxWorkCountPerTick && DrawQueue.Dequeue(Point))
		{
			Texture->SetPixelWithoutLock(Point.Point.X, Point.Point.Y, Point.Color);
			++CurrentDraw;
		}
	}
}

void AScreenSceneMultiThread::BeginDraw()
{
	WorkQueue.Empty();
	DrawQueue.Empty();
	CurrentCompute = 0;
	CurrentDraw = 0;
	Super::BeginDraw();
}

FVector AScreenSceneMultiThread::Sample_Implementation(const FVector& Normal) const
{
	return FVector();
}

FCriticalSection FDrawTask::CriticalSection;

bool FDrawTask::Init()
{
	return IsValid(Target);
}

uint32 FDrawTask::Run()
{
	while (IsValid(Target))
	{
		FPointToCompute Point;
		bool IsDeququeSuccess;
		{
			FScopeLock Lock(&CriticalSection);
			FPlatformMisc::MemoryBarrier();
			IsDeququeSuccess = Target->WorkQueue.Dequeue(Point);
		}
		if (IsDeququeSuccess)
		{
			FLightRay Ray(FVector(0), FVector((Target->Texture->Height - 1) * 0.5f / FMath::Tan(Target->FOV / 360 * 3.141593f), Point.X - (Target->Texture->Width - 1) * 0.5f, (Target->Texture->Height - 1) * 0.5f - Point.Y).GetSafeNormal());
			FLinearColor Color = Target->CastRayWithSpp(Ray, 0);
			ensure(Target->DrawQueue.Enqueue(FPointToDraw(Point, Color)));
		}
		else
		{
			FPlatformProcess::Sleep(0.01f);
		}

	}
	return 0;
}

void FDrawTask::Exit()
{
	UE_LOG(LogTemp, Log, TEXT(__FUNCTION__" %d[%d]:exit"), __LINE__, ThreadId);
}