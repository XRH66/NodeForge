// Project-owned test helper. Calls native widgets; no third-party implementation copied.
#include "NodeForgeLayoutChecks.h"
#include "NodeForgePrototype.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"
#include "MaterialGraph/MaterialGraphSchema.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NodeFactory.h"
#include "SGraphNode.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"

namespace NodeForgeLayoutChecks
{
int32 Execute(const FString& InputPath,const FString& ReportPath)
{
    auto R=MakeShared<FJsonObject>(); R->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    R->SetStringField(TEXT("project"),FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
    R->SetStringField(TEXT("scope"),TEXT("Freshly reopened native graph widgets; SlatePrepass/GetDesiredSize at scale 1; no compile, resave or graph semantic mutation"));
    auto Finish=[&](bool Good,const FString& Reason) { R->SetStringField(TEXT("operationStatus"),Good?TEXT("passed"):TEXT("failed")); R->SetStringField(TEXT("reason"),Reason); return NodeForgePrototype::WriteJson(R,ReportPath)?(Good?0:1):3; };
    FString Text,InputDigest,InputError; TSharedPtr<FJsonObject> Input;
    if(!NodeForgePrototype::ReadInputFile(InputPath,Text,InputDigest,InputError)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Input) || !Input) return Finish(false,TEXT("Invalid bounded graph input"));
    FString Path; if(!Input->TryGetStringField(TEXT("assetPath"),Path) || !Path.StartsWith(TEXT("/Game/NodeForgeTests/"),ESearchCase::CaseSensitive)) return Finish(false,TEXT("Only isolated test assets allowed"));
    const FString Name=Path.Mid(21);
    if(Name.IsEmpty() || Name.Len()>64) return Finish(false,TEXT("Invalid test asset name"));
    for(TCHAR C:Name) if(!((C>='A'&&C<='Z')||(C>='a'&&C<='z')||(C>='0'&&C<='9'))) return Finish(false,TEXT("Invalid test asset name"));
    // This read-only inspector is dispatched before the complete import parser.
    // Validate its own required subset before loading any resource or constructing widgets.
    TSet<FString> Fixed;TSet<FName> Ids;TArray<FString> DeclaredIds;
    const TArray<TSharedPtr<FJsonValue>>* Definitions=nullptr;
    if(!Input->TryGetArrayField(TEXT("nodes"),Definitions) || Definitions->IsEmpty() || Definitions->Num()>256)
        return Finish(false,TEXT("Layout inspection requires 1..256 node objects"));
    for(const auto& Value:*Definitions)
    {
        const auto Node=Value && Value->Type==EJson::Object?Value->AsObject():nullptr;
        FString Id;
        if(!Node || !Node->TryGetStringField(TEXT("id"),Id) || Id.IsEmpty() || Id.Len()>64)
            return Finish(false,TEXT("Layout node requires a valid object and logical string ID"));
        for(TCHAR C:Id)if(!((C>='A'&&C<='Z')||(C>='a'&&C<='z')||(C>='0'&&C<='9')))
            return Finish(false,TEXT("Layout node ID must use the existing ASCII letter/digit grammar"));
        if(Ids.Contains(FName(*Id)))return Finish(false,TEXT("Duplicate layout node ID"));
        Ids.Add(FName(*Id));DeclaredIds.Add(Id);
        if(Node->HasField(TEXT("position")))
        {
            const TArray<TSharedPtr<FJsonValue>>* Position=nullptr;
            if(!Node->TryGetArrayField(TEXT("position"),Position) || Position->Num()!=2)
                return Finish(false,TEXT("Layout position requires two bounded integers"));
            for(const auto& Coordinate:*Position)
            {
                double Number=0;
                if(!Coordinate || Coordinate->Type!=EJson::Number || !Coordinate->TryGetNumber(Number)
                    || !FMath::IsFinite(Number) || FMath::Abs(Number)>1000000.0 || Number!=FMath::TruncToDouble(Number))
                    return Finish(false,TEXT("Layout position requires two bounded integers"));
            }
            Fixed.Add(Id);
        }
    }
    if(!FSlateApplication::IsInitialized()) return Finish(false,TEXT("Slate not initialized; actual widget bounds were not measured"));
    UObject* Asset=LoadObject<UObject>(nullptr,*(Path+TEXT(".")+Name)); UEdGraph* Graph=nullptr;
    if(UBlueprint* BP=Cast<UBlueprint>(Asset)) Graph=FBlueprintEditorUtils::FindEventGraph(BP);
    else if(UMaterial* M=Cast<UMaterial>(Asset))
    {
        Fixed.Add(TEXT("Material"));
        UMaterialGraph* MG=NewObject<UMaterialGraph>(GetTransientPackage(),NAME_None,RF_Transient);
        MG->Material=M; MG->Schema=UMaterialGraphSchema::StaticClass(); MG->RebuildGraph(); Graph=MG;
    }
    if(!Graph) return Finish(false,TEXT("Saved blueprint/material graph not available"));
    struct FRect {FString Id; FVector2D P,S; bool Fixed;}; TArray<FRect> Rects;
    TArray<TSharedPtr<FJsonValue>> Nodes,Overlaps;TSet<FName> ActualIds;
    for(UEdGraphNode* Node:Graph->Nodes)
    {
        FString Id;
        if(Node->NodeComment.StartsWith(TEXT("NodeForge:"))) Id=Node->NodeComment.Mid(10);
        if(UMaterialGraphNode_Root* Root=Cast<UMaterialGraphNode_Root>(Node)) Id=TEXT("Material");
        if(Id.IsEmpty())
        {
            // Material expression IDs live in their native Desc, not graph comments.
            if(auto* MN=Cast<UMaterialGraphNode>(Node))
                if(MN->MaterialExpression && MN->MaterialExpression->Desc.StartsWith(TEXT("NodeForge:"))) Id=MN->MaterialExpression->Desc.Mid(10);
        }
        if(Id.IsEmpty()) return Finish(false,TEXT("Graph contains a node without a known logical identity"));
        if(!Cast<UMaterialGraphNode_Root>(Node) && !DeclaredIds.ContainsByPredicate([&](const FString& Declared){return Declared.Equals(Id,ESearchCase::CaseSensitive);}))
            return Finish(false,TEXT("Actual layout node identity is absent from input"));
        if(ActualIds.Contains(FName(*Id)))return Finish(false,TEXT("Duplicate actual layout node identity"));
        ActualIds.Add(FName(*Id));
        TSharedPtr<SGraphNode> Widget=FNodeFactory::CreateNodeWidget(Node);
        if(!Widget) return Finish(false,TEXT("Native widget creation failed: ")+Id);
        Widget->SlatePrepass(1.0f); const FVector2D Size=Widget->GetDesiredSize();
        if(Size.X<=0 || Size.Y<=0 || !FMath::IsFinite(Size.X) || !FMath::IsFinite(Size.Y)) return Finish(false,TEXT("Native desired size unavailable: ")+Id);
        FRect Rect{Id,FVector2D(Node->NodePosX,Node->NodePosY),Size,Fixed.Contains(Id)}; Rects.Add(Rect);
        auto N=MakeShared<FJsonObject>(); N->SetStringField(TEXT("id"),Id);
        N->SetArrayField(TEXT("position"),{MakeShared<FJsonValueNumber>(Rect.P.X),MakeShared<FJsonValueNumber>(Rect.P.Y)});
        N->SetArrayField(TEXT("measuredSize"),{MakeShared<FJsonValueNumber>(Size.X),MakeShared<FJsonValueNumber>(Size.Y)});
        N->SetBoolField(TEXT("fixedAnchor"),Rect.Fixed); Nodes.Add(MakeShared<FJsonValueObject>(N));
    }
    bool Good=Rects.Num()==Definitions->Num()+(Cast<UMaterial>(Asset)?1:0);
    for(const FString& Declared:DeclaredIds)Good &= ActualIds.Contains(FName(*Declared));
    for(int32 I=0;I<Rects.Num();++I) for(int32 J=I+1;J<Rects.Num();++J)
    {
        const auto& A=Rects[I]; const auto& B=Rects[J];
        if(A.P.X<B.P.X+B.S.X && B.P.X<A.P.X+A.S.X && A.P.Y<B.P.Y+B.S.Y && B.P.Y<A.P.Y+A.S.Y)
        { Overlaps.Add(MakeShared<FJsonValueString>(A.Id+TEXT("|")+B.Id)); if(!A.Fixed || !B.Fixed) Good=false; }
    }
    R->SetStringField(TEXT("assetPath"),Path); R->SetArrayField(TEXT("actualWidgetNodes"),Nodes); R->SetArrayField(TEXT("actualWidgetOverlaps"),Overlaps);
    R->SetBoolField(TEXT("noAutomaticWidgetOverlap"),Good);
    return Finish(Good,TEXT("Measured native desired rectangles; explicit anchor overlaps retained. This is not a screenshot or human readability acceptance."));
}
}

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeLayoutBoundsTest,"NodeForge.LayoutBounds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeLayoutBoundsTest::RunTest(const FString& Parameters)
{
    FString Input,Report;
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFLayoutInput="),Input) || !FParse::Value(FCommandLine::Get(),TEXT("NFLayoutReport="),Report))
    { AddError(TEXT("NFLayoutInput and NFLayoutReport required")); return false; }
    const int32 Code=NodeForgePrototype::Execute(TEXT("GraphLayoutInspect"),Input,Report);
    if(Code) AddError(TEXT("Actual widget bounds check failed; see its preserved report"));
    return Code==0;
}
#endif
