// Project-owned canonical identity and bounded report regression; saved resources stay read-only.
#include "NodeForgePrototype.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgeNodeDefinitions.h"
#include "Engine/Blueprint.h"
#include "NodeForgeMaterialGraph.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "MaterialShared.h"
#include "Editor.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include "GameFramework/Actor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "NodeForgeGraphDiff.h"
#include "NodeForgeAssetDefinitions.h"
#include "NodeForgeMaterialFunctions.h"
#include "NodeForgeMaterialInstances.h"
#include "NodeForgeTaskRules.h"
#include "NodeForgeResultView.h"
#include "NodeForgeLayoutChecks.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "Async/Async.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Algo/Reverse.h"

namespace
{
using FResourceObject=TSharedPtr<FJsonObject>;
FResourceObject CopyResourceJson(const FResourceObject& Source)
{
    FString Text;FResourceObject Result;
    if(Source && FJsonSerializer::Serialize(Source.ToSharedRef(),TJsonWriterFactory<>::Create(&Text)))
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Result);
    return Result;
}
FString OtherCase(const FString& Value)
{
    const FString Lower=Value.ToLower();
    return Lower.Equals(Value,ESearchCase::CaseSensitive)?Value.ToUpper():Lower;
}
TMap<FString,FString> ResourceAuditAssets()
{
    TMap<FString,FString> Result;TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files,*FPaths::ProjectContentDir(),TEXT("*"),true,false);
    for(const FString& File:Files)
        if(File.EndsWith(TEXT(".uasset")) || File.EndsWith(TEXT(".umap"))) Result.Add(File,NodeForgePrototype::FileSha256(File));
    return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeCanonicalResourceTest,"NodeForge.CanonicalResourceGuards",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeCanonicalResourceTest::RunTest(const FString& Parameters)
{
    FString Input,Output;const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFInput="),Input) || !FParse::Value(FCommandLine::Get(),TEXT("NFReport="),Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Input),Root/TEXT("Reports"))
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Root/TEXT("Reports"))
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()),Root/TEXT("ValidationProject/RegressionRuns"))
        || IFileManager::Get().FileExists(*Output))
    {AddError(TEXT("Fresh local report/config and isolated project required"));return false;}
    FString Raw,Hash,Error;FResourceObject Config;
    const TArray<TSharedPtr<FJsonValue>>* Parents=nullptr;
    if(!NodeForgePrototype::ReadInputFile(Input,Raw,Hash,Error)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Config) || !Config)
    {AddError(TEXT("Actual bounded config required"));return false;}
    if(Config->HasTypedField<EJson::Object>(TEXT("assetSettings")))
    {
        using namespace NodeForgeGraphDocument;
        const auto Before=ResourceAuditAssets();const auto Scoped=Config->GetObjectField(TEXT("assetSettings"));
        TArray<TSharedPtr<FJsonValue>> Rows,NativeEvidence;
        auto Row=[&](const FString& Name,bool Good){TestTrue(Name,Good);auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),Name);Item->SetBoolField(TEXT("passed"),Good);Item->SetStringField(TEXT("diagnostic"),Error);Rows.Add(MakeShared<FJsonValueObject>(Item));Error.Reset();};
        const FString CatalogPath=FPaths::GetPath(Output)/TEXT("NativeSettingsCatalog.json");
        Row(TEXT("ActualCurrentCatalog"),NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),CatalogPath)==0);
        FString CatalogText;FResourceObject Catalog;
        const bool HasCatalog=FFileHelper::LoadFileToString(CatalogText,*CatalogPath) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(CatalogText),Catalog);
        Row(TEXT("NativeCatalogRead"),HasCatalog);
        TArray<FGraphDocument> Documents;
        if(HasCatalog) for(const TCHAR* Kind:{TEXT("materialInput"),TEXT("blueprintInput")})
        {
            FString Text,Digest;FResourceObject Wire;FGraphDocument Document;
            bool Good=NodeForgePrototype::ReadInputFile(Scoped->GetStringField(Kind),Text,Digest,Error) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Wire);
            if(Good) Wire->SetStringField(TEXT("catalogDigest"),Catalog->GetStringField(TEXT("catalogDigest")));
            Good=Good && NodeForgeGraphCodec::Decode(Wire,Document,Error);
            Row(FString(Kind)+TEXT("ActualDigestDecode"),Good);if(Good)Documents.Add(Document);
        }
        if(Documents.Num()==2)
        {
            auto Mat=Documents[0];auto Bp=Documents[1];FResourceObject Evidence;
            auto MaterialCheck=[&](const FString& Name,const FGraphDocument& D,bool Representative)
            {
                UMaterial* M=NodeForgeMaterialGraph::Build(GetTransientPackage(),MakeUniqueObjectName(GetTransientPackage(),UMaterial::StaticClass(),TEXT("SettingsProbe")).ToString(),D,Error);
                bool Good=M && NodeForgeMaterialGraph::Readback(M,D,Evidence,Error);
                if(M && Representative)
                    Good &= M->MaterialDomain==MD_Surface && M->BlendMode==BLEND_Masked && M->GetShadingModels().IsUnlit() && M->TwoSided
                        && M->OpacityMaskClipValue==.5f && M->DitherOpacityMask && M->DitheredLODTransition && M->bAllowNegativeEmissiveColor;
                if(M && !Representative)
                    Good &= M->OpacityMaskClipValue==0.f && !M->DitherOpacityMask && !M->DitheredLODTransition && !M->bAllowNegativeEmissiveColor;
                if(Evidence)NativeEvidence.Add(MakeShared<FJsonValueObject>(Evidence));
                Row(Name,Good);
                if(M){M->ClearFlags(RF_Public|RF_Standalone);M->SetFlags(RF_Transient);M->MarkAsGarbage();}
            };
            MaterialCheck(TEXT("MaskedIndependentNativeValues"),Mat,true);
            auto OldMat=Mat;auto& OldSettings=OldMat.Asset.Get<FMaterialDefinition>();
            OldSettings.Blend=TEXT("Opaque");OldSettings.OpacityMaskClipValue=0.;OldSettings.DitherOpacityMask=false;OldSettings.DitheredLodTransition=false;OldSettings.AllowNegativeEmissiveColor=false;
            OldMat.Graphs[0].Nodes.RemoveAll([](const auto& N){return N.Id==TEXT("Mask");});
            OldMat.Graphs[0].Edges.RemoveAll([](const auto& E){return E.Input==TEXT("OpacityMask");});
            MaterialCheck(TEXT("OpaqueInactiveClipZeroAndFalseStored"),OldMat,false);
            OldSettings.Blend=TEXT("Translucent");OldSettings.Lighting=TEXT("SurfaceForwardShading");
            MaterialCheck(TEXT("TranslucentExistingModeAndFalseStored"),OldMat,false);
            auto Bad=Mat;Bad.Asset.Get<FMaterialDefinition>().Blend=TEXT("Opaque");
            const int32 BeforeEdges=Bad.Graphs[0].Edges.Num();Row(TEXT("InactiveMaskEdgeRejectedWithoutDeleting"),!NodeForgeMaterialGraph::Validate(Bad,Error) && Bad.Graphs[0].Edges.Num()==BeforeEdges);
            Bad=Mat;Bad.Asset.Get<FMaterialDefinition>().Shading=TEXT("DefaultLit");Row(TEXT("NegativeEmissiveDefaultLitRejected"),!NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Bad,3),Bad,Error));
            for(double Clip:{-0.1,1.1})
            {Bad=Mat;Bad.Asset.Get<FMaterialDefinition>().OpacityMaskClipValue=Clip;FGraphDocument Rejected;Row(TEXT("ClipRangeRejected"),!NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Bad,3),Rejected,Error));}
            Bad=Mat;Bad.Asset.Get<FMaterialDefinition>().OpacityMaskClipValue.Reset();FGraphDocument Rejected;
            Row(TEXT("MaskedClipPresenceRequired"),!NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Bad,3),Rejected,Error));
            auto* BP=NodeForgeBlueprintGraph::Build(GetTransientPackage(),TEXT("MemberSettingsProbe"),Bp,Error);
            Row(TEXT("SevenMembersProductionBuild"),BP!=nullptr);
            if(BP)
            {
                FKismetEditorUtilities::CompileBlueprint(BP);
                const bool Good=(BP->Status==BS_UpToDate || BP->Status==BS_UpToDateWithWarnings) && NodeForgeBlueprintGraph::Readback(BP,Bp,Evidence,Error);
                Row(TEXT("SevenCompiledPropertiesFlagsMetadataCDO"),Good);if(Evidence)NativeEvidence.Add(MakeShared<FJsonValueObject>(Evidence));
                const auto* Label=BP->NewVariables.FindByPredicate([](const auto& V){return V.VarName==TEXT("Label");});
                Row(TEXT("NativeStringMultilinePreserved"),Label && Label->MetaDataArray.ContainsByPredicate([](const auto& M){return M.DataKey==TEXT("MultiLine");}));
                auto* World=GEditor?GEditor->GetEditorWorldContext().World():nullptr;
                FActorSpawnParameters Params;Params.ObjectFlags=RF_Transient;Params.bNoFail=false;
                auto* Instance=Good && World?World->SpawnActor<AActor>(BP->GeneratedClass,FTransform::Identity,Params):nullptr;
                auto* Speed=FindFProperty<FNumericProperty>(BP->GeneratedClass,TEXT("Speed"));auto* Count=FindFProperty<FProperty>(BP->GeneratedClass,TEXT("Count"));
                const bool SpeedEditable=Instance && Speed && Speed->HasAnyPropertyFlags(CPF_Edit) && !Speed->HasAnyPropertyFlags(CPF_DisableEditOnInstance|CPF_EditConst) && Instance->CanEditChange(Speed);
                const bool CountEditable=Instance && Count && Count->HasAnyPropertyFlags(CPF_Edit) && !Count->HasAnyPropertyFlags(CPF_DisableEditOnInstance|CPF_EditConst) && Instance->CanEditChange(Count);
                Row(TEXT("EditorInstanceQualificationSpeedYesCountNo"),SpeedEditable && !CountEditable);
                if(SpeedEditable)
                {
                    Instance->PreEditChange(Speed);Speed->SetFloatingPointPropertyValue(Speed->ContainerPtrToValuePtr<void>(Instance),125.);
                    FPropertyChangedEvent Change(Speed,EPropertyChangeType::ValueSet);Instance->PostEditChangeProperty(Change);
                    auto* CDO=BP->GeneratedClass->GetDefaultObject();
                    const double Actual=Speed->GetFloatingPointPropertyValue(Speed->ContainerPtrToValuePtr<void>(Instance));
                    const double Default=Speed->GetFloatingPointPropertyValue(Speed->ContainerPtrToValuePtr<void>(CDO));
                    Row(TEXT("InstanceSpeed125ClassDefault100"),Actual==125. && Default==100.);
                    auto I=MakeShared<FJsonObject>();I->SetNumberField(TEXT("instanceSpeed"),Actual);I->SetNumberField(TEXT("classDefaultSpeed"),Default);I->SetBoolField(TEXT("CountEditable"),CountEditable);I->SetBoolField(TEXT("savedMap"),false);NativeEvidence.Add(MakeShared<FJsonValueObject>(I));
                }
                if(Instance)World->DestroyActor(Instance);
                BP->ClearFlags(RF_Public|RF_Standalone);BP->SetFlags(RF_Transient);BP->MarkAsGarbage();
            }
            auto Omitted=Bp;for(auto& V:Omitted.Asset.Get<FBlueprintDefinition>().Variables)V.Attributes.Reset();
            auto* Factory=NodeForgeBlueprintGraph::Build(GetTransientPackage(),TEXT("OriginalMemberFactoryProbe"),Omitted,Error);
            if(Factory)FKismetEditorUtilities::CompileBlueprint(Factory);
            bool FactoryGood=Factory && NodeForgeBlueprintGraph::Readback(Factory,Omitted,Evidence,Error);
            if(Factory)for(const auto& V:Factory->NewVariables)FactoryGood &= (V.PropertyFlags & CPF_DisableEditOnInstance)!=0;
            Row(TEXT("SevenOmittedAttributesOriginalFactory"),FactoryGood);
            if(Factory){Factory->ClearFlags(RF_Public|RF_Standalone);Factory->SetFlags(RF_Transient);Factory->MarkAsGarbage();}
            for(int32 Version:{2,3}) for(const auto& D:Documents)
            {FGraphDocument RoundTrip;Row(TEXT("CompleteSharedWireRoundTrip"),NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(D,Version),RoundTrip,Error) && NodeForgeGraphCodec::EqualOriginal(D,RoundTrip));}
            auto Old=OldMat;Old.Asset.Get<FMaterialDefinition>().OpacityMaskClipValue.Reset();Old.Asset.Get<FMaterialDefinition>().DitherOpacityMask.Reset();Old.Asset.Get<FMaterialDefinition>().DitheredLodTransition.Reset();Old.Asset.Get<FMaterialDefinition>().AllowNegativeEmissiveColor.Reset();
            for(const TCHAR* Digest:{TEXT("6b9074116e09b256da2bc4bb149e8083d1af07407b70541550433534dcfea4db"),TEXT("a6a80ffd78ef86d3c8fdfdbb64f8c1b73e9852795d05d9fcbad5ccb93ce939f2"),TEXT("3908fac046ddb215e53529a94afe53d3f61f86de0731af02e75738de32edc606")})
                for(int32 Version:{2,3})
                {
                    const bool Allowed=Version==2 || FString(Digest)!=TEXT("3908fac046ddb215e53529a94afe53d3f61f86de0731af02e75738de32edc606");
                    for(auto D:{Old,Omitted}) {D.CatalogDigest=Digest;FGraphDocument Decoded;const bool Accepted=NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(D,Version),Decoded,Error);Row(TEXT("ArchivedOriginalWireBoundary"),Accepted==Allowed);}
                    for(auto D:Documents){D.CatalogDigest=Digest;FGraphDocument Decoded;Row(TEXT("ArchivedNewSettingRejected"),!NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(D,Version),Decoded,Error));}
                }
            auto Explicit=Omitted;FMemberAttributes Attr;Attr.Category=TEXT("Default");Explicit.Asset.Get<FBlueprintDefinition>().Variables[0].Attributes=Attr;
            Row(TEXT("MemberAttributePresenceOriginalSensitive"),!NodeForgeGraphCodec::EqualOriginal(Omitted,Explicit));
            auto OmittedMaterial=Old;auto ExplicitMaterial=Old;ExplicitMaterial.Asset.Get<FMaterialDefinition>().DitherOpacityMask=false;
            Row(TEXT("MaterialFalsePresenceOriginalSensitive"),!NodeForgeGraphCodec::EqualOriginal(OmittedMaterial,ExplicitMaterial));
            auto Wire=NodeForgeGraphCodec::Encode(Bp,3);auto Attributes=Wire->GetObjectField(TEXT("asset"))->GetObjectField(TEXT("members"))->GetArrayField(TEXT("variables"))[0]->AsObject()->GetObjectField(TEXT("attributes"));
            Attributes->SetBoolField(TEXT("unknown"),false);Row(TEXT("UnknownAttributeRejected"),!NodeForgeGraphCodec::Decode(Wire,Rejected,Error));Attributes->RemoveField(TEXT("unknown"));
            Attributes->RemoveField(TEXT("tooltip"));Row(TEXT("AttributeGroupRequiredKeys"),!NodeForgeGraphCodec::Decode(Wire,Rejected,Error));
            FMemberAttributes TextAttr;TextAttr.Category=TEXT("设置|运动");TextAttr.Tooltip=TEXT("中文\n提示");Row(TEXT("UnicodeCategoryAndNewlineTooltip"),NodeForgeAssetDefinitions::MemberAttributesValid(TextAttr));
            TextAttr.Category=TEXT("设置||运动");Row(TEXT("EmptyCategorySegmentRejected"),!NodeForgeAssetDefinitions::MemberAttributesValid(TextAttr));
            FString Pair;Pair.AppendChar(TCHAR(0xD83D));Pair.AppendChar(TCHAR(0xDE80));TextAttr.Category.Reset();for(int32 I=0;I<128;++I)TextAttr.Category+=Pair;
            Row(TEXT("Unicode128CodepointsNotUTF16Units"),NodeForgeAssetDefinitions::MemberAttributesValid(TextAttr));TextAttr.Category+=Pair;Row(TEXT("Unicode129CodepointsRejected"),!NodeForgeAssetDefinitions::MemberAttributesValid(TextAttr));
        }
        const auto After=ResourceAuditAssets();bool Unchanged=Before.Num()==After.Num();for(const auto& Pair:Before)Unchanged &= After.FindRef(Pair.Key)==Pair.Value;
        Row(TEXT("SavedAssetBytesUnchanged"),Unchanged);
        auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("test"),TEXT("NodeForge.CanonicalResourceGuards.AssetSettings"));Report->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Report->SetArrayField(TEXT("cases"),Rows);Report->SetArrayField(TEXT("actualNativeEvidence"),NativeEvidence);Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
        return NodeForgePrototype::WriteJson(Report,Output) && !HasAnyErrors();
    }

    if(Config->HasTypedField<EJson::Object>(TEXT("blueprintTick")))
    {
        using namespace NodeForgeGraphDocument;
        const auto Before=ResourceAuditAssets();const auto Scoped=Config->GetObjectField(TEXT("blueprintTick"));
        TArray<TSharedPtr<FJsonValue>> Rows,NativeEvidence;
        auto Row=[&](const TCHAR* Name,bool Good){TestTrue(Name,Good);auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),Name);Item->SetBoolField(TEXT("passed"),Good);Rows.Add(MakeShared<FJsonValueObject>(Item));};
        const FString CatalogPath=FPaths::GetPath(Output)/TEXT("NativeTickCatalog.json");
        Row(TEXT("ActualCurrentCatalog"),NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),CatalogPath)==0);
        FString CatalogText;FResourceObject Catalog;
        Row(TEXT("NativeCatalogRead"),FFileHelper::LoadFileToString(CatalogText,*CatalogPath) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(CatalogText),Catalog));
        FString Path,Text,Digest;
        const bool InputRead=Scoped->TryGetStringField(TEXT("inputPath"),Path) && NodeForgePrototype::ReadInputFile(Path,Text,Digest,Error);
        FResourceObject Wire;FGraphDocument Document;
        const bool Decoded=InputRead && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Wire) && Catalog.IsValid();
        if(Decoded) Wire->SetStringField(TEXT("catalogDigest"),Catalog->GetStringField(TEXT("catalogDigest")));
        Row(TEXT("CandidateTemplateDecodedWithActualNativeDigest"),Decoded && NodeForgeGraphCodec::Decode(Wire,Document,Error));
        if(!Document.Graphs.IsEmpty())
        {
            Row(TEXT("TwoEventsValidSameProductionCore"),NodeForgeBlueprintGraph::Validate(Document,Error));
            auto* Native=NodeForgeBlueprintGraph::Build(GetTransientPackage(),TEXT("TickQualification"),Document,Error);FResourceObject Evidence;
            Row(TEXT("TwoEventsNativeBuildMAKEAndNoExtraNodes"),Native && NodeForgeBlueprintGraph::Readback(Native,Document,Evidence,Error));
            if(Native)
            {
                FKismetEditorUtilities::CompileBlueprint(Native);
                Row(TEXT("CompiledTickExactStateReadback"),(Native->Status==BS_UpToDate || Native->Status==BS_UpToDateWithWarnings) && NodeForgeBlueprintGraph::Readback(Native,Document,Evidence,Error));
                if(Evidence)NativeEvidence.Add(MakeShared<FJsonValueObject>(Evidence));
                AActor* CDO=Native->GeneratedClass->GetDefaultObject<AActor>();
                CDO->PrimaryActorTick.TickInterval=1.f;Row(TEXT("CustomTickIntervalRejected"),!NodeForgeBlueprintGraph::Readback(Native,Document,Evidence,Error));CDO->PrimaryActorTick.TickInterval=0.f;
                CDO->PrimaryActorTick.bStartWithTickEnabled=false;Row(TEXT("DisabledStartTickRejected"),!NodeForgeBlueprintGraph::Readback(Native,Document,Evidence,Error));CDO->PrimaryActorTick.bStartWithTickEnabled=true;
                Native->ClearFlags(RF_Public|RF_Standalone);Native->SetFlags(RF_Transient);Native->MarkAsGarbage();
            }
            auto Begin=Document;Begin.Graphs[0].Nodes.RemoveAll([](const auto& N){return N.Binding.Kind==EBindingKind::Event && N.Binding.Member==TEXT("ReceiveTick");});
            Begin.Graphs[0].Edges.RemoveAll([&](const auto& E){return !Begin.Graphs[0].FindNode(E.From) || !Begin.Graphs[0].FindNode(E.To);});
            for(int32 Version:{2,3})
            {
                Begin.CatalogDigest=TEXT("a6a80ffd78ef86d3c8fdfdbb64f8c1b73e9852795d05d9fcbad5ccb93ce939f2");
                FGraphDocument Old;Row(TEXT("Build133OriginalWireBeginPlayRemainsValid"),NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Begin,Version),Old,Error) && NodeForgeBlueprintGraph::Validate(Old,Error));
                auto Tick=Document;Tick.CatalogDigest=Begin.CatalogDigest;FGraphDocument Rejected;
                Row(TEXT("Build133WireTickRejected"),NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Tick,Version),Rejected,Error) && !NodeForgeBlueprintGraph::Validate(Rejected,Error));
            }
            Begin.CatalogDigest=TEXT("3908fac046ddb215e53529a94afe53d3f61f86de0731af02e75738de32edc606");FGraphDocument Old132;
            Row(TEXT("Build132V2Valid"),NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Begin,2),Old132,Error) && NodeForgeBlueprintGraph::Validate(Old132,Error));
            Row(TEXT("Build132V3Rejected"),!NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Begin,3),Old132,Error));
            auto Unknown=Document;Unknown.CatalogDigest=FString::ChrN(64,TEXT('f'));Row(TEXT("UnknownDigestRejected"),!NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Unknown,3),Old132,Error));
            for(const TCHAR* Member:{TEXT("ReceiveBeginPlay"),TEXT("ReceiveTick")})
            {
                auto Bad=Document;auto Duplicate=*Bad.Graphs[0].Nodes.FindByPredicate([&](const auto& N){return N.Binding.Member==Member;});Duplicate.Id=TEXT("Duplicate");Bad.Graphs[0].Nodes.Add(Duplicate);
                Row(TEXT("DuplicateExactEventRejected"),!NodeForgeBlueprintGraph::Validate(Bad,Error));
            }
            auto Bad=Document;Bad.Graphs[0].Nodes[0].Binding.Owner=TEXT("/Script/Engine.Pawn");Row(TEXT("WrongEventOwnerRejected"),!NodeForgeBlueprintGraph::Validate(Bad,Error));
            Bad=Document;Bad.Graphs[0].Nodes[0].Binding.Member=TEXT("ReceiveEndPlay");Row(TEXT("UnknownEventRejected"),!NodeForgeBlueprintGraph::Validate(Bad,Error));
            Bad=Document;Bad.Graphs[0].Edges[0].Output=TEXT("InvalidPin");Row(TEXT("WrongNativePinRejected"),!NodeForgeBlueprintGraph::Build(GetTransientPackage(),TEXT("TickBadPin"),Bad,Error));
            auto NoEvents=Document;NoEvents.Graphs[0].Nodes.RemoveAll([](const auto& N){return N.Binding.Kind==EBindingKind::Event;});NoEvents.Graphs[0].Edges.RemoveAll([&](const auto& E){return !NoEvents.Graphs[0].FindNode(E.From) || !NoEvents.Graphs[0].FindNode(E.To);});Row(TEXT("NoEventShapeStillValid"),NodeForgeBlueprintGraph::Validate(NoEvents,Error));
        }
        const auto After=ResourceAuditAssets();bool Unchanged=Before.Num()==After.Num();for(const auto& Pair:Before)Unchanged &= After.FindRef(Pair.Key)==Pair.Value;
        Row(TEXT("SavedAssetBytesUnchanged"),Unchanged);
        auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("test"),TEXT("NodeForge.CanonicalResourceGuards.BlueprintTick"));Report->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Report->SetArrayField(TEXT("cases"),Rows);Report->SetArrayField(TEXT("actualNativeEvidence"),NativeEvidence);Report->SetStringField(TEXT("templateInputSha256"),Digest);Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
        return NodeForgePrototype::WriteJson(Report,Output) && !HasAnyErrors();
    }
    if(Config->HasTypedField<EJson::Object>(TEXT("jsonArchitecture")))
    {
        // Targeted J verification inside the existing native regression entry.
        // Every constructor/readback below is the production implementation.
        const auto Before=ResourceAuditAssets();const auto Scoped=Config->GetObjectField(TEXT("jsonArchitecture"));
        TArray<TSharedPtr<FJsonValue>> Rows,ActualEvidence,InputProof;
        auto Row=[&](const TCHAR* Name,bool Good){TestTrue(Name,Good);auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),Name);Item->SetBoolField(TEXT("passed"),Good);Rows.Add(MakeShared<FJsonValueObject>(Item));};
        const FString CatalogPath=FPaths::GetPath(Output)/TEXT("NativeJsonCatalog.json");
        Row(TEXT("ActualCurrentCatalog"),NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),CatalogPath)==0);
        auto ReadDocument=[&](const TCHAR* Key,NodeForgeGraphDocument::FGraphDocument& Document)
        {
            FString Path,Text,Digest;
            if(!Scoped->TryGetStringField(Key,Path) || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Path),Root/TEXT("Reports"))
                || !NodeForgePrototype::ReadInputFile(Path,Text,Digest,Error))return false;
            auto Proof=MakeShared<FJsonObject>();Proof->SetStringField(TEXT("path"),Path);Proof->SetStringField(TEXT("sha256"),Digest);InputProof.Add(MakeShared<FJsonValueObject>(Proof));
            return NodeForgeGraphCodec::DecodeText(Text,Document,Error) && Document.RawInputSha256==Digest;
        };
        NodeForgeGraphDocument::FGraphDocument Material,Blueprint,Instance;
        const bool Mat=ReadDocument(TEXT("materialInput"),Material),BP=ReadDocument(TEXT("blueprintInput"),Blueprint),MI=ReadDocument(TEXT("instanceInput"),Instance);
        Row(TEXT("MaterialDecoded"),Mat);Row(TEXT("BlueprintDecoded"),BP);Row(TEXT("InstanceDecoded"),MI);
        for(const auto* Document:{&Material,&Blueprint,&Instance})
        {
            if(Document->CatalogDigest.IsEmpty())continue;
            FString Two,Three;NodeForgeGraphDocument::FGraphDocument D2,D3;
            const bool Encoded=NodeForgeGraphCodec::EncodeText(*Document,2,Two,Error) && NodeForgeGraphCodec::EncodeText(*Document,3,Three,Error);
            Row(TEXT("NativeV2V3LosslessDescription"),Encoded && NodeForgeGraphCodec::DecodeText(Two,D2,Error) && NodeForgeGraphCodec::DecodeText(Three,D3,Error)
                && NodeForgeGraphCodec::EqualOriginal(*Document,D2) && NodeForgeGraphCodec::EqualOriginal(D2,D3) && D2.RawInputSha256!=D3.RawInputSha256);
            const auto Difference=NodeForgeGraphDiff::Compare(D2,D3);
            Row(TEXT("RepresentationHasNoDifference1Delta"),!Difference->GetBoolField(TEXT("requestStructureOrValuesChanged")) && !Difference->GetBoolField(TEXT("layoutChanged")));
        }
        if(Mat)
        {
            auto Changed=Material;Changed.Asset.Get<NodeForgeGraphDocument::FMaterialDefinition>().TwoSided=!Changed.Asset.Get<NodeForgeGraphDocument::FMaterialDefinition>().TwoSided;
            const auto Diff=NodeForgeGraphDiff::Compare(Material,Changed);Row(TEXT("TwoSidedExactSettingsDelta"),Diff->GetArrayField(TEXT("settingsChanged")).Num()==1 && !Diff->GetBoolField(TEXT("layoutChanged")));
            auto Wire=NodeForgeGraphCodec::Encode(Material,3);Wire->SetStringField(TEXT("catalogDigest"),TEXT("3908fac046ddb215e53529a94afe53d3f61f86de0731af02e75738de32edc606"));
            NodeForgeGraphDocument::FGraphDocument Rejected;Row(TEXT("Build132V3Rejected"),!NodeForgeGraphCodec::Decode(Wire,Rejected,Error));
        }
        if(BP && !Blueprint.Graphs.IsEmpty())
        {
            auto Position=Blueprint;Position.Graphs[0].Nodes[0].Position=FIntPoint(17,31);
            const auto Diff=NodeForgeGraphDiff::Compare(Blueprint,Position);Row(TEXT("PositionOnlyLayoutDelta"),Diff->GetBoolField(TEXT("layoutChanged")) && !Diff->GetBoolField(TEXT("requestStructureOrValuesChanged")));
            auto Reordered=Blueprint;Algo::Reverse(Reordered.Graphs[0].Nodes);Row(TEXT("OriginalPreservesNodeOrder"),!NodeForgeGraphCodec::EqualOriginal(Blueprint,Reordered));
            const auto OrderedDiff=NodeForgeGraphDiff::Compare(Blueprint,Reordered);Row(TEXT("DifferenceIgnoresNodeOrder"),!OrderedDiff->GetBoolField(TEXT("requestStructureOrValuesChanged")) && !OrderedDiff->GetBoolField(TEXT("layoutChanged")));
            const auto Wire=NodeForgeGraphCodec::Encode(Blueprint,3);
            auto Reject=[&](const TCHAR* Name,const FResourceObject& Bad){NodeForgeGraphDocument::FGraphDocument Ignored;Row(Name,!NodeForgeGraphCodec::Decode(Bad,Ignored,Error));};
            auto Bad=CopyResourceJson(Wire);Bad->SetStringField(TEXT("assetType"),TEXT("Blueprint"));Reject(TEXT("MixedRootRejected"),Bad);
            Bad=CopyResourceJson(Wire);Bad->SetField(TEXT("target"),MakeShared<FJsonValueNull>());Reject(TEXT("NullTargetRejected"),Bad);
            Bad=CopyResourceJson(Wire);Bad->GetObjectField(TEXT("asset"))->GetObjectField(TEXT("members"))->GetArrayField(TEXT("variables"))[0]->AsObject()->GetObjectField(TEXT("type"))->SetStringField(TEXT("kind"),TEXT("Array"));Reject(TEXT("TypeKindRejected"),Bad);
            FString Text;NodeForgeGraphCodec::EncodeText(Blueprint,3,Text,Error);NodeForgeGraphDocument::FGraphDocument Ignored;
            Row(TEXT("DuplicateKeyRejected"),!NodeForgeGraphCodec::DecodeText(TEXT("{\"format\":\"NodeForgeGraph\",\"format\":\"NodeForgeGraph\"}"),Ignored,Error) && Error.Contains(TEXT("duplicate JSON key")));
            Row(TEXT("Utf8ByteBoundRejected"),!NodeForgeGraphCodec::DecodeText(Text+FString::ChrN(65537,TEXT(' ')),Ignored,Error) && Error.Contains(TEXT("input size")));
            Row(TEXT("DeepJsonRejected"),!NodeForgeGraphCodec::DecodeText(FString::ChrN(33,TEXT('['))+TEXT("0")+FString::ChrN(33,TEXT(']')),Ignored,Error) && Error.Contains(TEXT("nesting")));
        }
        if(MI)
        {
            auto* Native=NodeForgeMaterialInstances::Build(GetTransientPackage(),TEXT("JsonArchitectureTransient"),Instance,Error);FResourceObject Evidence;
            const bool Readback=Native && NodeForgeMaterialInstances::Readback(Native,Instance,Evidence,Error);
            Row(TEXT("ProductionTransientInstanceActualReadback"),Readback && Evidence->GetBoolField(TEXT("exactOverridesAndEffectiveValuesMatch")) && Evidence->GetBoolField(TEXT("parentBytesUnchanged")));
            if(Evidence)ActualEvidence.Add(MakeShared<FJsonValueObject>(Evidence));
            if(Native)
            {
                Row(TEXT("ThreeNativeOverrideArrays"),Native->ScalarParameterValues.Num()==1 && Native->VectorParameterValues.Num()==1 && Native->TextureParameterValues.Num()==1 && Native->GetOutermost()==GetTransientPackage());
                Native->MarkAsGarbage();
            }
            auto Wrong=Instance;Wrong.Asset.Get<NodeForgeGraphDocument::FInstanceDefinition>().ParentAssetSha256=FString::ChrN(64,TEXT('0'));
            Row(TEXT("StaleParentShaRejected"),!NodeForgeMaterialInstances::Validate(Wrong,Error));
        }
        const auto After=ResourceAuditAssets();bool Unchanged=Before.Num()==After.Num() && NodeForgePrototype::FileSha256(Input)==Hash;
        for(const auto& Pair:Before)Unchanged &= After.FindRef(Pair.Key)==Pair.Value;
        Row(TEXT("AllSavedAssetsAndConfigUnchanged"),Unchanged);
        auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("test"),TEXT("NodeForge.CanonicalResourceGuards.JsonArchitecture"));Report->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Report->SetArrayField(TEXT("cases"),Rows);Report->SetArrayField(TEXT("actualNativeEvidence"),ActualEvidence);Report->SetArrayField(TEXT("inputProof"),InputProof);
        Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());Report->SetStringField(TEXT("moduleFileSha256"),NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
        Report->SetStringField(TEXT("scope"),TEXT("Current production codec and Difference/1; transient production instance factory/native override/effective readback and actual saved parent SHA. No asset save, old full batch, UI, runtime or visual test."));
        return NodeForgePrototype::WriteJson(Report,Output) && !HasAnyErrors();
    }
    if(!Config->TryGetArrayField(TEXT("parents"),Parents) || Parents->Num()!=2)
    {AddError(TEXT("Two actual saved test parents required"));return false;}
    const auto BeforeAssets=ResourceAuditAssets();
    const FString Work=FPaths::GetPath(Output)/TEXT("CanonicalEvidence");
    if(IFileManager::Get().DirectoryExists(*Work) || !IFileManager::Get().MakeDirectory(*Work,true))
    {AddError(TEXT("Fresh evidence directory required"));return false;}
    TArray<TSharedPtr<FJsonValue>> Rows;
    auto Row=[&](const FString& Name,bool Passed)
    {TestTrue(Name,Passed);auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),Name);Item->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(Item));};
    auto Read=[&](const FString& Path)->FResourceObject
    {FString Text;FResourceObject Object;if(FFileHelper::LoadFileToString(Text,*Path))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object);return Object;};
    const FString CatalogPath=Work/TEXT("ActualCatalog.json");
    const bool CatalogCreated=NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),CatalogPath)==0;
    const auto CatalogReport=Read(CatalogPath);
    Row(TEXT("ActualNativeCatalogReady"),CatalogCreated && CatalogReport.IsValid());
    if(CatalogCreated && CatalogReport)
    {
        const auto Catalog=CatalogReport->GetObjectField(TEXT("catalog"));
        const FString Digest=CatalogReport->GetStringField(TEXT("catalogDigest"));
        auto Query=MakeShared<FJsonObject>();Query->SetStringField(TEXT("assetKind"),TEXT("Material"));Query->SetStringField(TEXT("query"),TEXT("Multiply"));
        FResourceObject Bundle;
        Row(TEXT("CanonicalTaskRuleAndCaseInsensitiveSearch"),NodeForgeTaskRules::Select(Query,Catalog.ToSharedRef(),Digest,Bundle,Error)
            &&Bundle->GetNumberField(TEXT("matchedNodeCount"))==1);
        Query->SetStringField(TEXT("query"),TEXT("MULTIPLY"));
        Row(TEXT("UppercaseSearchRemainsSupported"),NodeForgeTaskRules::Select(Query,Catalog.ToSharedRef(),Digest,Bundle,Error)
            &&Bundle->GetNumberField(TEXT("matchedNodeCount"))==1);
        Query->SetStringField(TEXT("assetKind"),TEXT("material"));
        Row(TEXT("AssetKindCaseAliasRejectedAndNoStaleBundle"),!NodeForgeTaskRules::Select(Query,Catalog.ToSharedRef(),Digest,Bundle,Error)&&!Bundle);
        Query->RemoveField(TEXT("assetKind"));Query->SetStringField(TEXT("AssetKind"),TEXT("Material"));
        Row(TEXT("TaskRuleFieldAliasRejected"),!NodeForgeTaskRules::Select(Query,Catalog.ToSharedRef(),Digest,Bundle,Error)&&!Bundle);
    }
    const FString FunctionPath=TEXT("/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep");
    auto FunctionConfig=MakeShared<FJsonObject>();FunctionConfig->SetStringField(TEXT("function"),FunctionPath);FResourceObject Signature;
    Row(TEXT("CanonicalSavedFunctionInspected"),NodeForgeMaterialFunctions::Inspect(FunctionConfig,Signature,Error)&&Signature.IsValid());
    if(Signature)
    {
        auto Alias=MakeShared<FJsonObject>();Alias->SetStringField(TEXT("Function"),FunctionPath);FResourceObject Ignored;
        Row(TEXT("FunctionInspectionFieldAliasRejected"),!NodeForgeMaterialFunctions::Inspect(Alias,Ignored,Error));
        FunctionConfig->SetStringField(TEXT("function"),FunctionPath.ToLower());
        Row(TEXT("FunctionObjectPathCaseAliasRejected"),!NodeForgeMaterialFunctions::Inspect(FunctionConfig,Ignored,Error));
        const auto Props=Signature->GetObjectField(TEXT("properties"));
        auto Bind=[&](const FResourceObject& Properties)
        {
            if(!NodeForgeAssetDefinitions::FieldsValid(Properties->Values,NodeForgeAssetDefinitions::EFields::MaterialFunctionPropertiesV2,Error))return false;
            NodeForgeGraphDocument::FNodeBinding Binding;Binding.Kind=NodeForgeGraphDocument::EBindingKind::MaterialFunction;
            Binding.Path=Properties->GetStringField(TEXT("MaterialFunction"));Binding.AssetSha256=Properties->GetStringField(TEXT("FunctionAssetSha256"));
            auto* Call=NewObject<UMaterialExpressionMaterialFunctionCall>(GetTransientPackage());const bool Good=NodeForgeMaterialFunctions::Bind(Call,Binding,Error);Call->MarkAsGarbage();return Good;
        };
        Row(TEXT("CanonicalFunctionBindingAccepted"),Bind(Props));
        auto Wrong=CopyResourceJson(Props);const FString Wanted=Wrong->GetStringField(TEXT("FunctionAssetSha256"));
        Wrong->SetStringField(TEXT("FunctionAssetSha256"),Wanted.ToUpper());
        Row(TEXT("FunctionHashCaseAliasRejected"),!Bind(Wrong));
        Wrong=CopyResourceJson(Props);const FString Value=Wrong->GetStringField(TEXT("MaterialFunction"));
        Wrong->RemoveField(TEXT("MaterialFunction"));Wrong->SetStringField(TEXT("materialFunction"),Value);
        Row(TEXT("FunctionBindingFieldAliasRejected"),!Bind(Wrong));
    }
    auto ValidateInstanceFixture=[&](const FResourceObject& Input,FString& Reason)
    {NodeForgeGraphDocument::FGraphDocument Document;return NodeForgeGraphCodec::Decode(Input,Document,Reason) && NodeForgeMaterialInstances::Validate(Document,Reason);};
    TSet<FString> ParameterKinds;
    for(int32 Index=0;Index<Parents->Num();++Index)
    {
        const FString Parent=(*Parents)[Index]->AsString();auto ParentConfig=MakeShared<FJsonObject>();ParentConfig->SetStringField(TEXT("parent"),Parent);
        FResourceObject Evidence;const FString Prefix=FString::Printf(TEXT("Parent%d"),Index+1);
        Row(Prefix+TEXT("CanonicalInspection"),NodeForgeMaterialInstances::Inspect(ParentConfig,Evidence,Error)&&Evidence.IsValid());
        if(!Evidence)continue;
        auto Alias=MakeShared<FJsonObject>();Alias->SetStringField(TEXT("Parent"),Parent);FResourceObject Ignored;
        Row(Prefix+TEXT("InspectionFieldAliasRejected"),!NodeForgeMaterialInstances::Inspect(Alias,Ignored,Error));
        ParentConfig->SetStringField(TEXT("parent"),Parent.ToLower());
        Row(Prefix+TEXT("ObjectPathCaseAliasRejected"),!NodeForgeMaterialInstances::Inspect(ParentConfig,Ignored,Error));
        auto Settings=MakeShared<FJsonObject>();
        for(const TCHAR* Key:{TEXT("parent"),TEXT("parentAssetSha256"),TEXT("scalarParameters"),TEXT("vectorParameters"),TEXT("textureParameters")})
            Settings->SetField(Key,Evidence->Values.FindChecked(Key));
        auto Request=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Empty;
        Request->SetStringField(TEXT("format"),TEXT("NodeForgeGraph"));Request->SetNumberField(TEXT("version"),2);
        Request->SetStringField(TEXT("requestId"),TEXT("CanonicalInstanceFixture"));Request->SetStringField(TEXT("assetType"),TEXT("MaterialInstance"));
        Request->SetStringField(TEXT("assetPath"),TEXT("/Game/NodeForgeTests/CanonicalInstanceFixture"));Request->SetStringField(TEXT("catalogDigest"),CatalogReport->GetStringField(TEXT("catalogDigest")));
        Request->SetArrayField(TEXT("nodes"),Empty);Request->SetArrayField(TEXT("edges"),Empty);Request->SetObjectField(TEXT("settings"),Settings);
        Row(Prefix+TEXT("CanonicalInstanceRequestAccepted"),ValidateInstanceFixture(Request,Error));
        auto Wrong=CopyResourceJson(Request);Wrong->GetObjectField(TEXT("settings"))->SetStringField(TEXT("parentAssetSha256"),Settings->GetStringField(TEXT("parentAssetSha256")).ToUpper());
        Row(Prefix+TEXT("HashCaseAliasRejected"),!ValidateInstanceFixture(Wrong,Error));
        Wrong=CopyResourceJson(Request);auto WrongSettings=Wrong->GetObjectField(TEXT("settings"));
        const auto ScalarMap=WrongSettings->GetObjectField(TEXT("scalarParameters"));WrongSettings->RemoveField(TEXT("scalarParameters"));WrongSettings->SetObjectField(TEXT("ScalarParameters"),ScalarMap);
        Row(Prefix+TEXT("SettingsFieldAliasRejected"),!ValidateInstanceFixture(Wrong,Error));
        for(const TCHAR* Kind:{TEXT("scalarParameters"),TEXT("vectorParameters"),TEXT("textureParameters")})
        {
            const auto Map=Settings->GetObjectField(Kind);
            if(Map->Values.IsEmpty())continue;
            const auto It=Map->Values.CreateConstIterator();const FString Key=It.Key(),Changed=OtherCase(Key);
            Wrong=CopyResourceJson(Request);const auto ChangedMap=Wrong->GetObjectField(TEXT("settings"))->GetObjectField(Kind);
            const auto Value=ChangedMap->Values.FindChecked(Key);ChangedMap->RemoveField(Key);ChangedMap->SetField(Changed,Value);
            Row(Prefix+Kind+TEXT("ParameterCaseAliasRejected"),!Changed.Equals(Key,ESearchCase::CaseSensitive)&&!ValidateInstanceFixture(Wrong,Error));
            ParameterKinds.Add(Kind);
        }
    }
    Row(TEXT("AllThreeActualParameterKindsCovered"),ParameterKinds.Num()==3);
    // These malformed requests used to reach unchecked AsObject/ID access before the import parser.
    const FString LayoutAsset=(*Parents)[0]->AsString();
    const FString LayoutPath=LayoutAsset.Left(LayoutAsset.Find(TEXT("."),ESearchCase::CaseSensitive,ESearchDir::FromEnd));
    TArray<TSharedPtr<FJsonValue>> InvalidNodes;
    InvalidNodes.Add(MakeShared<FJsonValueNull>());InvalidNodes.Add(MakeShared<FJsonValueNumber>(7));
    InvalidNodes.Add(MakeShared<FJsonValueObject>(MakeShared<FJsonObject>()));
    auto BadId=MakeShared<FJsonObject>();BadId->SetNumberField(TEXT("id"),7);InvalidNodes.Add(MakeShared<FJsonValueObject>(BadId));
    auto BadPosition=MakeShared<FJsonObject>();BadPosition->SetStringField(TEXT("id"),TEXT("Node"));
    BadPosition->SetArrayField(TEXT("position"),{MakeShared<FJsonValueNumber>(0.5),MakeShared<FJsonValueNumber>(0)});
    InvalidNodes.Add(MakeShared<FJsonValueObject>(BadPosition));
    for(int32 Index=0;Index<InvalidNodes.Num();++Index)
    {
        auto Bad=MakeShared<FJsonObject>();Bad->SetStringField(TEXT("assetPath"),LayoutPath);Bad->SetArrayField(TEXT("nodes"),{InvalidNodes[Index]});
        const FString File=Work/(TEXT("LayoutInvalid")+FString::FromInt(Index)+TEXT("Input.json"));
        const FString Result=Work/(TEXT("LayoutInvalid")+FString::FromInt(Index)+TEXT("Result.json"));
        const bool Wrote=NodeForgePrototype::WriteJson(Bad,File);
        const int32 Code=Wrote?NodeForgePrototype::Execute(TEXT("GraphLayoutInspect"),File,Result):-1;
        const auto Failure=Read(Result);
        FString Reason;if(Failure)Failure->TryGetStringField(TEXT("reason"),Reason);
        Row(FString::Printf(TEXT("MalformedLayout%dRejectedBeforeWidgets"),Index),Wrote&&Code==1&&Failure
            &&Failure->GetStringField(TEXT("operationStatus"))==TEXT("failed")
            &&(Reason.Contains(TEXT("node"),ESearchCase::IgnoreCase)||Reason.Contains(TEXT("position"),ESearchCase::IgnoreCase))
            &&!Failure->HasField(TEXT("actualWidgetNodes")));
    }
    const FString RacePath=Work/TEXT("ConcurrentReport.json");
    FEvent* Start=FPlatformProcess::GetSynchEventFromPool(true);
    TArray<TFuture<bool>> Writers;
    for(int32 Index=1;Index<=8;++Index)
        Writers.Add(Async(EAsyncExecution::Thread,[Start,RacePath,Index]()
        {Start->Wait();auto Item=MakeShared<FJsonObject>();Item->SetNumberField(TEXT("writer"),Index);return NodeForgePrototype::WriteJson(Item,RacePath);}));
    Start->Trigger();int32 Successes=0,SuccessfulWriter=0;
    for(int32 Index=0;Index<Writers.Num();++Index)
        if(Writers[Index].Get()){++Successes;SuccessfulWriter=Index+1;}
    FPlatformProcess::ReturnSynchEventToPool(Start);
    const auto Winner=Read(RacePath);
    auto ConcurrentEvidence=MakeShared<FJsonObject>();ConcurrentEvidence->SetNumberField(TEXT("attemptedWriters"),Writers.Num());
    ConcurrentEvidence->SetNumberField(TEXT("successfulWriters"),Successes);ConcurrentEvidence->SetBoolField(TEXT("validFinalJson"),Winner.IsValid());
    ConcurrentEvidence->SetNumberField(TEXT("successfulWriterId"),Successes==1?SuccessfulWriter:0);
    if(Winner)ConcurrentEvidence->SetNumberField(TEXT("winner"),Winner->GetNumberField(TEXT("writer")));
    const FString ConcurrentEvidencePath=Work/TEXT("ConcurrentOutcome.json");
    const bool ConcurrentRecorded=NodeForgePrototype::WriteJson(ConcurrentEvidence,ConcurrentEvidencePath);
    Row(TEXT("ConcurrentReportHasExactlyOneWriter"),Successes==1 && Winner && Winner->GetNumberField(TEXT("writer"))>=1 && Winner->GetNumberField(TEXT("writer"))<=8);
    Row(TEXT("ConcurrentFileMatchesSuccessfulWriter"),Successes==1 && Winner && Winner->GetNumberField(TEXT("writer"))==SuccessfulWriter);
    Row(TEXT("ConcurrentOutcomeRecorded"),ConcurrentRecorded);
    TArray<FString> PendingFiles;
    IFileManager::Get().FindFiles(PendingFiles,*(Work/TEXT("*.pending")),true,false);
    Row(TEXT("OwnReportTemporaryFilesRemoved"),PendingFiles.IsEmpty());
    const FString ReportText=TEXT("{\"overallStatus\":\"succeeded\",\"nextAction\":\"中文\"}");
    const int32 JsonBytes=FTCHARToUTF8(*ReportText).Length();
    for(int32 Size:{1048575,1048576,1048577})
    {
        const FString Path=Work/(FString::FromInt(Size)+TEXT(".json"));
        const FString Content=ReportText+FString::ChrN(Size-JsonBytes,TEXT(' '));
        const bool Saved=FFileHelper::SaveStringToFile(Content,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        FString Original,Description;const bool ReadOk=NodeForgeResultView::Read(Path,Original,Description);
        Row(FString::Printf(TEXT("ReportBytes%d"),Size),Saved&&IFileManager::Get().FileSize(*Path)==Size
            &&(Size<=1048576?(ReadOk&&Original.Equals(Content,ESearchCase::CaseSensitive)):(!ReadOk&&Original.IsEmpty())));
    }
    bool Safe=Hash.Equals(NodeForgePrototype::FileSha256(Input),ESearchCase::CaseSensitive);
    const auto AfterAssets=ResourceAuditAssets();Safe &= BeforeAssets.Num()==AfterAssets.Num();
    for(const auto& Pair:BeforeAssets)Safe &= Pair.Value.Len()==64&&AfterAssets.Contains(Pair.Key)&&Pair.Value.Equals(AfterAssets.FindRef(Pair.Key),ESearchCase::CaseSensitive);
    Row(TEXT("SavedAssetsAndInputUnchanged"),Safe);
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("test"),TEXT("NodeForge.CanonicalResourceGuards"));
    Report->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Report->SetArrayField(TEXT("cases"),Rows);
    Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    const FString Module=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    Report->SetStringField(TEXT("moduleFilename"),Module);Report->SetStringField(TEXT("moduleFileSha256"),NodeForgePrototype::FileSha256(Module));
    Report->SetStringField(TEXT("scope"),TEXT("Actual native catalog/function and saved test-parent preflight identity; bounded result reading. No asset build/save, UI click, persistence or runtime proof."));
    return NodeForgePrototype::WriteJson(Report,Output)&&!HasAnyErrors();
}
#endif
