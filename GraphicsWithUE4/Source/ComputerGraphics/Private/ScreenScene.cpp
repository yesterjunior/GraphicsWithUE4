// Fill out your copyright notice in the Description page of Project Settings.


#include "ScreenScene.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TriangleMesh.h"
#include "BVHTree.h"
#include "DynamicTextureComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/PointLight.h"

// Sets default values
AScreenScene::AScreenScene()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshObject(TEXT("StaticMesh'/Game/HW01/Plane.Plane'"));
	if (StaticMeshObject.Succeeded()) {
		StaticMesh->SetStaticMesh(StaticMeshObject.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterial> MaterialObject(TEXT("Material'/Game/HW06/TextureMaterial.TextureMaterial'"));
	if (MaterialObject.Succeeded()) {
		StaticMesh->SetMaterial(0, MaterialObject.Object);
	}

	Texture = CreateDefaultSubobject<UDynamicTextureComponent>(TEXT("Texture"));

    CurrentDrawingX = 0;
    CurrentDrawingY = 0;

    bEnableDrawFrame = false;
    bEnableDrawPath = false;
}

// Called when the game starts or when spawned
void AScreenScene::BeginPlay()
{
	Super::BeginPlay();

    TArray<AActor*> OutActors;
    UGameplayStatics::GetAllActorsOfClass(this, APointLight::StaticClass(), OutActors);
    for (auto Light : OutActors)
    {
        Lights.Add(Light->GetActorLocation());
    }
}

void AScreenScene::DrawOnePixel()
{
    if (CurrentDrawingY < Texture->Height)   
    {
        FLightRay Ray(FVector(0), FVector((Texture->Height - 1) * 0.5f / FMath::Tan(FOV/360 * 3.141593f), CurrentDrawingX - (Texture->Width - 1) * 0.5f, (Texture->Height - 1) * 0.5f - CurrentDrawingY).GetSafeNormal());
        if (Texture->SetPixelWithoutLock(CurrentDrawingX, CurrentDrawingY, CastRayWithSpp(Ray, 0)))
        {
            ++CurrentDrawingX;
            if (CurrentDrawingX >= Texture->Width)
            {
                ++CurrentDrawingY;
                CurrentDrawingX = 0;
            }
        }
    }
    else
    {
        bEnableDrawFrame = false;
        CurrentDrawingY = 0;
    }
}

void AScreenScene::DrawOneLinePixel()
{
    struct FDynamicTextureLock Lock(Texture);
    FPlatformMisc::MemoryBarrier();
    for (int32 i = 0; i < Texture->Width; ++i)
    {
        DrawOnePixel();
    }
}

FLinearColor AScreenScene::CastRayWithSpp(const FLightRay& Ray, int32 Depth)
{
    FLinearColor Result(0, 0, 0, 0);
    for (int32 i = 0; i < Spp; ++i)
    {
        Result += CastRay(Ray, Depth);
    }
    Result = Result / Spp;
    return FLinearColor(Result.R, Result.G, Result.B, 1);
}

// Called every frame
void AScreenScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    
    if (bEnableDrawFrame)
    {
        DrawOneLinePixel();
    }
    else if (bEnableDrawPath)
    {
        FLightRay Ray(FVector(0), FVector((Texture->Height - 1) * 0.5f / FMath::Tan(FOV / 360 * 3.141593f), CurrentDrawingX - (Texture->Width - 1) * 0.5f, (Texture->Height - 1) * 0.5f - CurrentDrawingY).GetSafeNormal());
        CastRay(Ray, 0);
    }

    if (ShowTree && BvhTree)
    {
        BvhTree->DrawTree(GetWorld(), TreeDepth);
    }
}

void AScreenScene::BeginDraw()
{
    bEnableDrawFrame = true;
    CurrentDrawingX = 0;
    CurrentDrawingY = 0;
    TArray<AActor*> OutActors;
    UGameplayStatics::GetAllActorsOfClass(this, ATriangleMesh::StaticClass(), OutActors);
    for (AActor* A : OutActors)
    {
        Cast<ATriangleMesh>(A)->ClearTriangleColor();
    }

    bEnableDrawPath = false;
}

void AScreenScene::BeginPath(FVector Location)
{
    if (!bEnableDrawFrame)
    {
        bEnableDrawPath = true;
        CurrentDrawingX = FMath::RoundToInt((Texture->Width - 1) * 0.5f + Location.Y);
        CurrentDrawingY = FMath::RoundToInt((Texture->Height - 1) * 0.5f - Location.Z);
    }
}

void AScreenScene::BuildTree()
{
    if (IsValid(BvhTree))
        return;
    BvhTree = NewObject<UBVHTree>();
    TArray<AActor*> OutActors;
    UGameplayStatics::GetAllActorsOfClass(this, ATriangleMesh::StaticClass(), OutActors);
    TArray<IObjectInterface*> Objects;
    Objects.Reserve(OutActors.Num());
    for (AActor* A : OutActors)
    {
        IObjectInterface* Obj = Cast<IObjectInterface>(A);
        if (ensure(Obj))
            Objects.Add(Obj);
    }
    BvhTree->BuildTree(Objects);
    TriangleMeshes.Reserve(OutActors.Num());
    for (AActor* Actor : OutActors)
    {
        TriangleMeshes.Add(Cast<ATriangleMesh>(Actor));
    }
}

void AScreenScene::SimpleLight(FIntersection& Position, float& Pdf) const
{
    float emit_area_sum = 0;
    for (ATriangleMesh* Mesh : TriangleMeshes)
    {
        // 是光源
        if (!Mesh->GetEmit().IsZero())
        {
            emit_area_sum += Mesh->GetArea();
        }
    }
    
    float p = FMath::FRand() * emit_area_sum;
    emit_area_sum = 0;
    for (ATriangleMesh* Mesh : TriangleMeshes)
    {
        // 是光源
        if (!Mesh->GetEmit().IsZero())
        {
            emit_area_sum += Mesh->GetArea();
            if (p <= emit_area_sum)
            {
                Mesh->Sample(Position, Pdf);
                break;
            }
        }
    }
}

FLinearColor AScreenScene::CastRay_Implementation(const FLightRay& Ray, int32 Depth)
{
    if (Depth > 5 || !BvhTree) {
        return FLinearColor::Black;
    }
    FIntersection HitResult = BvhTree->Intersect(Ray, bEnableDrawPath);
    const UTriangle* HitObject = Cast<UTriangle>(HitResult.Object.GetObject());
    FLinearColor hitColor = Texture->BackGroundColor;
    FVector HitPoint = Ray.Origin + Ray.Direction * 3000 / Ray.Direction.X;

    if (HitResult.bBlockingHit && HitObject) {
        HitPoint = HitResult.Coords;
        FVector N = HitResult.Normal;
        FLinearColor LightAmt = FLinearColor::Black;
        FVector ShadowPointOrig = (FVector::DotProduct(Ray.Direction, N) < 0) ?
            HitPoint + N * EPSILON : HitPoint - N * EPSILON;
        for (int32 i = 0; i < Lights.Num(); ++i)
        {
            FVector LightDir = Lights[i] - HitPoint;
            LightDir.Normalize();
            bool inShadow = BvhTree->Intersect(FLightRay(ShadowPointOrig, LightDir)).bBlockingHit;
            float LdotN = inShadow ? 0 : FMath::Max(0.f, FVector::DotProduct(LightDir, N));
            LightAmt += FLinearColor::White * LdotN;
            UKismetSystemLibrary::DrawDebugLine(GetWorld(), ShadowPointOrig, Lights[i], LdotN <= 0 ? FLinearColor::Black : hitColor);
        }
        hitColor = (LightAmt * (HitObject->GetColor() * 0.6f));
    }
    UKismetSystemLibrary::DrawDebugLine(GetWorld(), Ray.Origin, HitPoint, hitColor);
    return hitColor;
}