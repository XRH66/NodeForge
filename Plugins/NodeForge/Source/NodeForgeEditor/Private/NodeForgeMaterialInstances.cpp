// Factory/InitialParent setup adapted from ZiggyMar/unreal-mcp MCPCommandHandler.cpp
// HandleCreateMaterialInstance, fixed d1a85396d9c62328521256005fe7c420725651cc.
// Copyright (c) 2026 Ziggy. MIT: ../../../Licenses/ZiggyMarMIT.txt.
// Uses native factory creation inside our staging package, not AssetTools publication.
// Schema, saved-parent protection, typed preflight and independent override readback are new.
#include "NodeForgeMaterialInstances.h"
#include "NodeForgeAssetDefinitions.h"
#include "NodeForgePrototype.h"
#include "NodeForgeResourceSource.h"
#include "NodeForgeNodeDefinitions.h"
#include "Engine/Texture2D.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace NodeForgeMaterialInstances
{
using namespace NodeForgeGraphDocument;
namespace
{
enum class EKind { Scalar, Vector, Texture };
struct FParsed
{
    UMaterial* Parent=nullptr;
    FString ParentHash;
    FValueMap Scalars,Vectors,Textures;
    NodeForgeResourceSource::FCapture ParentCapture;
    TArray<TUniquePtr<NodeForgeResourceSource::FCapture>> TextureCaptures;
    TMap<FString,UTexture2D*> ResolvedTextures;
};
bool Fail(FString& Error,const FString& Reason) {Error=TEXT("MaterialInstance: ")+Reason;return false;}
bool Fields(const FObject& O,std::initializer_list<const TCHAR*> Names)
{
    if(!O || O->Values.Num()!=Names.size()) return false;
    for(const auto& Entry:O->Values)
    {
        bool Found=false;
        for(const TCHAR* Name:Names) Found |= Entry.Key.Equals(Name,ESearchCase::CaseSensitive);
        if(!Found) return false;
    }
    return true;
}
bool Color(const FValue& V,FLinearColor& Result)
{
    if(!NodeForgeAssetDefinitions::Value(EValueKind::LinearColor,V)) return false;
    const auto& A=V->AsArray();
    Result=FLinearColor(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber(),A[3]->AsNumber());return true;
}
TSharedPtr<FJsonValue> ColorValue(const FLinearColor& C)
{
    TArray<TSharedPtr<FJsonValue>> V;
    for(float N:{C.R,C.G,C.B,C.A}) V.Add(MakeShared<FJsonValueNumber>(N));
    return MakeShared<FJsonValueArray>(V);
}
FString ParentHash(UMaterial* M)
{
    return M?NodeForgePrototype::FileSha256(FPackageName::LongPackageNameToFilename(M->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension())):FString();
}
UMaterial* Parent(const FString& P,FString& Hash,FString& Error,int32 ReadRevision,
    NodeForgeResourceSource::FCapture& Capture,bool RequireOrigin=false)
{
    if(!Capture.Begin(P,TEXT("Material"),Error,ReadRevision,RequireOrigin)) return nullptr;
    UMaterial* M=CastChecked<UMaterial>(Capture.Object());
    if(M->MaterialDomain!=MD_Surface)
    {Fail(Error,TEXT("parent must be a saved, clean, ordinary Surface Material; instance parents/layers are outside this slice"));return nullptr;}
    Hash=Capture.Sha256();
    return M;
}
void Infos(UMaterial* M,EKind Kind,TArray<FMaterialParameterInfo>& Values)
{
    TArray<FGuid> Guids;
    if(Kind==EKind::Scalar) M->GetAllScalarParameterInfo(Values,Guids);
    else if(Kind==EKind::Vector) M->GetAllVectorParameterInfo(Values,Guids);
    else M->GetAllTextureParameterInfo(Values,Guids);
}
bool Known(UMaterial* M,EKind Kind,const FString& Key)
{
    TArray<FMaterialParameterInfo> Values;Infos(M,Kind,Values);
    return Values.ContainsByPredicate([&](const FMaterialParameterInfo& I){return I.Association==EMaterialParameterAssociation::GlobalParameter && I.Index==INDEX_NONE && I.Name.ToString().Equals(Key,ESearchCase::CaseSensitive);});
}
bool Sampling(UMaterial* M,const FString& Key,UTexture2D* T,FString& Error)
{
    // Native recursion and native compiler sampler verification, not a second sampler table.
    TArray<UMaterialExpressionTextureBase*> Expressions;
    M->GetAllExpressionsInMaterialAndFunctionsOfType(Expressions);
    int32 Matches=0;
    for(auto* E:Expressions)
    {
        if(!E || !E->HasAParameterName() || !E->GetParameterName().ToString().Equals(Key,ESearchCase::CaseSensitive)) continue;
        ++Matches;FString NativeError;
        if(!UMaterialExpressionTextureBase::VerifySamplerType(GMaxRHIFeatureLevel,nullptr,T,E->SamplerType,NativeError))
            return Fail(Error,TEXT("texture sampler mismatch for ")+Key+TEXT(" at ")+E->GetPathName()+TEXT(": ")+NativeError);
    }
    return Matches>0?true:Fail(Error,TEXT("no native texture sampler signature for parameter: ")+Key);
}
bool Parse(const FGraphDocument& R,FParsed& P,FString& Error)
{
    const auto* S=R.Asset.TryGet<FInstanceDefinition>();
    if(!S || !R.Graphs.IsEmpty()) return Fail(Error,TEXT("instance requires its typed asset and zero graphs"));
    const int32 ReadRevision=NodeForgeNodeDefinitions::Capabilities(R.CatalogDigest).ResourceReadRevision;
    P.Parent=Parent(S->ParentPath,P.ParentHash,Error,ReadRevision,P.ParentCapture);if(!P.Parent) return false;
    if(!S->ParentAssetSha256.Equals(P.ParentHash,ESearchCase::CaseSensitive)) return Fail(Error,TEXT("parent byte hash mismatch; inspect current saved parent first"));
    P.Scalars=S->Scalars;P.Vectors=S->Vectors;P.Textures=S->Textures;
    TSet<FName> Names;
    for(EKind Kind:{EKind::Scalar,EKind::Vector,EKind::Texture})
    {
        const FValueMap& Map=Kind==EKind::Scalar?P.Scalars:Kind==EKind::Vector?P.Vectors:P.Textures;
        for(const auto& Entry:Map)
        {
            FName Name(*Entry.Key);
            if(Name.IsNone() || Names.Contains(Name) || !Known(P.Parent,Kind,Entry.Key)) return Fail(Error,TEXT("unknown, wrong-kind, noncanonical or duplicate global parameter: ")+Entry.Key);
            Names.Add(Name);double V=0;FLinearColor C;
            if((Kind==EKind::Scalar && !NodeForgeAssetDefinitions::Number(Entry.Value,V,1000000.0)) || (Kind==EKind::Vector && !Color(Entry.Value,C)))
                return Fail(Error,TEXT("invalid typed override value/resource: ")+Entry.Key);
            if(Kind==EKind::Texture)
            {
                FString Path;
                if(!Entry.Value || Entry.Value->Type!=EJson::String || !Entry.Value->TryGetString(Path))
                    return Fail(Error,TEXT("texture override requires exact object path: ")+Entry.Key);
                auto Capture=MakeUnique<NodeForgeResourceSource::FCapture>();
                if(!Capture->Begin(Path,TEXT("Texture2D"),Error,ReadRevision,false)) return false;
                auto* Texture=CastChecked<UTexture2D>(Capture->Object());
                if(!Sampling(P.Parent,Entry.Key,Texture,Error) || !Capture->Verify(Error)) return false;
                P.ResolvedTextures.Add(Entry.Key,Texture);P.TextureCaptures.Add(MoveTemp(Capture));
            }
        }
    }
    return P.ParentCapture.Verify(Error);
}
bool Global(const FMaterialParameterInfo& I) {return I.Association==EMaterialParameterAssociation::GlobalParameter && I.Index==INDEX_NONE;}
}
TSharedRef<FJsonObject> Catalog()
{
    auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("assetType"),TEXT("MaterialInstance"));
    R->SetStringField(TEXT("nativeClass"),UMaterialInstanceConstant::StaticClass()->GetPathName());
    R->SetStringField(TEXT("adapter"),TEXT("NodeForgeMaterialInstance1"));
    R->SetStringField(TEXT("scope"),TEXT("new ordinary constant instance; saved clean Surface Material parent; native global scalar/vector/Texture2D overrides only"));
    R->SetStringField(TEXT("schema"),TEXT("nodes/edges empty; settings exactly parent,parentAssetSha256,scalarParameters,vectorParameters,textureParameters"));
    R->SetStringField(TEXT("discovery"),TEXT("MaterialParameters inspects actual parent parameter names/types/defaults and saved bytes before import"));
    R->SetStringField(TEXT("verification"),TEXT("native setters' bool is not success evidence in UE5.3.2; independently verify exact override arrays, effective values, shader inheritance, save/reopen"));
    R->SetStringField(TEXT("textureSamplingPolicy"),TEXT("ordinary nonvirtual Texture2D; native recursive parameter expressions and VerifySamplerType for every matching sampler; missing signature or incompatible normal/color/sRGB rejected before creation"));
    return R;
}
bool Inspect(const FObject& Config,FObject& Evidence,FString& Error)
{
    if(!Fields(Config,{TEXT("parent")}) || !Config->HasTypedField<EJson::String>(TEXT("parent"))) return Fail(Error,TEXT("inspection accepts exactly parent"));
    NodeForgeResourceSource::FCapture Capture;
    FString Hash;UMaterial* M=Parent(Config->GetStringField(TEXT("parent")),Hash,Error,1,Capture,true);if(!M) return false;
    Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("parent"),M->GetPathName());Evidence->SetStringField(TEXT("parentAssetSha256"),Hash);
    for(EKind Kind:{EKind::Scalar,EKind::Vector,EKind::Texture})
    {
        TArray<FMaterialParameterInfo> Values;Infos(M,Kind,Values);
        Values.Sort([](const FMaterialParameterInfo& A,const FMaterialParameterInfo& B){return A.Name.ToString()<B.Name.ToString();});
        auto Map=MakeShared<FJsonObject>();
        for(const auto& I:Values)
        {
            if(!Global(I)) continue;
            if(Kind==EKind::Scalar) {float V=0;if(!M->GetScalarParameterValue(FHashedMaterialParameterInfo(I),V)) return Fail(Error,TEXT("native scalar default unavailable"));Map->SetNumberField(I.Name.ToString(),V);}
            else if(Kind==EKind::Vector) {FLinearColor V;if(!M->GetVectorParameterValue(FHashedMaterialParameterInfo(I),V)) return Fail(Error,TEXT("native vector default unavailable"));Map->SetField(I.Name.ToString(),ColorValue(V));}
            else {UTexture* V=nullptr;if(!M->GetTextureParameterValue(FHashedMaterialParameterInfo(I),V)) return Fail(Error,TEXT("native texture default unavailable"));if(V) Map->SetStringField(I.Name.ToString(),V->GetPathName());else Map->SetField(I.Name.ToString(),MakeShared<FJsonValueNull>());}
        }
        Evidence->SetObjectField(Kind==EKind::Scalar?TEXT("scalarParameters"):Kind==EKind::Vector?TEXT("vectorParameters"):TEXT("textureParameters"),Map);
    }
    Evidence->SetStringField(TEXT("scope"),TEXT("actual native global parameters only; layers/static switches/instance parents not supported"));
    TArray<UMaterialExpressionTextureBase*> Textures;
    M->GetAllExpressionsInMaterialAndFunctionsOfType(Textures);
    TArray<TSharedPtr<FJsonValue>> Signatures;
    for(auto* E:Textures)
    {
        if(!E || !E->HasAParameterName() || !Known(M,EKind::Texture,E->GetParameterName().ToString())) continue;
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("parameter"),E->GetParameterName().ToString());
        Row->SetStringField(TEXT("expression"),E->GetPathName());
        Row->SetStringField(TEXT("class"),E->GetClass()->GetPathName());
        // GetSamplerTypeEnum is inline over an unexported static in installed UE5.3.2.
        // Use the actual reflected enum already used by the material node catalog.
        UEnum* Enum=FindFProperty<FByteProperty>(E->GetClass(),TEXT("SamplerType"))->Enum;
        Row->SetStringField(TEXT("samplerType"),Enum->GetNameStringByValue(E->SamplerType));
        if(E->Texture)
        {
            Row->SetStringField(TEXT("defaultTexture"),E->Texture->GetPathName());
            Row->SetStringField(TEXT("textureNativeClass"),E->Texture->GetClass()->GetPathName());
            Row->SetStringField(TEXT("defaultTextureSamplerType"),Enum->GetNameStringByValue(UMaterialExpressionTextureBase::GetSamplerTypeForTexture(E->Texture)));
            Row->SetBoolField(TEXT("sRGB"),E->Texture->SRGB);
            Row->SetBoolField(TEXT("virtualTextureStreaming"),E->Texture->VirtualTextureStreaming);
        }
        Signatures.Add(MakeShared<FJsonValueObject>(Row));
    }
    Evidence->SetArrayField(TEXT("textureSamplerSignatures"),Signatures);
    return Capture.Verify(Error) && ParentHash(M)==Hash?true:Fail(Error,TEXT("inspection changed parent hash/dirty state"));
}
bool Validate(const FGraphDocument& R,FString& Error) {FParsed P;return Parse(R,P,Error);}
UMaterialInstanceConstant* Build(UPackage* Package,const FString& Name,const FGraphDocument& R,FString& Error)
{
    FParsed P;if(!Parse(R,P,Error)) return nullptr;
    // Fixed MIT source's native Factory/InitialParent setup; our shared importer owns publication/save.
    auto* Factory=NewObject<UMaterialInstanceConstantFactoryNew>();Factory->InitialParent=P.Parent;
    auto* I=Cast<UMaterialInstanceConstant>(Factory->FactoryCreateNew(UMaterialInstanceConstant::StaticClass(),Package,FName(*Name),RF_Public|RF_Standalone|RF_Transactional,nullptr,GWarn));
    if(!I || I->Parent!=P.Parent) {Fail(Error,TEXT("native factory did not bind exact parent"));return nullptr;}
    for(const auto& E:P.Scalars) {double V=0;NodeForgeAssetDefinitions::Number(E.Value,V,1000000.0);UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(I,FName(*E.Key),V);}
    for(const auto& E:P.Vectors) {FLinearColor V;Color(E.Value,V);UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(I,FName(*E.Key),V);}
    for(const auto& E:P.Textures) UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(I,FName(*E.Key),P.ResolvedTextures.FindChecked(E.Key));
    // The three UE5.3.2 setters return false even after successful native writes. Readback decides.
    FObject Actual;if(!Readback(I,R,Actual,Error)) return nullptr;
    return I;
}
bool Readback(UMaterialInstanceConstant* I,const FGraphDocument& R,FObject& Evidence,FString& Error)
{
    FParsed P;if(!Parse(R,P,Error)) return false;
    if(!I || I->GetClass()!=UMaterialInstanceConstant::StaticClass() || I->Parent!=P.Parent) return Fail(Error,TEXT("actual native instance/parent differs"));
    bool Match=I->ScalarParameterValues.Num()==P.Scalars.Num() && I->VectorParameterValues.Num()==P.Vectors.Num() && I->TextureParameterValues.Num()==P.Textures.Num()
        && I->DoubleVectorParameterValues.IsEmpty() && I->RuntimeVirtualTextureParameterValues.IsEmpty() && I->SparseVolumeTextureParameterValues.IsEmpty();
    auto S=MakeShared<FJsonObject>(),V=MakeShared<FJsonObject>(),T=MakeShared<FJsonObject>();
    TSet<FName> Names;
    for(const auto& E:I->ScalarParameterValues)
    {
        const FString Key=E.ParameterInfo.Name.ToString();double Expected=0;float Effective=0;
        Match &= Global(E.ParameterInfo) && !Names.Contains(E.ParameterInfo.Name) && NodeForgeAssetDefinitions::Number(P.Scalars.FindRef(Key),Expected,1000000.0)
            && E.ParameterValue==static_cast<float>(Expected) && I->GetScalarParameterValue(FHashedMaterialParameterInfo(E.ParameterInfo),Effective) && Effective==E.ParameterValue;
        Names.Add(E.ParameterInfo.Name);S->SetNumberField(Key,E.ParameterValue);
    }
    for(const auto& E:I->VectorParameterValues)
    {
        const FString Key=E.ParameterInfo.Name.ToString();FLinearColor Expected,Effective;
        Match &= Global(E.ParameterInfo) && !Names.Contains(E.ParameterInfo.Name) && Color(P.Vectors.FindRef(Key),Expected)
            && E.ParameterValue==Expected && I->GetVectorParameterValue(FHashedMaterialParameterInfo(E.ParameterInfo),Effective) && Effective==E.ParameterValue;
        Names.Add(E.ParameterInfo.Name);V->SetField(Key,ColorValue(E.ParameterValue));
    }
    for(const auto& E:I->TextureParameterValues)
    {
        const FString Key=E.ParameterInfo.Name.ToString();UTexture* Effective=nullptr;
        Match &= Global(E.ParameterInfo) && !Names.Contains(E.ParameterInfo.Name) && E.ParameterValue && P.ResolvedTextures.FindRef(Key)==E.ParameterValue
            && I->GetTextureParameterValue(FHashedMaterialParameterInfo(E.ParameterInfo),Effective) && Effective==E.ParameterValue;
        Names.Add(E.ParameterInfo.Name);if(E.ParameterValue) T->SetStringField(Key,E.ParameterValue->GetPathName());else T->SetField(Key,MakeShared<FJsonValueNull>());
    }
    auto Settings=MakeShared<FJsonObject>();Settings->SetStringField(TEXT("parent"),I->Parent->GetPathName());Settings->SetStringField(TEXT("parentAssetSha256"),ParentHash(P.Parent));
    Settings->SetObjectField(TEXT("scalarParameters"),S);Settings->SetObjectField(TEXT("vectorParameters"),V);Settings->SetObjectField(TEXT("textureParameters"),T);
    Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("asset"),I->GetPathName());Evidence->SetStringField(TEXT("nativeClass"),I->GetClass()->GetPathName());Evidence->SetObjectField(TEXT("settings"),Settings);
    TArray<TSharedPtr<FJsonValue>> Empty;Evidence->SetArrayField(TEXT("nodes"),Empty);Evidence->SetArrayField(TEXT("edges"),Empty);
    Evidence->SetBoolField(TEXT("exactOverridesAndEffectiveValuesMatch"),Match);Evidence->SetBoolField(TEXT("parentBytesUnchanged"),ParentHash(P.Parent)==P.ParentHash);Evidence->SetBoolField(TEXT("matchesRequest"),Match);
    return Match?true:Fail(Error,TEXT("native override array/effective value readback mismatch"));
}
}
