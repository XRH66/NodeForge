// Regression through the production validator, graph builder and report view.
#include "NodeForgeMaterialGraph.h"
#include "NodeForgeGraphLayout.h"
#include "NodeForgeResultView.h"
#include "NodeForgePrototype.h"
#include "NodeForgeInstallation.h"
#include "HAL/PlatformTime.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeValidationRules, "NodeForge.ValidationRules.SharedGraphAndResult",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeValidationRules::RunTest(const FString&)
{
    using namespace NodeForgeGraphDocument;
    FString Digest, Error;
    if (!TestTrue(TEXT("Read the installed real catalog identity"), NodeForgeInstallation::CatalogIdentity(Digest,Error))) return false;
    // Initialize the same process-local native catalog as Import/GraphPreflight.
    const FString Directory=NodeForgeInstallation::StateDirectory()/TEXT("Reports/ValidationRules")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if (!TestEqual(TEXT("Native catalog qualification"),NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),Directory/TEXT("Catalog.json")),0)) return false;
    FGraphDocument Request;Request.WireVersion=3;Request.CatalogDigest=Digest;
    Request.RequestId=TEXT("ValidationRules");Request.Target=TEXT("/Game/NodeForgeTests/ValidationRules");
    FMaterialDefinition Settings;Settings.Domain=TEXT("Surface");Settings.Blend=TEXT("Opaque");Settings.Shading=TEXT("Unlit");
    Request.Asset.Set<FMaterialDefinition>(Settings);
    FGraphDefinition Graph;Graph.Id=TEXT("MaterialGraph");Graph.Kind=TEXT("Material");
    FNodeDescription First;First.Id=TEXT("N0");First.Class=TEXT("/Script/Engine.MaterialExpressionConstant");
    First.Properties.Add(TEXT("R"),MakeShared<FJsonValueNumber>(0.25));Graph.Nodes.Add(First);
    // One graph family, exponentially many paths but only 96 nodes. Do not run
    // the former native recursive layout: its exact static visit count is 2^96-1.
    for(int32 I=1;I<96;++I)
    {
        FNodeDescription Node;Node.Id=FString::Printf(TEXT("N%d"),I);Node.Class=TEXT("/Script/Engine.MaterialExpressionAdd");Graph.Nodes.Add(Node);
        const FString Previous=FString::Printf(TEXT("N%d"),I-1);
        Graph.Edges.Add({Previous,TEXT("0"),Node.Id,TEXT("A")});
        Graph.Edges.Add({Previous,TEXT("0"),Node.Id,TEXT("B")});
    }
    Graph.Edges.Add({TEXT("N95"),TEXT("0"),TEXT("Material"),TEXT("EmissiveColor")});Request.Graphs.Add(Graph);
    const double Start=FPlatformTime::Seconds();
    const bool Valid=NodeForgeMaterialGraph::Validate(Request,Error);
    TestTrue(TEXT("Shared-DAG semantic validation succeeds"),Valid);
    UMaterial* Material=NodeForgeMaterialGraph::Build(GetTransientPackage(),MakeUniqueObjectName(GetTransientPackage(),UMaterial::StaticClass()).ToString(),Request,Error);
    TestNotNull(TEXT("Shared-DAG construction/layout succeeds"),Material);
    const double Seconds=FPlatformTime::Seconds()-Start;
    TestTrue(TEXT("Validation and layout remain bounded on the local UE5.3.2 host"),Seconds<5.0);
    if(Material)
    {
        TestEqual(TEXT("Every declared expression is retained"),Material->GetExpressions().Num(),96);
        TSharedPtr<FJsonObject> Evidence;
        TestTrue(TEXT("Real graph readback matches the same data"),NodeForgeMaterialGraph::Readback(Material,Request,Evidence,Error));
        Material->MarkAsGarbage();
    }
    auto Bad=Request;Bad.Graphs[0].Edges[0].Input=TEXT("NotARealPin");
    TestFalse(TEXT("Unknown pin remains rejected by the common validator"),NodeForgeMaterialGraph::Validate(Bad,Error));
    Bad=Request;Bad.Graphs[0].Nodes[0].Properties.Add(TEXT("NotARealProperty"),MakeShared<FJsonValueNumber>(1));
    TestFalse(TEXT("Unknown property remains rejected"),NodeForgeMaterialGraph::Validate(Bad,Error));
    Bad=Request;Bad.Graphs[0].Edges.Add({TEXT("N95"),TEXT("0"),TEXT("N1"),TEXT("A")});
    TestFalse(TEXT("Illegal input replacement/cycle remains rejected"),NodeForgeMaterialGraph::Validate(Bad,Error));
    auto Unknown=MakeShared<FJsonObject>();Unknown->SetStringField(TEXT("mode"),TEXT("GraphPreflight"));
    Unknown->SetStringField(TEXT("overallStatus"),TEXT("unknown"));Unknown->SetStringField(TEXT("operationStatus"),TEXT("passed"));
    Unknown->SetObjectField(TEXT("preflightSummary"),MakeShared<FJsonObject>());
    TestFalse(TEXT("Unknown receipt never displays input-check success"),NodeForgeResultView::Compact(Unknown).Contains(TEXT("输入检查通过")));
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("passed"),!HasAnyErrors());Result->SetNumberField(TEXT("sharedDagNodes"),96);
    Result->SetNumberField(TEXT("validationAndLayoutSeconds"),Seconds);Result->SetStringField(TEXT("scope"),TEXT("Native semantic/build/readback/result regression; no shader compilation, save, UI click or GPU recovery claim"));
    TestTrue(TEXT("Save bounded regression evidence"),NodeForgePrototype::WriteJson(Result,Directory/TEXT("Checks.json")));
    return !HasAnyErrors();
}
#endif
