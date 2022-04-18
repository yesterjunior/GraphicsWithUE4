// Fill out your copyright notice in the Description page of Project Settings.


#include "BVHTree.h"
#include "TriangleMesh.h"
#include "Kismet/KismetSystemLibrary.h"

void UBVHTree::BuildTree(TArray<IObjectInterface*> Objects, int32 MaxTriangleInNode)
{
    check(MaxTriangleInNode > 0);
    _MaxTriangleInNode = MaxTriangleInNode;
    RootNode = MakeShareable(RecursiveBuild(Objects));
}

FBVHNode* UBVHTree::RecursiveBuild(TArray<IObjectInterface*> Objects)
{
    if (Objects.Num() <= 0)
    {
        return nullptr;
    }
    FBVHNode* Node = new FBVHNode();
    // Compute Bound of all primitives in BVH Node
    FBounds3 Bound;
    for (IObjectInterface* Object : Objects)
        Bound.Union(Object->GetBounds());
    if (Objects.Num() <= _MaxTriangleInNode) {
        // Create leaf _BVHBuildNode_
        Node->Objects = Objects;
        Node->Bound = FBounds3();
        Node->Area = 0;
        for (IObjectInterface* Object : Objects)
        {
            Node->Bound.Union(Object->GetBounds());
            Node->Area += Object->GetArea();
        }
        Node->Left = nullptr;
        Node->Right = nullptr;
        return Node;
    }
    else {
        FBounds3 CentroidBounds;
        for (IObjectInterface* Object : Objects)
            CentroidBounds.Union(Object->GetBounds().Centroid());
        int32 dim = CentroidBounds.maxExtent();

        Objects.Sort([dim](const IObjectInterface& Obj0, const IObjectInterface& Obj1) {
            return Obj0.GetBounds().Centroid()[dim] < Obj1.GetBounds().Centroid()[dim];
            });

        int32 beginning = 0;
        int32 middling = (Objects.Num() / 2);
        int32 ending = Objects.Num();

        TArray<IObjectInterface*> LeftShapes, RightShapes;
        LeftShapes.Reserve(middling + (ending & 1));
        RightShapes.Reserve(middling);
        for (int32 i = beginning; i < ending; ++i)
        {
            if (i < middling)
            {
                LeftShapes.Add(Objects[i]);
            }
            else
            {
                RightShapes.Add(Objects[i]);
            }
        }

        ensure(Objects.Num() == (LeftShapes.Num() + RightShapes.Num()));

        Node->Left = MakeShareable(RecursiveBuild(LeftShapes));
        Node->Right = MakeShareable(RecursiveBuild(RightShapes));

        Node->Bound.Union(Node->Left ? Node->Left->Bound : FBounds3());
        Node->Bound.Union(Node->Right ? Node->Right->Bound : FBounds3());
        Node->Area = Node->Left->Area + Node->Right->Area;
    }
    
    return Node;
}

FIntersection UBVHTree::Intersect(const FLightRay& Ray, bool bDraw)
{
    if (RootNode)
    {
        if (bDraw && IsInGameThread())
        {
            if (DrawDepth > DrawDepthMax + 2)
            {
                ColorTriangle(RootNode.ToSharedRef(), FLinearColor::Red);
                DrawDepth = 0;
            }
        }
        return GetIntersection(RootNode.ToSharedRef(), Ray, bDraw, 0);
    }
    return FIntersection();
}

void UBVHTree::ClearTriangleColor()
{
    if (RootNode)
    {
        ColorTriangle(RootNode.ToSharedRef(), FLinearColor::Gray);
        DrawDepth = 0;
    }
}

void UBVHTree::Sample(FIntersection& Position, float& Pdf)
{
    float p = FMath::Sqrt(FMath::FRand()) * RootNode->Area;
    GetSample(RootNode.ToSharedRef(), p, Position, Pdf);
    Pdf /= RootNode->Area;
}

void UBVHTree::DrawTree(UObject* WorldContextObject, int32 Depth)
{
    if (RootNode)
    {
        RootNode->DrawNode(WorldContextObject, Depth);
    }
}

FIntersection UBVHTree::GetIntersection(TSharedRef<FBVHNode, ESPMode::ThreadSafe> NodeRef, const FLightRay& Ray, bool bDraw, int32 Depth)
{
    if (DrawDepthMax < Depth)
    {
        DrawDepthMax = Depth;
    }
    FIntersection HitResult;
    FBVHNode& Node = NodeRef.Get();
    bool IsNeg[] = { Ray.Direction.X > 0, Ray.Direction.Y > 0, Ray.Direction.Z > 0 };
    if (Node.Bound.IntersectP(Ray, Ray.DirectionInv, IsNeg))
    {
        FIntersection L, R;
        ensure(!!Node.Left == !!Node.Right);
        if (Node.Left)
        {
            L = GetIntersection(Node.Left.ToSharedRef(), Ray, bDraw, Depth + 1);
        }
        if (Node.Right)
        {
            R = GetIntersection(Node.Right.ToSharedRef(), Ray, bDraw, Depth + 1);
        }
        if (L.bBlockingHit && R.bBlockingHit)
        {
            HitResult = L.Distance < R.Distance ? L : R;
            if (bDraw && Depth == DrawDepth && IsInGameThread())
            {
                ColorTriangle((L.Distance < R.Distance ? Node.Right : Node.Left).ToSharedRef(), FLinearColor::Blue);
            }
        }
        else if (L.bBlockingHit ^ R.bBlockingHit)
        {
            HitResult = L.bBlockingHit ? L : R;
        }
        else
        {
            IObjectInterface* HitObject = nullptr;
            float MinDistance = TNumericLimits<float>::Max();
            for (IObjectInterface* Obj : Node.Objects)
            {
                HitResult = Obj->GetIntersection(Ray, bDraw);
                if (HitResult.bBlockingHit && HitResult.Distance < MinDistance)
                {
                    MinDistance = HitResult.Distance;
                    HitObject = Obj;
                }
            }
        }
        if (bDraw && Depth == 0 && HitResult.Object && IsInGameThread())
        {
            HitResult.Object->SetColor(FLinearColor::Red);
        }
    }
    else if (bDraw && IsInGameThread())
    {
        if (DrawDepth == Depth)
            ColorTriangle(NodeRef, FLinearColor::Gray);
    }
    return HitResult;
}

void UBVHTree::ColorTriangle(TSharedRef<FBVHNode, ESPMode::ThreadSafe> NodeRef, FLinearColor Color)
{
    FBVHNode& Node = NodeRef.Get();
    if (Node.Left)
        ColorTriangle(Node.Left.ToSharedRef(), Color);
    if (Node.Right)
        ColorTriangle(Node.Right.ToSharedRef(), Color);
    for (IObjectInterface* Obj : Node.Objects)
    {
        Obj->SetColor(Color);
    }
}

void UBVHTree::GetSample(TSharedRef<FBVHNode, ESPMode::ThreadSafe> NodeRef, float p, FIntersection& Position, float& Pdf)
{
    if (!NodeRef->Left.IsValid() || !NodeRef->Right.IsValid()) {
        NodeRef->Objects[0]->Sample(Position, Pdf);
        Pdf *= NodeRef->Area;
        return;
    }
    if (p < NodeRef->Left->Area)
        GetSample(NodeRef->Left.ToSharedRef(), p, Position, Pdf);
    else
        GetSample(NodeRef->Right.ToSharedRef(), p - NodeRef->Left->Area, Position, Pdf);
}

void FBVHNode::DrawNode(UObject* WorldContextObject, int32 Depth)
{
    if (!IsInGameThread()) return;
    if (Depth == 0)
    {
        FVector Center = Bound.Centroid();
        FLinearColor Color = (Center / Center.GetAbsMax()).GetAbs();
        UKismetSystemLibrary::DrawDebugBox(WorldContextObject, Center, (Bound.pMax - Bound.pMin) * 0.52f, Color);
    }
    else
    {
        if (Left)
        {
            Left->DrawNode(WorldContextObject, Depth - 1);
        }
        if (Right)
        {
            Right->DrawNode(WorldContextObject, Depth - 1);
        }
    }
}
