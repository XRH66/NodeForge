#pragma once
#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
class FJsonObject;

// Local Commandlet host boundary. Business handlers remain in Prototype::Execute.
namespace NodeForgeControlOperations
{
    enum class EHost : uint8 { Commandlet = 1, ActiveEditor = 2 };
    enum class EArguments : uint8 { None, Path, Text, CallId, ReleaseCompleted, Hello, Query, Resource, ResourceQuery, Export };
    enum class EDispatch : uint8 { ReadOnly, GameThread, Management, Handshake };
    enum class EEditorFeature : uint8 { Read = 1, Draft = 2, Graph = 4, Stop = 8 };
    // Shared metadata access; Commandlet describe/dispatch continue using this same table.
    struct FOperationDefinition
    {
        const TCHAR* Name;
        const TCHAR* Mode;
        bool InputRequired;
        bool PreviousRequired;
        const TCHAR* ResultKind;
        const TCHAR* SideEffect;
        uint8 Hosts;
        EArguments Arguments;
        EDispatch Dispatch;
        bool RevisionRequired;
        EEditorFeature EditorFeature;
    };
    TConstArrayView<FOperationDefinition> Definitions();
    bool Supports(const FOperationDefinition& Definition, EHost Host);
    const FOperationDefinition* Find(const FString& Name, EHost Host);
    TSharedRef<FJsonObject> ArgumentsSchema(const FOperationDefinition& Definition);
    TSharedRef<FJsonObject> DescribeEditor(uint8 EnabledFeatures);
    bool IsFormalCall(const FString& Params);
    int32 Execute(const FString& Params);
}
