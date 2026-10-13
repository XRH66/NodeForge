#include "NodeForgeTaskRules.h"
#include "Serialization/JsonSerializer.h"

bool NodeForgeTaskRules::Select(const TSharedPtr<FJsonObject>& Config,const TSharedRef<FJsonObject>& Catalog,
    const FString& FullDigest,TSharedPtr<FJsonObject>& Bundle,FString& Error)
{
    Bundle.Reset();Error.Empty();
    FString Kind,Query;
    if(Config)
        for(const auto& Entry:Config->Values)
            if(!Entry.Key.Equals(TEXT("assetKind"),ESearchCase::CaseSensitive) && !Entry.Key.Equals(TEXT("query"),ESearchCase::CaseSensitive))
            {Error=TEXT("GraphRules requires canonical assetKind/query field names");return false;}
    if(!Config.IsValid() || Config->Values.Num()!=2 || !Config->TryGetStringField(TEXT("assetKind"),Kind)
        || !Config->TryGetStringField(TEXT("query"),Query) || (!Kind.Equals(TEXT("Material"),ESearchCase::CaseSensitive) && !Kind.Equals(TEXT("Blueprint"),ESearchCase::CaseSensitive)) || Query.Len()>256)
    {Error=TEXT("GraphRules requires exactly assetKind Material|Blueprint and query of at most 256 characters");return false;}
    TArray<FString> Terms;Query.TrimStartAndEnd().ParseIntoArrayWS(Terms);
    if(Terms.Num()>8) {Error=TEXT("GraphRules supports at most eight whitespace-separated search terms");return false;}
    const FString SectionName=Kind.Equals(TEXT("Material"),ESearchCase::CaseSensitive)?TEXT("material"):TEXT("blueprint");
    const auto Section=Catalog->GetObjectField(SectionName);
    const auto& Nodes=Section->GetArrayField(TEXT("nodes"));
    TArray<TSharedPtr<FJsonValue>> Selected;
    for(const auto& Node:Nodes)
    {
        const auto Entry=Node->AsObject();
        FString Class;
        if(!Entry.IsValid() || !Entry->TryGetStringField(TEXT("class"),Class))
        {Error=TEXT("GraphRules encountered an incomplete native catalog entry");return false;}
        FString Identity=Class,VariableType;
        const TSharedPtr<FJsonObject>* Properties=nullptr;
        if(Entry->TryGetObjectField(TEXT("properties"),Properties))
        {
            FString Owner,Member;
            (*Properties)->TryGetStringField(TEXT("owner"),Owner);(*Properties)->TryGetStringField(TEXT("member"),Member);
            Identity+=TEXT(" ")+Owner+TEXT(" ")+Member;
        }
        if(Entry->TryGetStringField(TEXT("variableType"),VariableType)) Identity+=TEXT(" ")+VariableType;
        bool Match=Terms.IsEmpty();
        for(const FString& Term:Terms) Match |= Identity.Contains(Term,ESearchCase::IgnoreCase);
        if(Match) Selected.Add(Node);
    }
    if(Selected.IsEmpty()) {Error=TEXT("GraphRules found no supported native node identity matching the query; no replacement or guessed node returned");return false;}
    auto Subset=MakeShared<FJsonObject>();Subset->Values=Section->Values;
    Subset->SetArrayField(TEXT("nodes"),Selected);
    Bundle=MakeShared<FJsonObject>();
    Bundle->SetStringField(TEXT("format"),TEXT("NodeForgeTaskRules"));Bundle->SetNumberField(TEXT("version"),1);
    Bundle->SetStringField(TEXT("inputFormat"),TEXT("NodeForgeGraph"));Bundle->SetNumberField(TEXT("inputVersion"),3);
    Bundle->SetObjectField(TEXT("assetDefinitions"),Catalog->GetObjectField(TEXT("assetDefinitions")));
    Bundle->SetStringField(TEXT("engine"),Catalog->GetStringField(TEXT("engine")));
    Bundle->SetStringField(TEXT("adapter"),Catalog->GetStringField(TEXT("adapter")));
    Bundle->SetStringField(TEXT("assetKind"),Kind);Bundle->SetStringField(TEXT("query"),Query);
    Bundle->SetStringField(TEXT("catalogDigest"),FullDigest);
    Bundle->SetNumberField(TEXT("sourceNodeCount"),Nodes.Num());Bundle->SetNumberField(TEXT("matchedNodeCount"),Selected.Num());
    Bundle->SetObjectField(TEXT("capabilities"),Subset);
    Bundle->SetStringField(TEXT("searchPolicy"),TEXT("case-insensitive OR substring over actual class, function owner/member or variable type; empty query returns one full family; stable native catalog order; no asset-name/effect recipes"));
    Bundle->SetStringField(TEXT("usage"),TEXT("Read-only rules, not an import request. Use full catalogDigest above; never hash this subset as an import catalog. Combine with GraphProtocol and resource signatures. Search does not prove compilation/runtime; mandatory scaffolds and needed connected nodes must be explicitly included in the final JSON. Unbound MaterialFunction entry requires native resource inspection before using dynamic pins."));
    return true;
}
