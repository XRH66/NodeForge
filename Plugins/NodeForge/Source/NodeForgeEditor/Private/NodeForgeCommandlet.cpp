#include "NodeForgeCommandlet.h"
#include "NodeForgePrototype.h"
#include "NodeForgeControlOperations.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/EngineVersion.h"
#include "Misc/CommandLine.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UNodeForgeCommandlet::UNodeForgeCommandlet()
{
    // UE allocates FNULLSceneInterface when GIsClient is false, even with an RHI.
    // Only actual pixel validation needs a full rendering scene; regular import is unchanged.
    FString RequestedMode;
    FParse::Value(FCommandLine::Get(),TEXT("NFMode="),RequestedMode);
    IsClient = RequestedMode==TEXT("VisualCheck") || RequestedMode==TEXT("GraphVisualCheck") || RequestedMode==TEXT("CandidateVisualCheck");
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}
int32 UNodeForgeCommandlet::Main(const FString& Params)
{
    if (NodeForgeControlOperations::IsFormalCall(Params))
        return NodeForgeControlOperations::Execute(Params);
    FString ReportPath;
    FParse::Value(*Params, TEXT("NFReport="), ReportPath);
    if (ReportPath.IsEmpty()) return 2;
    FString Mode, Input, Previous;
    FParse::Value(*Params, TEXT("NFMode="), Mode);
    FParse::Value(*Params, TEXT("NFInput="), Input);
    FParse::Value(*Params, TEXT("NFPrevious="), Previous);
    if (!Mode.IsEmpty() && Mode != TEXT("Smoke"))
        return NodeForgePrototype::Execute(Mode, Input, ReportPath, Previous);
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("milestone"), TEXT("pluginSkeleton"));
    Report->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
    Report->SetStringField(TEXT("project"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
    Report->SetStringField(TEXT("moduleLoad"), TEXT("passed"));
    Report->SetStringField(TEXT("commandletExecution"), TEXT("passed"));
    return NodeForgePrototype::WriteJson(Report, ReportPath) ? 0 : 3;
}
