#include "NodeForgePanelState.h"
#include "NodeForgeEditorControlProtocol.h"
#include "Misc/Paths.h"

bool FNodeForgePanelState::ValidateExternalText(const FString& Text, FString& Error)
{
    Error.Empty();
    if (Text.TrimStartAndEnd().IsEmpty())
    { Error = TEXT("内容为空，未替换现有输入。"); return false; }
    if (Text[0] == 0xfeff)
    { Error = TEXT("内容含 BOM，未替换现有输入；请使用 UTF-8 无 BOM。"); return false; }
    for (int32 Index = 0; Index < Text.Len(); ++Index)
        if (Text[Index] == 0) { Error = TEXT("内容含 NUL，未替换现有输入。"); return false; }
    FTCHARToUTF8 Utf8(*Text);
    if (Utf8.Length() > 65536)
    { Error = TEXT("内容超过 64 KiB，未替换现有输入；没有截断。"); return false; }
    FUTF8ToTCHAR Roundtrip(Utf8.Get(), Utf8.Length());
    if (!Text.Equals(FString(Roundtrip.Length(), Roundtrip.Get()), ESearchCase::CaseSensitive))
    { Error = TEXT("内容无法完整编码为 UTF-8，未替换现有输入。"); return false; }
    return true;
}

bool FNodeForgePanelState::Edit(const FString& Text)
{
    if (Current.Text.Equals(Text, ESearchCase::CaseSensitive)) return false;
    if (!bManualEditing) { Previous = Current; bHasPrevious = true; bManualEditing = true; }
    Current.Text = Text;
    Current.bEdited = !Current.Text.Equals(Current.OriginalText, ESearchCase::CaseSensitive);
    ++CurrentRevision;
    return true;
}

bool FNodeForgePanelState::Replace(const FString& Text, const FString& Origin,
                                  const FString& SourcePath, const FString& SourceDigest, FString& Error,
                                  const TSharedPtr<FJsonObject>& ExportOrigin)
{
    if (!ValidateExternalText(Text, Error)) return false;
    const bool SameText = Current.Text.Equals(Text, ESearchCase::CaseSensitive);
    const bool SameExport = (!Current.ExportOrigin && !ExportOrigin)
        || (Current.ExportOrigin && ExportOrigin && Current.ExportOrigin->GetStringField(TEXT("reportPath")) == ExportOrigin->GetStringField(TEXT("reportPath"))
            && Current.ExportOrigin->GetStringField(TEXT("reportSha256")) == ExportOrigin->GetStringField(TEXT("reportSha256")));
    if (SameText && SameExport && Current.Origin.Equals(Origin, ESearchCase::CaseSensitive)
        && Current.SourcePath.Equals(SourcePath, ESearchCase::CaseSensitive)
        && Current.SourceDigest.Equals(SourceDigest, ESearchCase::CaseSensitive)) { bManualEditing = false; return true; }
    // Repeated identical content must not replace the previous useful draft.
    if (!SameText || !SameExport) { Previous = Current; bHasPrevious = true; }
    Current = FNodeForgeDraft();
    Current.Text = Text; Current.OriginalText = Text; Current.Origin = Origin;
    Current.SourcePath = SourcePath; Current.SourceDigest = SourceDigest;
    Current.ExportOrigin = ExportOrigin;
    bManualEditing = false;
    ++CurrentRevision;
    return true;
}

bool FNodeForgePanelState::Restore()
{
    if (!bHasPrevious) return false;
    Current = MoveTemp(Previous); Previous = FNodeForgeDraft(); bHasPrevious = false;
    bManualEditing = false;
    ++CurrentRevision; // Restoring text never restores an old validation result.
    return true;
}

bool FNodeForgePanelState::PrepareCopy(const FString& Text, const FString& PreparationReport, FString& Error)
{
    if(!ValidateExternalText(Text,Error) || PreparationReport.IsEmpty())return false;
    Previous=Current;bHasPrevious=true;
    Current.Text=Text;Current.PreparationReport=PreparationReport;
    Current.bEdited=!Text.Equals(Current.OriginalText,ESearchCase::CaseSensitive);
    bManualEditing=false;
    ++CurrentRevision;
    return true;
}

FString FNodeForgePanelState::DescribeSource() const
{
    if (Current.Text.IsEmpty()) return TEXT("来源：尚未输入 · 0 / 65536 字节");
    FString Source = Current.Origin == TEXT("example") ? TEXT("来源：案例")
        : Current.Origin == TEXT("file") ? TEXT("文件：") + FPaths::GetCleanFilename(Current.SourcePath)
        : Current.Origin == TEXT("clipboard") ? TEXT("来源：剪贴板")
        : Current.Origin == TEXT("export") ? TEXT("来源：已导出构图") : TEXT("来源：手工输入");
    if (Current.bEdited) Source += TEXT("（已编辑）");
    if (!Current.PreparationReport.IsEmpty()) Source += TEXT(" · 已准备新副本");
    const int32 Bytes = FTCHARToUTF8(*Current.Text).Length();
    return Source + FString::Printf(TEXT(" · %d / 65536 字节%s"), Bytes,
        Bytes > 65536 ? TEXT("（超限，检查和生成会拒绝）") : TEXT(""));
}

TSharedRef<FJsonObject> FNodeForgePanelState::Provenance(const FString& SnapshotPath, const FString& SnapshotDigest) const
{
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("contentSource"), TEXT("currentDraft"));
    Result->SetStringField(TEXT("origin"), Current.Origin);
    Result->SetStringField(TEXT("sourcePath"), Current.SourcePath);
    Result->SetStringField(TEXT("sourceOriginalDigest"), Current.SourceDigest);
    if(Current.ExportOrigin)Result->SetObjectField(TEXT("exportOrigin"),ExportProvenance());
    if(!Current.PreparationReport.IsEmpty())Result->SetStringField(TEXT("copyPreparationReport"),Current.PreparationReport);
    Result->SetBoolField(TEXT("sourceEdited"), Current.bEdited);
    Result->SetStringField(TEXT("draftRevision"), LexToString(CurrentRevision));
    Result->SetStringField(TEXT("snapshotPath"), SnapshotPath);
    Result->SetStringField(TEXT("snapshotDigest"), SnapshotDigest);
    return Result;
}
TSharedPtr<FJsonObject> FNodeForgePanelState::ExportProvenance() const
{
    if(!Current.ExportOrigin)return nullptr;
    auto Result=MakeShared<FJsonObject>();Result->Values=Current.ExportOrigin->Values;
    TSharedPtr<FJsonObject> Graph;FTCHARToUTF8 Utf8(*Current.Text);
    const bool Parsed=Utf8.Length()<=NodeForgeEditorControl::MaxGraphBytes
        && NodeForgeEditorControl::ParseObject(MakeArrayView(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length()),Graph);
    const bool Derived=!Parsed || NodeForgeEditorControl::SemanticDigest(Graph.ToSharedRef())!=Current.ExportOrigin->GetStringField(TEXT("payloadSemanticDigest"));
    Result->SetBoolField(TEXT("derived"),Derived);
    Result->SetStringField(TEXT("currentPayloadRelation"),!Parsed?TEXT("invalidDraft"):Derived?TEXT("derived"):TEXT("original"));
    return Result;
}
