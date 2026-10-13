// Native UE5.3.2 SetMaterialFunction/GetInputsView/GetOutputs own dynamic connectors.
// This small adapter adds saved-reference validation and independent signature readback.
// Existing fixed MIT ChiR24 ConnectNodes adaptation remains the shared connection backend.
#include "NodeForgeMaterialFunctions.h"
#include "NodeForgePrototype.h"
#include "NodeForgeResourceSource.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialFunction.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace NodeForgeMaterialFunctions
{
namespace
{
bool Fail(FString& Error,const FString& Message) {Error=TEXT("MaterialFunction: ")+Message;return false;}
bool ExactFields(const FObject& Object,std::initializer_list<const TCHAR*> Names)
{
    if(!Object || Object->Values.Num()!=Names.size()) return false;
    for(const auto& Entry:Object->Values)
    {
        bool Found=false;
        for(const TCHAR* Name:Names) Found |= Entry.Key.Equals(Name,ESearchCase::CaseSensitive);
        if(!Found) return false;
    }
    return true;
}
FString Hash(UMaterialFunctionInterface* F)
{
    return F?NodeForgePrototype::FileSha256(FPackageName::LongPackageNameToFilename(F->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension())):FString();
}
bool Ports(UMaterialExpressionMaterialFunctionCall* Call,FString& Error)
{
    if(!Call || !Call->MaterialFunction || Call->FunctionInputs.Num()>64 || Call->FunctionOutputs.Num()<1 || Call->FunctionOutputs.Num()>64)
        return Fail(Error,TEXT("native function requires 0..64 inputs and 1..64 outputs"));
    TSet<FGuid> Ids;TSet<FName> Names;
    for(const auto& I:Call->FunctionInputs)
    {
        if(!I.ExpressionInput || !I.ExpressionInputId.IsValid() || I.ExpressionInput->Id!=I.ExpressionInputId
            || I.ExpressionInput->InputName.IsNone() || Ids.Contains(I.ExpressionInputId) || Names.Contains(I.ExpressionInput->InputName))
            return Fail(Error,TEXT("missing/duplicate native input identity"));
        Ids.Add(I.ExpressionInputId);Names.Add(I.ExpressionInput->InputName);
    }
    Ids.Reset();
    for(const auto& O:Call->FunctionOutputs)
    {
        if(!O.ExpressionOutput || !O.ExpressionOutputId.IsValid() || O.ExpressionOutput->Id!=O.ExpressionOutputId || Ids.Contains(O.ExpressionOutputId))
            return Fail(Error,TEXT("missing/duplicate native output identity"));
        Ids.Add(O.ExpressionOutputId);
    }
    return Call->GetInputsView().Num()==Call->FunctionInputs.Num() && Call->GetOutputs().Num()==Call->FunctionOutputs.Num()
        ?true:Fail(Error,TEXT("native call connector count differs from function signature"));
}
}
bool Bind(UMaterialExpressionMaterialFunctionCall* Call,const NodeForgeGraphDocument::FNodeBinding& B,FString& Error,int32 ReadRevision)
{
    const FString& Path=B.Path; const FString& Wanted=B.AssetSha256;
    if(B.Kind!=NodeForgeGraphDocument::EBindingKind::MaterialFunction)
        return Fail(Error,TEXT("material function binding required; inspect first"));
    NodeForgeResourceSource::FCapture Capture;
    if(!Capture.Begin(Path,TEXT("MaterialFunction"),Error,ReadRevision,false)) return false;
    auto* F=CastChecked<UMaterialFunction>(Capture.Object());
    if(!Wanted.Equals(Hash(F),ESearchCase::CaseSensitive)) return Fail(Error,TEXT("saved function byte hash mismatch"));
    if(!Call || !Call->SetMaterialFunction(F) || Call->MaterialFunction!=F || !Ports(Call,Error))
        return Error.IsEmpty()?Fail(Error,TEXT("native function binding failed")):false;
    return Capture.Verify(Error) && Hash(F).Equals(Wanted,ESearchCase::CaseSensitive)?true:Fail(Error,TEXT("function changed during native binding"));
}
NodeForgeGraphDocument::FNodeBinding Binding(UMaterialExpressionMaterialFunctionCall* Call)
{
    NodeForgeGraphDocument::FNodeBinding Result;
    Result.Kind=NodeForgeGraphDocument::EBindingKind::MaterialFunction;
    if(Call && Call->MaterialFunction) {Result.Path=Call->MaterialFunction->GetPathName();Result.AssetSha256=Hash(Call->MaterialFunction);}
    return Result;
}
FObject Properties(UMaterialExpressionMaterialFunctionCall* Call)
{
    auto R=MakeShared<FJsonObject>();
    if(Call && Call->MaterialFunction)
    {
        R->SetStringField(TEXT("MaterialFunction"),Call->MaterialFunction->GetPathName());
        R->SetStringField(TEXT("FunctionAssetSha256"),Hash(Call->MaterialFunction));
    }
    return R;
}
FObject Signature(UMaterialExpressionMaterialFunctionCall* Call)
{
    auto R=MakeShared<FJsonObject>();if(!Call) return R;
    R->SetObjectField(TEXT("properties"),Properties(Call));
    TArray<TSharedPtr<FJsonValue>> Inputs,Outputs;
    for(int32 I=0;I<Call->FunctionInputs.Num();++I)
    {
        const auto& Native=Call->FunctionInputs[I];auto P=MakeShared<FJsonObject>();
        P->SetStringField(TEXT("name"),Call->GetInputName(I).ToString());
        P->SetStringField(TEXT("guid"),Native.ExpressionInputId.ToString(EGuidFormats::DigitsWithHyphens));
        P->SetNumberField(TEXT("nativeType"),Call->GetInputType(I));
        P->SetBoolField(TEXT("required"),Call->IsInputConnectionRequired(I));
        if(Native.ExpressionInput)
        {
            P->SetBoolField(TEXT("usesNativePreviewDefault"),Native.ExpressionInput->bUsePreviewValueAsDefault);
            const auto V=Native.ExpressionInput->PreviewValue;
            P->SetArrayField(TEXT("nativePreviewValue"),{MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z),MakeShared<FJsonValueNumber>(V.W)});
        }
        Inputs.Add(MakeShared<FJsonValueObject>(P));
    }
    for(int32 I=0;I<Call->FunctionOutputs.Num();++I)
    {
        const auto& Native=Call->FunctionOutputs[I];auto P=MakeShared<FJsonObject>();
        P->SetStringField(TEXT("index"),FString::FromInt(I));P->SetStringField(TEXT("name"),Native.Output.OutputName.ToString());
        P->SetStringField(TEXT("guid"),Native.ExpressionOutputId.ToString(EGuidFormats::DigitsWithHyphens));
        Outputs.Add(MakeShared<FJsonValueObject>(P));
    }
    R->SetArrayField(TEXT("inputs"),Inputs);R->SetArrayField(TEXT("outputs"),Outputs);
    R->SetStringField(TEXT("defaultPolicy"),TEXT("required inputs must be connected; optional unconnected inputs use native function preview defaults; no per-call default adapter"));
    R->SetStringField(TEXT("hashScope"),TEXT("direct saved function file only; nested function/texture dependencies are not recursively hash-locked"));
    return R;
}
bool Inspect(const FObject& Config,FObject& Evidence,FString& Error)
{
    FString Path;if(!ExactFields(Config,{TEXT("function")}) || !Config->TryGetStringField(TEXT("function"),Path)) return Fail(Error,TEXT("inspection accepts exactly function"));
    NodeForgeResourceSource::FCapture Capture;
    if(!Capture.Begin(Path,TEXT("MaterialFunction"),Error)) return false;
    auto* F=CastChecked<UMaterialFunction>(Capture.Object());
    const FString Before=Hash(F);
    auto* Call=NewObject<UMaterialExpressionMaterialFunctionCall>(GetTransientPackage());
    NodeForgeGraphDocument::FNodeBinding P; P.Kind=NodeForgeGraphDocument::EBindingKind::MaterialFunction; P.Path=Path;P.AssetSha256=Before;
    const bool Good=Bind(Call,P,Error);if(Good) Evidence=Signature(Call);
    Call->MarkAsGarbage();
    return Good && Capture.Verify(Error) && Hash(F)==Before;
}
bool Matches(UMaterialExpressionMaterialFunctionCall* A,UMaterialExpressionMaterialFunctionCall* B)
{
    FString Error;
    if(!Ports(A,Error) || !Ports(B,Error) || A->MaterialFunction!=B->MaterialFunction
        || A->FunctionInputs.Num()!=B->FunctionInputs.Num() || A->FunctionOutputs.Num()!=B->FunctionOutputs.Num()) return false;
    for(int32 I=0;I<A->FunctionInputs.Num();++I)
        if(A->FunctionInputs[I].ExpressionInputId!=B->FunctionInputs[I].ExpressionInputId || A->GetInputName(I)!=B->GetInputName(I) || A->GetInputType(I)!=B->GetInputType(I)) return false;
    for(int32 I=0;I<A->FunctionOutputs.Num();++I)
        if(A->FunctionOutputs[I].ExpressionOutputId!=B->FunctionOutputs[I].ExpressionOutputId || A->FunctionOutputs[I].Output.OutputName!=B->FunctionOutputs[I].Output.OutputName) return false;
    return true;
}
}
