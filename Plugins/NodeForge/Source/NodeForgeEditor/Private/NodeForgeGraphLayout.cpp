// Copyright Autonomix. All Rights Reserved.
// Kahn / longest-path layering adapted from PRQELT/Autonomix,
// commit 47a46ae8cc3f00230edceeb33f0d4f9d80cb185b,
// Source/AutonomixActions/Private/Blueprint/AutonomixBlueprintActions.cpp,
// FAutonomixBlueprintActions::AutoLayoutNodes. Only the layering skeleton is reused.
// NodeForge changes: all data dependencies, stable IDs, size estimates, fixed
// anchors, native material depth hints, weighted ordering and collision avoidance.
// MIT License
// Copyright (c) 2025 Autonomix
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "NodeForgeGraphLayout.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Misc/DefaultValueHelper.h"
#include "UObject/NoExportTypes.h"

namespace NodeForgeGraphLayout
{
using namespace NodeForgeGraphDocument;
namespace
{
struct FParent { int32 Index; double Weight; bool bElse; };
struct FItem
{
    FString Id;
    FVector2D Position = FVector2D::ZeroVector;
    FVector2D Size = FVector2D(320, 180);
    bool bExplicit = false;
    int32 Layer = 0;
    int32 NativeDepth = 0;
    int32 InDegree = 0;
    TArray<int32> Children;
    TArray<FParent> Parents;
    UEdGraphNode* BlueprintNode = nullptr;
    UMaterialExpression* Expression = nullptr;
};
struct FOccupied { FString Id; FVector2D Position; FVector2D Size; };
bool Overlap(const FVector2D& A, const FVector2D& AS, const FVector2D& B, const FVector2D& BS, double Gap = 48)
{
    return A.X < B.X + BS.X + Gap && A.X + AS.X + Gap > B.X
        && A.Y < B.Y + BS.Y + Gap && A.Y + AS.Y + Gap > B.Y;
}
TArray<TSharedPtr<FJsonValue>> XY(const FVector2D& V)
{
    return {MakeShared<FJsonValueNumber>(V.X), MakeShared<FJsonValueNumber>(V.Y)};
}
double TextWidth(const FString& Text) { return FMath::Min(Text.Len(), 140) * 8.5; }
FString StableDefaultText(const UEdGraphPin* Pin)
{
    const FName Category = Pin->PinType.PinCategory;
    if (Category == UEdGraphSchema_K2::PC_Real || Category == UEdGraphSchema_K2::PC_Float
        || Category == UEdGraphSchema_K2::PC_Double || Category == UEdGraphSchema_K2::PC_Int)
    {
        double Number = 0;
        if (LexTryParseString(Number, *Pin->DefaultValue) && FMath::IsFinite(Number))
            return FString::Printf(TEXT("%.12g"), Number);
    }
    if (Category == UEdGraphSchema_K2::PC_Boolean) return Pin->DefaultValue.ToLower();
    if (Category == UEdGraphSchema_K2::PC_Struct
        && Pin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
    {
        FVector Value;
        if (FDefaultValueHelper::ParseVector(Pin->DefaultValue, Value) && !Value.ContainsNaN())
            return FString::Printf(TEXT("%.12g,%.12g,%.12g"), Value.X, Value.Y, Value.Z);
    }
    return Pin->DefaultValue;
}
FVector2D Estimate(UEdGraphNode* Node, const FNodeDescription& Definition)
{
    // Display titles/friendly-pin caches and native numeric formatting can change
    // on PostLoad. Use stable binding/pin identities and canonical numbers so an
    // unchanged saved graph recomputes exactly the same layout in a fresh process.
    FString Label = Node->GetClass()->GetName();
    if (Definition.Binding.Kind == EBindingKind::Member) Label = Definition.Binding.Name;
    else if (Definition.Binding.Kind == EBindingKind::Function || Definition.Binding.Kind == EBindingKind::Event) Label = Definition.Binding.Member;
    double Width = FMath::Max(300.0, TextWidth(Label) + 130.0);
    int32 Inputs = 0, Outputs = 0;
    double InputWidth = 0, OutputWidth = 0;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin || Pin->bHidden) continue;
        const FString Name = Pin->PinName.ToString();
        if (Pin->Direction == EGPD_Input)
        {
            ++Inputs;
            InputWidth = FMath::Max(InputWidth, TextWidth(Name) + (Pin->LinkedTo.IsEmpty() ? TextWidth(StableDefaultText(Pin)) + 56 : 0));
        }
        else { ++Outputs; OutputWidth = FMath::Max(OutputWidth, TextWidth(Name)); }
    }
    return FVector2D(FMath::CeilToDouble(FMath::Max(Width, InputWidth + OutputWidth + 130) / 16.0) * 16,
        FMath::CeilToDouble(FMath::Max(160, 110 + 34 * FMath::Max(Inputs, Outputs)) / 16.0) * 16);
}
FVector2D Estimate(UMaterialExpression* Expression)
{
    TArray<FString> Captions;
    Expression->GetCaption(Captions);
    double Width = FMath::Max(240, Expression->GetWidth());
    for (const FString& Caption : Captions) Width = FMath::Max(Width, TextWidth(Caption) + 90);
    int32 Count = 0;
    for (FExpressionInput* Input : Expression->GetInputsView())
    {
        if (Input) Width = FMath::Max(Width, TextWidth(Expression->GetInputName(Count).ToString()) + 180);
        ++Count;
    }
    return FVector2D(FMath::CeilToDouble(Width / 16.0) * 16,
        FMath::CeilToDouble(FMath::Max(Expression->GetHeight(), 110 + 34 * FMath::Max(Count, Expression->GetOutputs().Num())) / 16.0) * 16);
}
bool Run(UMaterial* Asset, const FGraphDocument& Request, const TMap<FString, UEdGraphNode*>* BlueprintInstances,
    const TMap<FString, UMaterialExpression*>* MaterialInstances, FObject& Evidence, FString& Error, bool ApplyPositions = true)
{
    if (Request.Graphs.Num() != 1 || Request.Graphs[0].Nodes.Num() > 256)
    { Error = TEXT("Layout requires one bounded graph"); return false; }
    const FGraphDefinition& Graph = Request.Graphs[0];
    TMap<FString, const FNodeDescription*> ById;
    for (const FNodeDescription& Definition : Graph.Nodes)
    {
        if (ById.Contains(Definition.Id)) { Error = TEXT("Duplicate layout identity"); return false; }
        ById.Add(Definition.Id, &Definition);
    }
    TArray<FString> Ids;
    ById.GetKeys(Ids); Ids.Sort();
    TArray<FItem> Items;
    TMap<FString, int32> Index;
    for (const FString& Id : Ids)
    {
        FItem Item; Item.Id = Id;
        if (BlueprintInstances)
        {
            Item.BlueprintNode = BlueprintInstances->FindRef(Id);
            if (!Item.BlueprintNode) { Error = TEXT("Layout blueprint instance missing: ") + Id; return false; }
            Item.Size = Estimate(Item.BlueprintNode, *ById.FindChecked(Id));
        }
        else
        {
            Item.Expression = MaterialInstances->FindRef(Id);
            if (!Item.Expression) { Error = TEXT("Layout material instance missing: ") + Id; return false; }
            Item.Size = Estimate(Item.Expression);
        }
        if (ById[Id]->Position.IsSet())
        {
            Item.bExplicit = true;
            Item.Position = FVector2D(ById[Id]->Position->X, ById[Id]->Position->Y);
        }
        Index.Add(Id, Items.Num()); Items.Add(MoveTemp(Item));
    }
    // Sort edges independently of input order. Several pins between the same pair
    // count once for topology but all contribute their ordering weights.
    TArray<FEdgeDescription> OrderedEdges = Graph.Edges;
    OrderedEdges.Sort([](const FEdgeDescription& A, const FEdgeDescription& B)
    {
        const TPair<FString,FString> Parts[] = {{A.From,B.From},{A.Output,B.Output},{A.To,B.To},{A.Input,B.Input}};
        for (const auto& Pair : Parts)
        {
            const int32 Compare = Pair.Key.Compare(Pair.Value);
            if (Compare != 0) return Compare < 0;
        }
        return false;
    });
    for (const FEdgeDescription& Edge : OrderedEdges)
    {
        const FString& From = Edge.From; const FString& To = Edge.To;
        if (Asset && To == TEXT("Material"))
        {
            if (!Index.Contains(From)) { Error = TEXT("Layout material output source missing"); return false; }
            continue;
        }
        if (!Index.Contains(From) || !Index.Contains(To)) { Error = TEXT("Layout edge endpoint missing"); return false; }
        const int32 Source = Index[From], Target = Index[To];
        if (!Items[Source].Children.Contains(Target)) { Items[Source].Children.Add(Target); ++Items[Target].InDegree; }
        bool bExec = false;
        if (Items[Source].BlueprintNode)
            for (UEdGraphPin* Pin : Items[Source].BlueprintNode->Pins)
                if (Pin && Pin->Direction == EGPD_Output && Pin->PinName.ToString() == Edge.Output)
                    bExec = Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
        Items[Target].Parents.Add({Source, bExec ? 4.0 : 1.0, bExec && Edge.Output == TEXT("else")});
    }
    // Reused Autonomix Kahn / longest-path skeleton, with stable logical identity
    // and every dependency, rather than exec-only / pointer iteration order.
    TArray<int32> Queue, Order;
    for (int32 I = 0; I < Items.Num(); ++I) if (Items[I].InDegree == 0) Queue.Add(I);
    while (!Queue.IsEmpty())
    {
        Queue.Sort(); const int32 Current = Queue[0]; Queue.RemoveAt(0);
        Order.Add(Current); Items[Current].Children.Sort();
        for (int32 Child : Items[Current].Children)
        {
            Items[Child].Layer = FMath::Max(Items[Child].Layer, Items[Current].Layer + 1);
            if (--Items[Child].InDegree == 0) Queue.Add(Child);
        }
    }
    if (Order.Num() != Items.Num()) { Error = TEXT("Layout dependency cycle; no coordinates changed"); return false; }
    if (Asset)
    {
        // Native operation only changes coordinates. The request has already
        // passed semantic preflight; the defensive topology check precedes it.
        UMaterialEditingLibrary::LayoutMaterialExpressions(Asset);
        int32 MaxDepth = 0;
        for (FItem& Item : Items)
        {
            Item.NativeDepth = FMath::Max(0, static_cast<int32>(FMath::RoundToInt(-Item.Expression->MaterialExpressionEditorX / 260.0)) - 1);
            MaxDepth = FMath::Max(MaxDepth, Item.NativeDepth);
        }
        for (FItem& Item : Items) Item.Layer = MaxDepth - Item.NativeDepth;
    }
    int32 MaxLayer = 0;
    for (const FItem& Item : Items) MaxLayer = FMath::Max(MaxLayer, Item.Layer);
    TArray<double> Widths, Columns;
    Widths.Init(0, MaxLayer + 1); Columns.Init(0, MaxLayer + 1);
    for (const FItem& Item : Items) Widths[Item.Layer] = FMath::Max(Widths[Item.Layer], Item.Size.X);
    if (Asset)
    {
        double X = Asset->EditorX;
        for (int32 L = MaxLayer; L >= 0; --L) { X -= Widths[L] + 120; Columns[L] = X; }
    }
    else for (int32 L = 1; L <= MaxLayer; ++L) Columns[L] = Columns[L - 1] + Widths[L - 1] + 120;
    TArray<FOccupied> Occupied;
    if (Asset) Occupied.Add({TEXT("Material"), FVector2D(Asset->EditorX, Asset->EditorY), FVector2D(360,640)});
    TArray<TSharedPtr<FJsonValue>> Warnings;
    for (const FItem& Item : Items)
        if (Item.bExplicit)
        {
            for (const FOccupied& Anchor : Occupied)
                if (Overlap(Item.Position, Item.Size, Anchor.Position, Anchor.Size, 0))
                {
                    Warnings.Add(MakeShared<FJsonValueString>(Anchor.Id+TEXT("|")+Item.Id+TEXT(": explicit coordinates retained")));
                }
            Occupied.Add({Item.Id, Item.Position, Item.Size});
        }
    // Weighted parent barycenters reduce crossings; this is a heuristic, not an
    // optimum. Else execution paths favor a lower row than the then path.
    for (int32 L = 0; L <= MaxLayer; ++L)
    {
        TArray<int32> Layer;
        TMap<int32, double> Scores;
        for (int32 I = 0; I < Items.Num(); ++I)
            if (Items[I].Layer == L)
            {
                Layer.Add(I); double Sum = 0, Weight = 0;
                for (const FParent& Parent : Items[I].Parents)
                {
                    Sum += (Items[Parent.Index].Position.Y + (Parent.bElse ? Items[Parent.Index].Size.Y + 120 : 0)) * Parent.Weight;
                    Weight += Parent.Weight;
                }
                Scores.Add(I, Items[I].bExplicit ? Items[I].Position.Y : (Weight > 0 ? Sum / Weight : 0));
            }
        Layer.Sort([&](int32 A, int32 B) { return Scores[A] == Scores[B] ? A < B : Scores[A] < Scores[B]; });
        for (int32 I : Layer)
        {
            FItem& Item = Items[I]; if (Item.bExplicit) continue;
            Item.Position = FVector2D(Columns[L], FMath::RoundToInt(Scores[I] / 16.0) * 16);
            bool bClear = false;
            for (int32 Attempt = 0; Attempt <= Occupied.Num(); ++Attempt)
            {
                double NextY = Item.Position.Y; bool bCollision = false;
                for (const FOccupied& Other : Occupied)
                    if (Overlap(Item.Position, Item.Size, Other.Position, Other.Size))
                    { bCollision = true; NextY = FMath::Max(NextY, Other.Position.Y + Other.Size.Y + 64); }
                if (!bCollision) { bClear = true; break; }
                Item.Position.Y = FMath::CeilToDouble(NextY / 16.0) * 16;
            }
            if (!bClear) { Error = TEXT("Layout could not find a free row"); return false; }
            Occupied.Add({Item.Id, Item.Position, Item.Size});
        }
    }
    Evidence = MakeShared<FJsonObject>();
    Evidence->SetStringField(TEXT("algorithm"), Asset ? TEXT("nativeMaterialDepth-stableWeightedLayers-anchors-v1") : TEXT("AutonomixKahn-allDependencies-stableWeightedLayers-anchors-v1"));
    Evidence->SetStringField(TEXT("sizePolicy"), Asset ? TEXT("Conservative native/title/pin/default estimates; not measured Slate bounds; 48 unit collision margin")
        : TEXT("Stable native binding/pin/canonical-default estimates v2; not measured Slate bounds; 48 unit collision margin"));
    TArray<TSharedPtr<FJsonValue>> Nodes;
    for (FItem& Item : Items)
    {
        Item.Position.X=FMath::RoundToInt(Item.Position.X);
        Item.Position.Y=FMath::RoundToInt(Item.Position.Y);
        if (Item.Expression) { Item.Expression->MaterialExpressionEditorX = static_cast<int32>(Item.Position.X); Item.Expression->MaterialExpressionEditorY = static_cast<int32>(Item.Position.Y); }
        else if (ApplyPositions) { Item.BlueprintNode->NodePosX = static_cast<int32>(Item.Position.X); Item.BlueprintNode->NodePosY = static_cast<int32>(Item.Position.Y); }
        FObject Node = MakeShared<FJsonObject>(); Node->SetStringField(TEXT("id"), Item.Id);
        Node->SetArrayField(TEXT("position"), XY(Item.Position)); Node->SetArrayField(TEXT("boundsSize"), XY(Item.Size));
        Node->SetBoolField(TEXT("explicitPosition"), Item.bExplicit); Node->SetNumberField(TEXT("layer"), Item.Layer);
        Node->SetStringField(TEXT("sizeSource"), TEXT("estimate")); Nodes.Add(MakeShared<FJsonValueObject>(Node));
    }
    Evidence->SetArrayField(TEXT("explicitAnchorOverlapWarnings"), Warnings);
    if (Asset)
    {
        FObject Root = MakeShared<FJsonObject>();
        Root->SetStringField(TEXT("id"),TEXT("Material"));
        Root->SetBoolField(TEXT("explicitPosition"),true);
        Root->SetNumberField(TEXT("layer"),MaxLayer+1);
        Root->SetArrayField(TEXT("position"), XY(FVector2D(Asset->EditorX, Asset->EditorY)));
        Root->SetArrayField(TEXT("boundsSize"), XY(FVector2D(360, 640)));
        Root->SetStringField(TEXT("sizeSource"), TEXT("estimate"));
        Nodes.Add(MakeShared<FJsonValueObject>(Root));
    }
    Evidence->SetArrayField(TEXT("nodes"), Nodes);
    return true;
}
}
bool Material(UMaterial* Asset, const FGraphDocument& Request, const TMap<FString, UMaterialExpression*>& Instances, FObject& Evidence, FString& Error)
{
    if (!Asset) { Error = TEXT("Layout material is null"); return false; }
    return Run(Asset, Request, nullptr, &Instances, Evidence, Error);
}
bool Blueprint(const FGraphDocument& Request, const TMap<FString, UEdGraphNode*>& Instances, FObject& Evidence, FString& Error, bool ApplyPositions)
{
    return Run(nullptr, Request, &Instances, nullptr, Evidence, Error, ApplyPositions);
}
}
