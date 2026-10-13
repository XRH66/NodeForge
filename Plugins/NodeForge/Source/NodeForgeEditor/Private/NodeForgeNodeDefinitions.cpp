// Shared production node definitions. Existing adapters retain their native construction paths.
#include "NodeForgeNodeDefinitions.h"
#include "NodeForgeAssetDefinitions.h"
#include "NodeForgeMaterialGraph.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Internationalization/Text.h"
#include "Materials/MaterialExpression.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "UObject/NoExportTypes.h"
#include "UObject/Package.h"

namespace NodeForgeNodeDefinitions
{
namespace
{
FString CurrentDigest;
TArray<TSharedPtr<FJsonValue>> FunctionExclusions, MaterialExclusions;
bool Exact(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
void Exclude(TArray<TSharedPtr<FJsonValue>>& Values, const FString& Identity, const FString& Reason)
{
    auto Row = MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("identity"), Identity); Row->SetStringField(TEXT("reason"), Reason);
    Values.Add(MakeShared<FJsonValueObject>(Row));
}

TArray<FBlueprintFunctionDefinition> ExistingFunctions()
{
    TArray<FBlueprintFunctionDefinition> Result = {
        {UKismetSystemLibrary::StaticClass(), TEXT("PrintString")},
        {UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr")},
        {UKismetStringLibrary::StaticClass(), TEXT("Conv_DoubleToString")},
        {UKismetMathLibrary::StaticClass(), TEXT("Add_DoubleDouble")},
        {UKismetMathLibrary::StaticClass(), TEXT("Multiply_DoubleDouble")},
        {UKismetMathLibrary::StaticClass(), TEXT("Greater_DoubleDouble")},
        {UKismetMathLibrary::StaticClass(), TEXT("BooleanAND")},
        {UKismetMathLibrary::StaticClass(), TEXT("MakeVector")},
        {UKismetMathLibrary::StaticClass(), TEXT("BreakVector")},
        {UKismetStringLibrary::StaticClass(), TEXT("Conv_VectorToString")},
        {AActor::StaticClass(), TEXT("K2_GetActorLocation")},
        {AActor::StaticClass(), TEXT("K2_SetActorLocation")}
    };
    auto AddFamily = [&Result](UClass* Owner, const TCHAR* Family, const TCHAR* Members, bool RequiresMathCatalog = false, bool RequiresStringTransformsCatalog = false)
    {
        TArray<FString> Names;
        FString(Members).ParseIntoArrayWS(Names);
        for (const FString& Name : Names) Result.Add({Owner, Name, Family, RequiresStringTransformsCatalog ? 9 : RequiresMathCatalog ? 7 : FString(Family) == TEXT("SelfRootComponent") ? 3 : 2});
    };
    AddFamily(UKismetMathLibrary::StaticClass(), TEXT("IntegerAndScalar"),
        TEXT("Add_IntInt Subtract_IntInt Multiply_IntInt GreaterEqual_IntInt EqualEqual_IntInt Conv_IntToDouble Subtract_DoubleDouble Divide_DoubleDouble FClamp BooleanOR Not_PreBool SelectInt SelectString"));
    AddFamily(UKismetStringLibrary::StaticClass(), TEXT("StringAndName"),
        TEXT("Conv_IntToString Conv_BoolToString Conv_NameToString Conv_RotatorToString Len Contains Mid"));
    AddFamily(UKismetStringLibrary::StaticClass(), TEXT("StringTransforms"), TEXT("Left Right ToUpper ToLower"), false, true);
    AddFamily(UKismetMathLibrary::StaticClass(), TEXT("VectorAndRotation"),
        TEXT("Add_VectorVector Subtract_VectorVector Dot_VectorVector Cross_VectorVector VSize MakeRotator BreakRotator"));
    AddFamily(AActor::StaticClass(), TEXT("SelfActorOrientation"),
        TEXT("K2_GetActorRotation K2_SetActorRotation GetActorScale3D SetActorScale3D GetActorForwardVector GetActorRightVector GetActorUpVector"));
    AddFamily(AActor::StaticClass(), TEXT("SelfRootComponent"), TEXT("K2_GetRootComponent"));
    AddFamily(USceneComponent::StaticClass(), TEXT("SelfRootComponent"),
        TEXT("K2_GetComponentLocation K2_GetComponentRotation K2_GetComponentScale K2_SetWorldLocation K2_SetWorldRotation SetWorldScale3D"));
    AddFamily(UKismetMathLibrary::StaticClass(), TEXT("ScalarMathInterpolation"), TEXT("Abs FMin FMax Lerp DegSin DegCos"), true);
    AddFamily(UKismetMathLibrary::StaticClass(), TEXT("RotationInterpolation"), TEXT("RLerp"), true);
    for (auto& Definition : Result)
        if (Definition.Family.IsEmpty() && (Definition.Owner == AActor::StaticClass()
            || Definition.Member == TEXT("MakeVector") || Definition.Member == TEXT("BreakVector")
            || Definition.Member == TEXT("Conv_VectorToString"))) Definition.MinimumRevision = 1;
    return Result;
}
TArray<FMaterialDefinition> ExistingMaterials()
{
    const TArray<FMaterialDefinition> Result = []
    {
        TArray<FMaterialDefinition> Values;
        auto Add = [&Values](const TCHAR* Suffix, const TCHAR* Properties)
        {
            FMaterialDefinition Spec;
            Spec.Class = FString(TEXT("/Script/Engine.MaterialExpression")) + Suffix;
            FString(Properties).ParseIntoArray(Spec.Properties, TEXT(","), true);
            Spec.Properties.Sort();
            Values.Add(MoveTemp(Spec));
        };
        Add(TEXT("Constant"), TEXT("R"));
        Add(TEXT("Constant2Vector"), TEXT("R,G"));
        Add(TEXT("Constant3Vector"), TEXT("Constant"));
        Add(TEXT("Constant4Vector"), TEXT("Constant"));
        Add(TEXT("ScalarParameter"), TEXT("DefaultValue,ParameterName"));
        Add(TEXT("VectorParameter"), TEXT("DefaultValue,ParameterName"));
        for (const TCHAR* Type : {TEXT("Add"), TEXT("Subtract"), TEXT("Multiply"), TEXT("Divide")})
            Add(Type, TEXT("ConstA,ConstB"));
        Add(TEXT("Power"), TEXT("ConstExponent"));
        Add(TEXT("Sine"), TEXT("Period"));
        Add(TEXT("Cosine"), TEXT("Period"));
        Add(TEXT("Abs"), TEXT(""));
        Add(TEXT("Clamp"), TEXT("MinDefault,MaxDefault"));
        Add(TEXT("OneMinus"), TEXT(""));
        Add(TEXT("Normalize"), TEXT(""));
        Add(TEXT("AppendVector"), TEXT(""));
        Add(TEXT("ComponentMask"), TEXT("R,G,B,A"));
        Add(TEXT("LinearInterpolate"), TEXT("ConstA,ConstB,ConstAlpha"));
        Add(TEXT("TextureCoordinate"), TEXT("CoordinateIndex,UTiling,VTiling,UnMirrorU,UnMirrorV"));
        Add(TEXT("Time"), TEXT("bIgnorePause,bOverride_Period,Period"));
        Add(TEXT("Panner"), TEXT("SpeedX,SpeedY,ConstCoordinate,bFractionalPart"));
        Add(TEXT("TextureSample"), TEXT("Texture,SamplerType,ConstCoordinate"));
        // New families use the same native creation/property/connection/readback path.
        auto Extend = [&Add,&Values](const TCHAR* Suffix, const TCHAR* Properties)
        {
            Add(Suffix,Properties);
            Values.Last().MinimumRevision = 1;
        };
        Extend(TEXT("TextureSampleParameter2D"), TEXT("Texture,SamplerType,ConstCoordinate,ParameterName"));
        Extend(TEXT("Fresnel"), TEXT("Exponent,BaseReflectFraction"));
        for (const TCHAR* Type : {TEXT("Saturate"),TEXT("Floor"),TEXT("Frac"),TEXT("DotProduct"),TEXT("CrossProduct")})
            Extend(Type,TEXT(""));
        Extend(TEXT("MaterialFunctionCall"),TEXT("MaterialFunction,FunctionAssetSha256"));
        Values.Last().RequiresFunctionBinding = true;
        Values.Last().MinimumRevision = 2;
        auto AddCurrentFamily = [&Extend,&Values](const TCHAR* Suffix, const TCHAR* Properties, const TCHAR* Family)
        {
            Extend(Suffix,Properties);
            Values.Last().MinimumRevision = 3;
            Values.Last().Family = Family;
        };
        AddCurrentFamily(TEXT("TextureObject"),TEXT("Texture,SamplerType"),TEXT("TextureObjectComposition"));
        AddCurrentFamily(TEXT("TextureObjectParameter"),TEXT("Texture,SamplerType,ParameterName"),TEXT("TextureObjectComposition"));
        // UE5.3.2 exposes inherited sample inputs, but this native Compile path
        // only returns a texture parameter object and does not consume UV/mip.
        // Reject connections rather than claiming those inherited pins work.
        Values.Last().ConsumesInputs = false;
        AddCurrentFamily(TEXT("Rotator"),TEXT("CenterX,CenterY,Speed,ConstCoordinate"),TEXT("UvTransform"));
        AddCurrentFamily(TEXT("Distance"),TEXT(""),TEXT("DistanceAndThreshold"));
        AddCurrentFamily(TEXT("Step"),TEXT("ConstX,ConstY"),TEXT("DistanceAndThreshold"));
        AddCurrentFamily(TEXT("SmoothStep"),TEXT("ConstMin,ConstMax,ConstValue"),TEXT("DistanceAndThreshold"));
        auto AddSpatial = [&AddCurrentFamily,&Values](const TCHAR* Suffix, const TCHAR* Properties)
        {
            AddCurrentFamily(Suffix,Properties,TEXT("SpatialInputs"));
            Values.Last().MinimumRevision = 4;
        };
        AddSpatial(TEXT("WorldPosition"),TEXT("WorldPositionShaderOffset"));
        AddSpatial(TEXT("ObjectPositionWS"),TEXT("OriginType"));
        for (const TCHAR* Type : {TEXT("CameraPositionWS"),TEXT("CameraVectorWS"),TEXT("VertexNormalWS"),TEXT("PixelNormalWS")})
            AddSpatial(Type,TEXT(""));
        auto AddSelection = [&AddCurrentFamily,&Values](const TCHAR* Suffix, const TCHAR* Properties)
        {
            AddCurrentFamily(Suffix,Properties,TEXT("ColorSelection"));
            Values.Last().MinimumRevision = 5;
        };
        for (const TCHAR* Type : {TEXT("Min"),TEXT("Max")})
            AddSelection(Type,TEXT("ConstA,ConstB"));
        AddSelection(TEXT("If"),TEXT("ConstB,EqualsThreshold"));
        AddSelection(TEXT("Desaturation"),TEXT("LuminanceFactors"));
        Values.Sort([](const FMaterialDefinition& A, const FMaterialDefinition& B) { return A.Class < B.Class; });
        return Values;
    }();
    return Result;
}


bool OrdinaryProperty(const FProperty* Property)
{
    if (!Property || Property->ArrayDim != 1) return false;
    if (CastField<FBoolProperty>(Property) || CastField<FIntProperty>(Property)
        || CastField<FFloatProperty>(Property) || CastField<FDoubleProperty>(Property)
        || CastField<FNameProperty>(Property) || CastField<FStrProperty>(Property)) return true;
    const auto* Struct = CastField<FStructProperty>(Property);
    return Struct && (Struct->Struct == TBaseStructure<FVector>::Get()
        || Struct->Struct == TBaseStructure<FRotator>::Get());
}
FString OrdinaryFunctionReason(UFunction* Function)
{
    if (!Function->HasAllFunctionFlags(FUNC_Native | FUNC_Static | FUNC_BlueprintPure)) return TEXT("requires native static pure computation");
    if (!EligibleExistingFunction(Function)) return TEXT("deprecated/internal/latent/custom-thunk metadata");
    for (const TCHAR* Key : {TEXT("WorldContext"), TEXT("DefaultToSelf"), TEXT("DeterminesOutputType"),
        TEXT("DynamicOutputParam"), TEXT("ExpandEnumAsExecs"), TEXT("ExpandBoolAsExecs"), TEXT("CustomStructureParam")})
        if (Function->HasMetaData(Key)) return FString(TEXT("special semantics: ")) + Key;
    const FString Name = Function->GetName(), Category = Function->GetMetaData(TEXT("Category"));
    if (Category.Contains(TEXT("Random")) || Name.Contains(TEXT("Random")) || Name.Contains(TEXT("Stream"))
        || Name.Contains(TEXT("Seed")) || Name.Contains(TEXT("Shuffle"))) return TEXT("stateful/random computation requires a special policy");
    for (TFieldIterator<FProperty> It(Function, EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        const FProperty* Property = *It;
        if (!Property->HasAnyPropertyFlags(CPF_Parm)) continue;
        if (!OrdinaryProperty(Property)) return TEXT("unsupported native parameter type: ") + Property->GetName() + TEXT(" ") + Property->GetCPPType();
        if (Property->HasAnyPropertyFlags(CPF_ReferenceParm) && !Property->HasAnyPropertyFlags(CPF_ConstParm))
            return TEXT("mutable input reference requires a separate semantics adapter: ") + Property->GetName();
    }
    return FString();
}
TArray<FBlueprintFunctionDefinition>& FunctionStorage()
{
    check(IsInGameThread());
    static TArray<FBlueprintFunctionDefinition> Values = []
    {
        auto Result = ExistingFunctions();
        for (UClass* Owner : {UKismetMathLibrary::StaticClass(), UKismetStringLibrary::StaticClass()})
        {
            TArray<UFunction*> Candidates;
            for (TFieldIterator<UFunction> It(Owner, EFieldIteratorFlags::ExcludeSuper); It; ++It) Candidates.Add(*It);
            Candidates.Sort([](const UFunction& A, const UFunction& B) { return A.GetName() < B.GetName(); });
            for (UFunction* Function : Candidates)
            {
                const FString Name = Function->GetName();
                if (Result.ContainsByPredicate([&](const auto& D) { return D.Owner == Owner && Exact(D.Member, Name); })) continue;
                const FString Reason = OrdinaryFunctionReason(Function);
                if (!Reason.IsEmpty()) { Exclude(FunctionExclusions, Owner->GetPathName() + TEXT(".") + Name, Reason); continue; }
                FBlueprintFunctionDefinition Definition{Owner, Name, TEXT("OrdinaryPureComputation"), 10};
                Definition.IsBulk = true; Definition.Admission = EAdmission::PendingNative;
                Result.Add(MoveTemp(Definition));
            }
        }
        Result.Sort([](const auto& A, const auto& B)
        {
            const FString X = A.Owner->GetPathName(), Y = B.Owner->GetPathName();
            return X == Y ? A.Member < B.Member : X < Y;
        });
        return Result;
    }();
    return Values;
}
TArray<FMaterialDefinition>& MaterialStorage()
{
    check(IsInGameThread());
    static TArray<FMaterialDefinition> Values = []
    {
        auto Result = ExistingMaterials();
        TArray<UClass*> Candidates;
        for (TObjectIterator<UClass> It; It; ++It)
            if (It->GetSuperClass() == UMaterialExpression::StaticClass()
                && It->GetOutermost()->GetName() == TEXT("/Script/Engine")) Candidates.Add(*It);
        Candidates.Sort([](const UClass& A, const UClass& B) { return A.GetPathName() < B.GetPathName(); });
        for (UClass* Class : Candidates)
        {
            const FString Name = Class->GetPathName();
            if (Result.ContainsByPredicate([&](const auto& D) { return Exact(D.Class, Name); })) continue;
            if (!Class->HasAnyClassFlags(CLASS_Native) || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
            { Exclude(MaterialExclusions, Name, TEXT("not a concrete native ordinary expression")); continue; }
            UMaterialExpression* Defaults = Class->GetDefaultObject<UMaterialExpression>();
            bool IsMath = false;
            for (const FText& Category : Defaults->MenuCategories)
                if (const FString* Source = FTextInspector::GetSourceString(Category)) IsMath |= Exact(*Source, TEXT("Math"));
            if (!IsMath) { Exclude(MaterialExclusions, Name, TEXT("outside the ordinary native Math expression category")); continue; }
            FMaterialDefinition Definition;
            Definition.Class = Name; Definition.MinimumRevision = 6;
            Definition.Family = TEXT("OrdinarySurfaceMath"); Definition.IsBulk = true;
            Definition.Admission = EAdmission::PendingNative;
            FString Reason;
            for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
            {
                FProperty* Property = *It;
                if (auto* Input = CastField<FStructProperty>(Property); Input && Input->Struct->GetFName() == NAME_ExpressionInput && Input->ArrayDim == 1) continue;
                if (Property->HasAnyPropertyFlags(CPF_Transient)) continue;
                const bool Scalar = Property->ArrayDim == 1 && (CastField<FBoolProperty>(Property)
                    || CastField<FIntProperty>(Property) || CastField<FFloatProperty>(Property) || CastField<FDoubleProperty>(Property));
                if (!Scalar || !Property->HasAnyPropertyFlags(CPF_Edit))
                { Reason = TEXT("requires special/default-only property semantics: ") + Property->GetName() + TEXT(" ") + Property->GetCPPType(); break; }
                Definition.Properties.Add(Property->GetName());
            }
            if (!Reason.IsEmpty()) { Exclude(MaterialExclusions, Name, Reason); continue; }
            Definition.Properties.Sort(); Result.Add(MoveTemp(Definition));
        }
        Result.Sort([](const auto& A, const auto& B) { return A.Class < B.Class; });
        return Result;
    }();
    return Values;
}
struct FArchive { const FString* Digest; int32 Material; int32 Blueprint; bool Instance; int32 MaximumGraphVersion = 2; };
const TArray<FArchive>& Archives()
{
    using namespace NodeForgeMaterialGraph;
    static const FString Build131Digest(TEXT("715528fbc5e20f55115457238f04eee9bc49354e602f9297f90f6b7c8ad1c6f3"));
    static const FString Build132Digest(TEXT("3908fac046ddb215e53529a94afe53d3f61f86de0731af02e75738de32edc606"));
    static const FString Build133Digest(TEXT("a6a80ffd78ef86d3c8fdfdbb64f8c1b73e9852795d05d9fcbad5ccb93ce939f2"));
    static const FString Build134Digest(TEXT("6b9074116e09b256da2bc4bb149e8083d1af07407b70541550433534dcfea4db"));
    static const FString Build135Digest(TEXT("ff94bd6ad0895356478184a5a503fe829e8f8fe0d85007fdd3de8ca2c1390c10"));
    static const TArray<FArchive> Values = {
        {&LegacyCatalogDigest,0,0,false}, {&MaterialFamiliesCatalogDigest,1,0,false},
        {&MaterialInstancesCatalogDigest,1,0,false}, {&TextureSamplersCatalogDigest,1,0,true},
        {&MaterialFunctionsCatalogDigest,2,0,true}, {&BlueprintMotionCatalogDigest,2,1,true},
        {&BlueprintFunctionFamiliesCatalogDigest,2,2,true}, {&TextureFunctionsCatalogDigest,3,2,true},
        {&BlueprintComponentsCatalogDigest,3,3,true}, {&MemberScalarsCatalogDigest,3,4,true},
        {&BlueprintSequenceCatalogDigest,3,5,true}, {&SpatialInputsCatalogDigest,4,5,true},
        {&RotatorMembersCatalogDigest,4,6,true}, {&ColorSelectionCatalogDigest,5,6,true},
        {&MathInterpolationCatalogDigest,5,7,true}, {&BlueprintSwitchesCatalogDigest,5,8,true},
        {&StringIdentityCatalogDigest,5,8,true}, {&Build131Digest,5,9,true}, {&Build132Digest,6,10,true}, {&Build133Digest,6,10,true,3}, {&Build134Digest,6,11,true,3}, {&Build135Digest,7,12,true,3}
    };
    return Values;
}
}

bool EligibleExistingFunction(const UFunction* Function)
{
    return Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure)
        && (Function->HasAnyFunctionFlags(FUNC_Static) || Function->GetOuterUClass() == AActor::StaticClass()
            || Function->GetOuterUClass() == USceneComponent::StaticClass())
        && !Function->HasMetaData(TEXT("DeprecatedFunction")) && !Function->HasMetaData(TEXT("BlueprintInternalUseOnly"))
        && !Function->HasMetaData(TEXT("ArrayParm")) && !Function->HasMetaData(TEXT("ArrayParam"))
        && !Function->HasMetaData(TEXT("CustomThunk")) && !Function->HasMetaData(TEXT("Latent"));
}
const TArray<FBlueprintEventDefinition>& Events()
{
    static const TArray<FBlueprintEventDefinition> Values = {
        {AActor::StaticClass(), TEXT("ReceiveBeginPlay"), 0, false},
        {AActor::StaticClass(), TEXT("ReceiveTick"), 11, true}
    };
    return Values;
}
const FBlueprintEventDefinition* Event(const FString& Owner, const FString& Member)
{
    return Events().FindByPredicate([&](const auto& D) { return Exact(D.Owner->GetPathName(), Owner) && Exact(D.Member, Member); });
}
const TArray<FBlueprintFunctionDefinition>& Functions() { return FunctionStorage(); }
const TArray<FMaterialDefinition>& Materials() { return MaterialStorage(); }
const TArray<FSpecialDefinition>& Specials()
{
    static const TArray<FSpecialDefinition> Values = {
        {TEXT("/Script/BlueprintGraph.K2Node_Event"),0,NodeForgeGraphDocument::EBindingKind::Event},
        {TEXT("/Script/BlueprintGraph.K2Node_CallFunction"),0,NodeForgeGraphDocument::EBindingKind::Function},
        {TEXT("/Script/BlueprintGraph.K2Node_IfThenElse"),0},
        {TEXT("/Script/BlueprintGraph.K2Node_VariableGet"),0,NodeForgeGraphDocument::EBindingKind::Member},
        {TEXT("/Script/BlueprintGraph.K2Node_VariableSet"),0,NodeForgeGraphDocument::EBindingKind::Member}, {TEXT("/Script/BlueprintGraph.K2Node_ExecutionSequence"),5},
        {TEXT("/Script/BlueprintGraph.K2Node_SwitchInteger"),8}, {TEXT("/Script/BlueprintGraph.K2Node_SwitchName"),8}
    };
    return Values;
}
const TArray<FMemberDefinition>& Members()
{
    static const TArray<FMemberDefinition> Values = [] {
        TArray<FMemberDefinition> Result;
        for (const auto& Type : NodeForgeAssetDefinitions::MemberTypes()) Result.Add({Type.Name, Type.MinimumRevision});
        return Result;
    }();
    return Values;
}
const FBlueprintFunctionDefinition* Function(const FString& Owner, const FString& Member)
{
    return Functions().FindByPredicate([&](const auto& D) { return Exact(D.Owner->GetPathName(), Owner) && Exact(D.Member, Member); });
}
const FMaterialDefinition* Material(const FString& Class)
{
    return Materials().FindByPredicate([&](const auto& D) { return Exact(D.Class, Class); });
}
const FSpecialDefinition* Special(const FString& Class)
{
    return Specials().FindByPredicate([&](const auto& D) { return Exact(D.Class, Class); });
}
int32 MemberMinimumRevision(const FString& Type)
{
    const auto* Definition = Members().FindByPredicate([&](const auto& D) { return Exact(D.Type, Type); });
    return Definition ? Definition->MinimumRevision : MAX_int32;
}
void QualifyFunction(const FString& Owner, const FString& Member, const FString& Error)
{
    for (auto& D : FunctionStorage()) if (Exact(D.Owner->GetPathName(), Owner) && Exact(D.Member, Member))
    { D.Admission = Error.IsEmpty() ? EAdmission::Admitted : EAdmission::Rejected; D.QualificationError = Error; return; }
}
void QualifyMaterial(const FString& Class, const FString& Error)
{
    for (auto& D : MaterialStorage()) if (Exact(D.Class, Class))
    { D.Admission = Error.IsEmpty() ? EAdmission::Admitted : EAdmission::Rejected; D.QualificationError = Error; return; }
}
TArray<TSharedPtr<FJsonValue>> Exclusions(const FString& AssetType)
{
    TArray<TSharedPtr<FJsonValue>> Result;
    if (AssetType == TEXT("Blueprint"))
    {
        Functions();
        Result = FunctionExclusions;
        for (const auto& D : Functions()) if (D.Admission == EAdmission::Rejected)
            Exclude(Result, D.Owner->GetPathName() + TEXT(".") + D.Member, D.QualificationError);
    }
    else
    {
        Materials();
        Result = MaterialExclusions;
        for (const auto& D : Materials()) if (D.Admission == EAdmission::Rejected) Exclude(Result, D.Class, D.QualificationError);
    }
    return Result;
}
FCapabilities Capabilities(const FString& Digest, EContext Context)
{
    if (Context == EContext::NativeCatalogProbe) return {7,12,true,3,1};
    if (!CurrentDigest.IsEmpty() && Exact(Digest, CurrentDigest)) return {7,12,true,3,1};
    for (const auto& Profile : Archives()) if (Exact(Digest, *Profile.Digest)) return {Profile.Material,Profile.Blueprint,Profile.Instance,Profile.MaximumGraphVersion};
    return {};
}
bool AllowsRequest(const FString& Digest, const FString& AssetType, int32 Version, const FString& ActualDigest)
{
    if (Digest.IsEmpty()) return false;
    if (Exact(Digest, ActualDigest)) return (Version == 1 || Version == 2 || Version == 3)
        && (AssetType == TEXT("Material") || AssetType == TEXT("Blueprint")
        || ((Version == 2 || Version == 3) && AssetType == TEXT("MaterialInstance")));
    const auto Profile = Capabilities(Digest);
    if (!AllowsGraphVersion(Digest, Version)) return false;
    return Profile.IsKnown() && (AssetType == TEXT("Material") || AssetType == TEXT("Blueprint")
        || (Profile.AllowsMaterialInstance && AssetType == TEXT("MaterialInstance")));
}
bool AllowsGraphVersion(const FString& Digest, int32 Version, EContext Context)
{
    if (Version != 2 && Version != 3) return false;
    if (Context == EContext::NativeCatalogProbe) return true;
    if (!CurrentDigest.IsEmpty() && Exact(CurrentDigest, Digest)) return true;
    const auto Profile = Capabilities(Digest);
    return Profile.IsKnown() && Version <= Profile.MaximumGraphVersion;
}
void RegisterCurrentDigest(const FString& Digest)
{
    check(IsInGameThread()); check(!Digest.IsEmpty());
    check(CurrentDigest.IsEmpty() || Exact(CurrentDigest, Digest)); CurrentDigest = Digest;
}
}
