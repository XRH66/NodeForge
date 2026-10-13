// Project-owned request delta; native JSON primitives with exact strings/keys, validated graph inputs.
#include "NodeForgeGraphDiff.h"
#include "NodeForgeGraphCodec.h"
#include "Dom/JsonValue.h"

namespace
{
using FValue=TSharedPtr<FJsonValue>;
using FObject=TSharedPtr<FJsonObject>;
using FArray=TArray<FValue>;
bool ExactValue(const FValue& A,const FValue& B,int32 Depth)
{
    if(A.IsValid()!=B.IsValid() || Depth>64)return false;
    if(!A)return true;
    if(A->Type!=B->Type)return false;
    if(A->Type==EJson::String)return A->AsString().Equals(B->AsString(),ESearchCase::CaseSensitive);
    if(A->Type==EJson::Array)
    {
        const auto& X=A->AsArray();const auto& Y=B->AsArray();if(X.Num()!=Y.Num())return false;
        for(int32 I=0;I<X.Num();++I)if(!ExactValue(X[I],Y[I],Depth+1))return false;
        return true;
    }
    if(A->Type==EJson::Object)
    {
        const auto X=A->AsObject(),Y=B->AsObject();if(!X||!Y||X->Values.Num()!=Y->Values.Num())return false;
        for(const auto& Pair:X->Values)
        {
            const FValue* Match=nullptr;
            for(const auto& Other:Y->Values)if(Pair.Key.Equals(Other.Key,ESearchCase::CaseSensitive)){Match=&Other.Value;break;}
            if(!Match||!ExactValue(Pair.Value,*Match,Depth+1))return false;
        }
        return true;
    }
    return FJsonValue::CompareEqual(*A,*B);
}
bool Equal(const FValue& A,const FValue& B)
{
    return NodeForgeGraphDiff::EqualValue(A,B);
}
void AddExact(TArray<FString>& Names,const FString& Name)
{
    if(!Names.ContainsByPredicate([&](const FString& Existing){return Existing.Equals(Name,ESearchCase::CaseSensitive);}))Names.Add(Name);
}
const FObject* FindNode(const TMap<FString,FObject>& Values,const FString& Id)
{
    for(const auto& Pair:Values)if(Pair.Key.Equals(Id,ESearchCase::CaseSensitive))return &Pair.Value;
    return nullptr;
}
FValue Field(const FObject& Object,const FString& Name)
{
    if(Object)for(const auto& Pair:Object->Values)if(Pair.Key.Equals(Name,ESearchCase::CaseSensitive))return Pair.Value;
    return FValue();
}
FArray Fields(const FObject& Before,const FObject& After,bool Layout)
{
    TArray<FString> Sorted;
    if(Before)for(const auto& Pair:Before->Values)AddExact(Sorted,Pair.Key);
    if(After)for(const auto& Pair:After->Values)AddExact(Sorted,Pair.Key);
    Sorted.Sort([](const FString& A,const FString& B){return A.Compare(B,ESearchCase::CaseSensitive)<0;});FArray Result;
    for(const FString& Name:Sorted)
    {
        if(Name==TEXT("id") || (Name==TEXT("position"))!=Layout)continue;
        const FValue A=Field(Before,Name),B=Field(After,Name);
        if(Equal(A,B))continue;
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("field"),Name);
        Row->SetBoolField(TEXT("beforePresent"),A.IsValid());Row->SetBoolField(TEXT("afterPresent"),B.IsValid());
        Row->SetField(TEXT("before"),A?FJsonValue::Duplicate(A):MakeShared<FJsonValueNull>());
        Row->SetField(TEXT("after"),B?FJsonValue::Duplicate(B):MakeShared<FJsonValueNull>());
        Result.Add(MakeShared<FJsonValueObject>(Row));
    }
    return Result;
}
TMap<FString,FObject> Nodes(const FObject& Request)
{
    TMap<FString,FObject> Result;
    for(const auto& Value:Request->GetArrayField(TEXT("nodes")))
    {const FObject Node=Value->AsObject();Result.Add(Node->GetStringField(TEXT("id")),Node);}
    return Result;
}
FArray Difference(const FArray& A,const FArray& B)
{
    FArray Result;
    for(const FValue& Value:A)
        if(!B.ContainsByPredicate([&](const FValue& Other){return Equal(Value,Other);}))
            Result.Add(FJsonValue::Duplicate(Value));
    return Result;
}
FValue Settings(const FObject& Request)
{
    FValue Result=FJsonValue::Duplicate(Field(Request,TEXT("settings")));
    // Variable declaration order does not define node identity or runtime behavior.
    const FObject Object=Result->AsObject();const FArray* Variables=nullptr;
    if(Object->TryGetArrayField(TEXT("variables"),Variables))
    {
        FArray Sorted=*Variables;
        Sorted.Sort([](const FValue& A,const FValue& B){return A->AsObject()->GetStringField(TEXT("name")).Compare(B->AsObject()->GetStringField(TEXT("name")),ESearchCase::CaseSensitive)<0;});
        Object->SetArrayField(TEXT("variables"),Sorted);
    }
    return Result;
}
}

bool NodeForgeGraphDiff::EqualValue(const FValue& A,const FValue& B)
{
    return ExactValue(A,B,0);
}

TSharedRef<FJsonObject> NodeForgeGraphDiff::Compare(const NodeForgeGraphDocument::FGraphDocument& BeforeDocument,const NodeForgeGraphDocument::FGraphDocument& AfterDocument)
{
    // Difference/1 retains its established logical field paths. This projection
    // is only the comparison/output representation, never the execution path.
    const FObject Before=NodeForgeGraphCodec::Encode(BeforeDocument,2),After=NodeForgeGraphCodec::Encode(AfterDocument,2);
    const auto A=Nodes(Before),B=Nodes(After);TArray<FString> Ids;
    for(const auto& Pair:A)AddExact(Ids,Pair.Key);for(const auto& Pair:B)AddExact(Ids,Pair.Key);
    Ids.Sort([](const FString& X,const FString& Y){return X.Compare(Y,ESearchCase::CaseSensitive)<0;});FArray Added,Removed,Changed,Positions;
    for(const FString& Id:Ids)
    {
        const FObject* Old=FindNode(A,Id);const FObject* New=FindNode(B,Id);
        if(!Old){Added.Add(MakeShared<FJsonValueObject>(New->ToSharedRef()));continue;}
        if(!New){Removed.Add(MakeShared<FJsonValueObject>(Old->ToSharedRef()));continue;}
        const FArray Semantic=Fields(*Old,*New,false),Layout=Fields(*Old,*New,true);
        auto AddRow=[&](const FArray& Changes,FArray& Destination)
        {
            if(Changes.IsEmpty())return;
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),Id);
            Row->SetArrayField(TEXT("changes"),Changes);Destination.Add(MakeShared<FJsonValueObject>(Row));
        };
        AddRow(Semantic,Changed);AddRow(Layout,Positions);
    }
    const FArray EdgeAdded=Difference(After->GetArrayField(TEXT("edges")),Before->GetArrayField(TEXT("edges")));
    const FArray EdgeRemoved=Difference(Before->GetArrayField(TEXT("edges")),After->GetArrayField(TEXT("edges")));
    const FArray SettingsChanges=Fields(Settings(Before)->AsObject(),Settings(After)->AsObject(),false);
    auto Result=MakeShared<FJsonObject>();Result->SetStringField(TEXT("format"),TEXT("NodeForgeGraphDifference"));Result->SetNumberField(TEXT("version"),1);
    Result->SetArrayField(TEXT("nodesAdded"),Added);Result->SetArrayField(TEXT("nodesRemoved"),Removed);
    Result->SetArrayField(TEXT("nodesChanged"),Changed);Result->SetArrayField(TEXT("positionsChanged"),Positions);
    Result->SetArrayField(TEXT("edgesAdded"),EdgeAdded);Result->SetArrayField(TEXT("edgesRemoved"),EdgeRemoved);
    Result->SetArrayField(TEXT("settingsChanged"),SettingsChanges);
    Result->SetBoolField(TEXT("requestStructureOrValuesChanged"),!Added.IsEmpty()||!Removed.IsEmpty()||!Changed.IsEmpty()||!EdgeAdded.IsEmpty()||!EdgeRemoved.IsEmpty()||!SettingsChanges.IsEmpty());
    Result->SetBoolField(TEXT("layoutChanged"),!Positions.IsEmpty());
    Result->SetStringField(TEXT("identityRule"),TEXT("Preserve IDs from actual source export; array/object ordering ignored. Different logical IDs are remove/add, never guessed matches."));
    Result->SetStringField(TEXT("scope"),TEXT("Explicit validated JSON request differences; missing versus explicit native defaults remain visible. Not native semantic equivalence, compilation, save or runtime proof."));
    return Result;
}
