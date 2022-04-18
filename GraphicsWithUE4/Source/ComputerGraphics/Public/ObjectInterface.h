// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ObjectInterface.h"
#include "ObjectInterface.generated.h"

const float EPSILON = 0.00001f;

USTRUCT(BlueprintType)
struct FLightRay
{
    GENERATED_USTRUCT_BODY()
    UPROPERTY(BlueprintReadOnly)
    FVector Origin;
    UPROPERTY(BlueprintReadOnly)
    FVector Direction;
    FVector DirectionInv;
    float t_min, t_max;
    FLightRay() {}
    FLightRay(const FVector& Ori, const FVector& Dir) : Origin(Ori), Direction(Dir) {
        DirectionInv = FVector(1. / Direction.X, 1. / Direction.Y, 1. / Direction.Z);
        t_min = 0.0;
        t_max = TNumericLimits<float>::Max();
    }

    FVector operator()(float t) const { return Origin + Direction * t; }
};

USTRUCT()
struct FBounds3
{
    GENERATED_USTRUCT_BODY()
    FVector pMin, pMax; // two points to specify the bounding box
    FBounds3() :pMin(FVector(TNumericLimits<float>::Max())), pMax(FVector(TNumericLimits<float>::Min())) {}
    FBounds3(const FVector p) : pMin(p), pMax(p) {}
    FBounds3(const FVector p1, const FVector p2) :pMin(p1.ComponentMin(p2)), pMax(p1.ComponentMax(p2)) { }

    FVector Diagonal() const { return pMax - pMin; }
    int maxExtent() const
    {
        FVector d = Diagonal();
        if (d.X > d.Y && d.X > d.Z)
            return 0;
        else if (d.Y > d.Z)
            return 1;
        else
            return 2;
    }

    double SurfaceArea() const
    {
        FVector d = Diagonal();
        return 2 * (d.X * d.Y + d.X * d.Z + d.Y * d.Z);
    }

    FVector Centroid() { return 0.5 * pMin + 0.5 * pMax; }
    FBounds3 Intersect(const FBounds3& b)
    {
        return FBounds3(pMin.ComponentMax(b.pMin), pMax.ComponentMin(b.pMax));
    }

    FVector Offset(const FVector& p) const
    {
        FVector o = p - pMin;
        if (pMax.X > pMin.X)
            o.X /= pMax.X - pMin.X;
        if (pMax.Y > pMin.Y)
            o.Y /= pMax.Y - pMin.Y;
        if (pMax.Z > pMin.Z)
            o.Z /= pMax.Z - pMin.Z;
        return o;
    }

    bool Overlaps(const FBounds3& b1, const FBounds3& b2)
    {
        bool X = (b1.pMax.X >= b2.pMin.X) && (b1.pMin.X <= b2.pMax.X);
        bool Y = (b1.pMax.Y >= b2.pMin.Y) && (b1.pMin.Y <= b2.pMax.Y);
        bool Z = (b1.pMax.Z >= b2.pMin.Z) && (b1.pMin.Z <= b2.pMax.Z);
        return (X && Y && Z);
    }

    bool Inside(const FVector& p, const FBounds3& b)
    {
        return (p.X >= b.pMin.X && p.X <= b.pMax.X && p.Y >= b.pMin.Y &&
            p.Y <= b.pMax.Y && p.Z >= b.pMin.Z && p.Z <= b.pMax.Z);
    }
    const FVector& operator[](int i) const
    {
        return (i == 0) ? pMin : pMax;
    }

    void Union(FBounds3 Other)
    {
        pMin = pMin.ComponentMin(Other.pMin);
        pMax = pMax.ComponentMax(Other.pMax);
    }

    void Union(FVector Point)
    {
        pMin = pMin.ComponentMin(Point);
        pMax = pMax.ComponentMax(Point);
    }

    bool IntersectP(const FLightRay& Ray, const FVector& invDir, const bool IsNeg[3]) const
    {
        float t_enter = FMath::Max3(
            ((IsNeg[0] ? pMin.X : pMax.X) - Ray.Origin.X) * invDir.X,
            ((IsNeg[1] ? pMin.Y : pMax.Y) - Ray.Origin.Y) * invDir.Y,
            ((IsNeg[2] ? pMin.Z : pMax.Z) - Ray.Origin.Z) * invDir.Z);
        float t_exit = FMath::Min3(
            ((IsNeg[0] ? pMax.X : pMin.X) - Ray.Origin.X) * invDir.X,
            ((IsNeg[1] ? pMax.Y : pMin.Y) - Ray.Origin.Y) * invDir.Y,
            ((IsNeg[2] ? pMax.Z : pMin.Z) - Ray.Origin.Z) * invDir.Z);
        return t_enter <= t_exit && t_exit >= 0;
    }
};

USTRUCT(BlueprintType)
struct FIntersection
{
    GENERATED_USTRUCT_BODY()
    
    FIntersection() {
        bBlockingHit = false;
        Coords = FVector();
        Normal = FVector();
        Distance = TNumericLimits<float>::Max();
        Object = nullptr;
    }
    UPROPERTY(BlueprintReadOnly)
    bool bBlockingHit;
    UPROPERTY(BlueprintReadOnly)
    FVector Coords;
    UPROPERTY(BlueprintReadOnly)
    FVector Normal;
    UPROPERTY(BlueprintReadOnly)
    FVector Emit;
    UPROPERTY(BlueprintReadOnly)
    FVector Kd;
    float Distance;
    TScriptInterface<class IObjectInterface> Object;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UObjectInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class COMPUTERGRAPHICS_API IObjectInterface
{
    GENERATED_BODY()

        // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
    virtual FBounds3 GetBounds() const { return FBounds3(); }
    virtual FIntersection GetIntersection(const FLightRay& Ray, bool bDraw = false) const { return FIntersection(); }
    virtual void SetColor(FLinearColor Color) const {};
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    FVector GetEmit() const;
    virtual float GetArea() const { return 0; };
    virtual void Sample(UPARAM(ref) FIntersection& Position, UPARAM(ref) float& Pdf) {};
};