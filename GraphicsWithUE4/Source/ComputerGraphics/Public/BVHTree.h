// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ObjectInterface.h"
#include "BVHTree.generated.h"

class FBVHNode : public TSharedFromThis<FBVHNode, ESPMode::ThreadSafe>
{
public:
	FBVHNode() : Left(nullptr), Right(nullptr), Bound(FBounds3()), Area(0) {
		Objects.Empty();
	}
	TSharedPtr<class FBVHNode, ESPMode::ThreadSafe> Left;
	TSharedPtr<class FBVHNode, ESPMode::ThreadSafe> Right;
	TArray<IObjectInterface*> Objects;
	FBounds3 Bound;
	float Area;

	void DrawNode(UObject* WorldContextObject, int32 Depth);
};

/**
 * 
 */
UCLASS()
class COMPUTERGRAPHICS_API UBVHTree : public UObject
{
	GENERATED_BODY()
	
public:
	UBVHTree() : RootNode(nullptr) , DrawDepth(100){}

	void BuildTree(TArray<IObjectInterface*> Objects, int32 MaxTriangleInNode = 1);

	UFUNCTION(BlueprintCallable)
	FIntersection Intersect(const FLightRay& Ray, bool bDraw = false);

	void ClearTriangleColor();

	UFUNCTION(BlueprintCallable)
	void RefreshDepth() { ++DrawDepth; }

	void Sample(FIntersection& Position, float& Pdf);

	void DrawTree(UObject* WorldContextObject, int32 Depth);
private:
	TSharedPtr<class FBVHNode, ESPMode::ThreadSafe> RootNode;
	int32 _MaxTriangleInNode;
	int32 DrawDepth;
	int32 DrawDepthMax;
	FBVHNode* RecursiveBuild(TArray<IObjectInterface*> Objects);
	FIntersection GetIntersection(TSharedRef<FBVHNode, ESPMode::ThreadSafe> NodeRef, const FLightRay& Ray, bool bDraw, int32 Depth);
	void ColorTriangle(TSharedRef<FBVHNode, ESPMode::ThreadSafe> NodeRef, FLinearColor Color);
	void GetSample(TSharedRef<FBVHNode, ESPMode::ThreadSafe> NodeRef, float p, FIntersection& Position, float& Pdf);
};
