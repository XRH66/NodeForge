// Project-owned presentation glue; uses UE JSON and the existing result contract.
#include "NodeForgeResultView.h"
#include "NodeForgePrototype.h"
#include "NodeForgeGraphDiff.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/Archive.h"

namespace NodeForgeResultView
{
FString Compact(const TSharedRef<FJsonObject>& Report)
{
    constexpr int32 Limit=1200;
    FString Text;
    bool Omitted=false;
    auto Clip=[&](FString Value,int32 Count)
    {
        Value.ReplaceInline(TEXT("\r"),TEXT(" "));Value.ReplaceInline(TEXT("\n"),TEXT(" "));
        if(Value.Len()>Count){Omitted=true;return Value.Left(Count-1)+TEXT("…");}
        return Value;
    };
    auto String=[](const TSharedPtr<FJsonObject>& Object,const TCHAR* Key)
    {FString Value;if(Object)Object->TryGetStringField(Key,Value);return Value;};
    auto Child=[](const TSharedPtr<FJsonObject>& Object,const TCHAR* Key)->TSharedPtr<FJsonObject>
    {const TSharedPtr<FJsonObject>* Value=nullptr;return Object && Object->TryGetObjectField(Key,Value) && Value?*Value:nullptr;};
    auto Field=[&](const TCHAR* Key,const TCHAR* Label,int32 Count=100)
    {const FString Value=String(Report,Key);if(!Value.IsEmpty())Text+=FString(Label)+TEXT("：")+Clip(Value,Count)+TEXT("\n");};
    auto Count=[](const TSharedPtr<FJsonObject>& Object,const TCHAR* Key)
    {const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;return Object && Object->TryGetArrayField(Key,Values)?Values->Num():0;};
    auto Brief=[&](const TSharedPtr<FJsonValue>& Value)->FString
    {
        if(!Value)return TEXT("未设置");
        if(Value->Type==EJson::String)return TEXT("“")+Clip(Value->AsString(),58)+TEXT("”");
        if(Value->Type==EJson::Number)return FString::Printf(TEXT("%.17g"),Value->AsNumber());
        if(Value->Type==EJson::Boolean)return Value->AsBool()?TEXT("true"):TEXT("false");
        if(Value->Type==EJson::Null)return TEXT("null");
        Omitted=true;
        if(Value->Type==EJson::Array)return FString::Printf(TEXT("[%d项]"),Value->AsArray().Num());
        return Value->Type==EJson::Object?FString::Printf(TEXT("{%d项}"),Value->AsObject()->Values.Num()):TEXT("未识别值");
    };
    const FString Mode=String(Report,TEXT("mode")),Overall=String(Report,TEXT("overallStatus")),Operation=String(Report,TEXT("operationStatus"));
    FString FailedCheck,CheckReason;
    if(const auto Checks=Child(Report,TEXT("checks")))
        for(const TCHAR* Stage:{TEXT("validation"),TEXT("build"),TEXT("initialReadback"),TEXT("compile"),TEXT("finalReadback"),TEXT("publish"),TEXT("save"),TEXT("persistence"),TEXT("runtime")})
            if(const auto Check=Child(Checks,Stage);String(Check,TEXT("status"))==TEXT("failed"))
            {FailedCheck=Stage;CheckReason=String(Check,TEXT("reason"));break;}
    const bool Conflict=!FailedCheck.IsEmpty() && (Overall==TEXT("succeeded") || Overall==TEXT("waitingPersistence") || Operation==TEXT("passed"));
    const bool Failed=Conflict || !String(Report,TEXT("failedStage")).IsEmpty() || Overall==TEXT("rejected") || Overall==TEXT("failed") || Overall==TEXT("cleanupRequired") || Operation==TEXT("failed") || Operation==TEXT("rejected");
    if(Conflict)Text+=TEXT("报告状态字段矛盾，不能确认成功；请核对完整报告。\n");
    else if(Overall==TEXT("cleanupRequired"))Text+=TEXT("生成未成功，需要检查残留（cleanupRequired）。\n");
    else if(Failed)Text+=TEXT("本次操作未通过（")+Clip(Overall.IsEmpty()?Operation:Overall,48)+TEXT("）。\n");
    else if(Overall==TEXT("waitingPersistence"))Text+=TEXT("资产已保存，尚待独立重开核验（waitingPersistence）。\n");
    else if(Mode==TEXT("PrepareNewCopy") && Operation==TEXT("passed"))Text+=TEXT("新副本输入已准备；尚未生成资产。\n");
    else if(Mode==TEXT("GraphPreview") && Operation==TEXT("passed"))Text+=TEXT("差异预览完成；未生成或修改资产。\n");
    else if(Mode==TEXT("GraphPreflight") && Operation==TEXT("passed"))Text+=TEXT("输入检查通过；尚未生成，目标未预留。\n");
    else if(Overall==TEXT("succeeded"))Text+=TEXT("本次操作已完成；运行和画面以各自验证结果为准。\n");
    else if(Operation==TEXT("passed"))Text+=TEXT("读取完成；尚未生成资产。\n");
    else Text+=TEXT("报告未提供可识别的通过状态；不能确认成功。\n");
    if(Failed)
    {
        const FString Reason=Conflict?CheckReason:!String(Report,TEXT("nextAction")).IsEmpty()?String(Report,TEXT("nextAction")):CheckReason;
        if(!Reason.IsEmpty())Text+=TEXT("诊断 / 下一步：")+Clip(Reason,180)+TEXT("\n");
    }
    Field(TEXT("failedStage"),TEXT("失败阶段"),48);
    if(String(Report,TEXT("failedStage")).IsEmpty()&&!FailedCheck.IsEmpty())Text+=TEXT("失败阶段：")+Clip(FailedCheck,48)+TEXT("\n");
    Field(TEXT("errorCode"),TEXT("错误编号"),64);
    if(const auto Checks=Child(Report,TEXT("checks")); Checks && (Mode==TEXT("Import") || Mode==TEXT("Verify") || Failed))
    {
        FString States;
        for(const auto& Stage:TArray<TPair<FString,FString>>{{TEXT("compile"),TEXT("编译")},{TEXT("save"),TEXT("保存")},{TEXT("persistence"),TEXT("重开")},{TEXT("runtime"),TEXT("运行")}})
        {
            const auto Check=Child(Checks,*Stage.Key);const FString Status=String(Check,TEXT("status"));
            FString Label=Status==TEXT("passed")?TEXT("通过"):Status==TEXT("failed")?TEXT("失败"):Status==TEXT("notRun")?TEXT("未验证"):Clip(Status,24);
            if(!Status.IsEmpty()){if(!States.IsEmpty())States+=TEXT(" · ");States+=Stage.Value+TEXT("：")+Label;}
        }
        if(!States.IsEmpty())Text+=States+TEXT("\n");
    }
    if(const auto Residue=Child(Report,TEXT("residue")); Residue && Failed)
    {
        FString State=String(Residue,TEXT("diskState")),Cleanup=String(Residue,TEXT("cleanup"));bool Value=false;
        if(State==TEXT("notWritten"))Text+=TEXT("本次未写入资产文件。\n");
        else if(!State.IsEmpty())Text+=TEXT("磁盘状态：")+Clip(State,32)+TEXT("\n");
        if(Residue->TryGetBoolField(TEXT("targetFilePresent"),Value) && Value)
            Text+=Child(Report,TEXT("conflict"))?TEXT("原目标文件已存在，本次没有覆盖。\n"):TEXT("目标文件存在；请先核对残留再重试。\n");
        if(!Cleanup.IsEmpty() && Cleanup!=TEXT("noneRequired"))Text+=TEXT("残留提醒：")+Clip(Cleanup,140)+TEXT("\n");
    }
    if(const auto Compilation=Child(Report,TEXT("compilation")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Errors=nullptr;const TArray<TSharedPtr<FJsonValue>>* Messages=nullptr;double ErrorCount=0,Warnings=0;
        if(Compilation->TryGetArrayField(TEXT("errors"),Errors) && !Errors->IsEmpty())
        {
            if((*Errors)[0] && (*Errors)[0]->Type==EJson::String)Text+=TEXT("编译诊断：")+Clip((*Errors)[0]->AsString(),160)+TEXT("\n");
            else {Text+=TEXT("编译诊断包含详细记录，请打开报告。\n");Omitted=true;}
            Omitted|=Errors->Num()>1;
        }
        else if(Compilation->TryGetNumberField(TEXT("errors"),ErrorCount) && Compilation->TryGetNumberField(TEXT("warnings"),Warnings) && (ErrorCount>0 || Warnings>0))
        {
            Text+=FString::Printf(TEXT("编译诊断：%.0f错误，%.0f警告\n"),ErrorCount,Warnings);
            if(Compilation->TryGetArrayField(TEXT("messages"),Messages) && !Messages->IsEmpty() && (*Messages)[0] && (*Messages)[0]->Type==EJson::Object)
            {Text+=Clip(String((*Messages)[0]->AsObject(),TEXT("message")),160)+TEXT("\n");Omitted|=Messages->Num()>1;}
        }
    }
    if(const auto Preflight=Child(Report,TEXT("preflightSummary")))
    {
        double Nodes=0,Edges=0,Defaults=0,Positions=0;
        if(Preflight->TryGetNumberField(TEXT("plannedNodeCount"),Nodes) && Preflight->TryGetNumberField(TEXT("plannedEdgeCount"),Edges))
            Text+=FString::Printf(TEXT("输入计划：%.0f个节点，%.0f条连接；尚未生成。\n"),Nodes,Edges);
        if(Preflight->TryGetNumberField(TEXT("explicitDefaultCount"),Defaults) && Preflight->TryGetNumberField(TEXT("explicitPositionCount"),Positions))
            Text+=FString::Printf(TEXT("显式配置：%.0f项默认值，%.0f个节点坐标；目标未预留。\n"),Defaults,Positions);
    }
    if(const auto Difference=Child(Report,TEXT("graphDifference")))
    {
        Text+=FString::Printf(TEXT("差异：节点 +%d / −%d / 改%d；连接 +%d / −%d；位置%d；设置%d。\n"),
            Count(Difference,TEXT("nodesAdded")),Count(Difference,TEXT("nodesRemoved")),Count(Difference,TEXT("nodesChanged")),
            Count(Difference,TEXT("edgesAdded")),Count(Difference,TEXT("edgesRemoved")),Count(Difference,TEXT("positionsChanged")),Count(Difference,TEXT("settingsChanged")));
        int32 Displayed=0;
        auto Detail=[&](const FString& Name,const TSharedPtr<FJsonValue>& Before,const TSharedPtr<FJsonValue>& After)
        {
            if(Displayed>=3){Omitted=true;return;}
            Text+=Clip(Name,90)+TEXT("：")+Brief(Before)+TEXT(" → ")+Brief(After)+TEXT("\n");++Displayed;
        };
        auto Change=[&](const FString& Prefix,const TSharedPtr<FJsonObject>& Row)
        {
            if(!Row)return;
            const FString Name=Prefix+String(Row,TEXT("field"));
            const auto* A=Row->Values.Find(TEXT("before"));const auto* B=Row->Values.Find(TEXT("after"));
            if(A && B && *A && *B && (*A)->Type==EJson::Object && (*B)->Type==EJson::Object)
            {
                const auto Before=(*A)->AsObject(),After=(*B)->AsObject();TSet<FString> Keys;
                for(const auto& Pair:Before->Values)Keys.Add(Pair.Key);for(const auto& Pair:After->Values)Keys.Add(Pair.Key);
                TArray<FString> Names=Keys.Array();Names.Sort();
                for(const FString& Key:Names)
                {
                    const auto* Old=Before->Values.Find(Key);const auto* New=After->Values.Find(Key);
                    if(Old && New && NodeForgeGraphDiff::EqualValue(*Old,*New))continue;
                    FString Label=Name+TEXT(".")+Key;
                    FString Parameter=String(After,TEXT("ParameterName"));if(Parameter.IsEmpty())Parameter=String(Before,TEXT("ParameterName"));
                    if(!Parameter.IsEmpty() && Key!=TEXT("ParameterName"))Label+=TEXT(" (")+Parameter+TEXT(")");
                    Detail(Label,Old?*Old:nullptr,New?*New:nullptr);
                }
            }
            else Detail(Name,A?*A:nullptr,B?*B:nullptr);
        };
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        if(Difference->TryGetArrayField(TEXT("nodesChanged"),Rows))for(const auto& Value:*Rows)
        {
            if(!Value || Value->Type!=EJson::Object)continue;const auto Row=Value->AsObject();const TArray<TSharedPtr<FJsonValue>>* Changes=nullptr;
            if(Row->TryGetArrayField(TEXT("changes"),Changes))for(const auto& Item:*Changes)
                if(Item && Item->Type==EJson::Object)Change(String(Row,TEXT("id"))+TEXT("."),Item->AsObject());
        }
        if(Difference->TryGetArrayField(TEXT("settingsChanged"),Rows))for(const auto& Value:*Rows)
            if(Value && Value->Type==EJson::Object)Change(TEXT("settings."),Value->AsObject());
        Text+=TEXT("仅预览；未生成副本，需单独生成与验证。\n");
    }
    if(const auto Rules=Child(Report,TEXT("taskRules")))
    {double Matches=0,Total=0;if(Rules->TryGetNumberField(TEXT("matchedNodeCount"),Matches) && Rules->TryGetNumberField(TEXT("sourceNodeCount"),Total))Text+=FString::Printf(TEXT("规则匹配：%.0f / %.0f；完整规则可复制或打开报告。\n"),Matches,Total);}
    if(Child(Report,TEXT("catalog")))Text+=TEXT("已读取本机节点目录；完整规则可复制或打开报告。发现或可创建不代表运行已验证。\n");
    if(Child(Report,TEXT("functionSignature")))Text+=TEXT("已读取函数真实签名；完整引脚与引用信息保存在报告。\n");
    if(Child(Report,TEXT("exportedRequest")))Text+=TEXT("已只读导出构图；新副本尚未生成。\n");
    if(const auto Preparation=Child(Report,TEXT("copyPreparation")))
    {
        Text+=TEXT("原名称：")+Clip(FPaths::GetCleanFilename(String(Preparation,TEXT("originalAssetPath"))),64)+TEXT("\n");
        Text+=TEXT("新目标：")+Clip(String(Preparation,TEXT("newAssetPath")),110)+TEXT("\n");
        Text+=TEXT("仅更改目标和请求ID；确认后点“生成新资产”。\n");
    }
    else Field(TEXT("assetPath"),TEXT("目标"),100);
    // Read-only previews and saved/reopened states already state their next step in Chinese.
    // Raw backend instructions remain in the complete report.
    const FString Footer=Omitted?TEXT("部分内容已省略。完整原文请打开或复制报告。"):TEXT("此处仅显示摘要；完整原文请打开或复制报告。");
    const FString TruncatedFooter=TEXT("\n…内容已省略。完整原文请打开或复制报告。");
    if(Text.Len()+Footer.Len()>Limit)return Text.Left(Limit-TruncatedFooter.Len())+TruncatedFooter;
    return Text+Footer;
}

FString Describe(const TSharedRef<FJsonObject>& Report)
{
    FString Text;
    auto Field=[&](const TCHAR* Key,const TCHAR* Label)
    {
        FString Value;
        if(Report->TryGetStringField(Key,Value) && !Value.IsEmpty())
            Text+=FString::Printf(TEXT("%s：%s\n"),Label,*Value);
    };
    Field(TEXT("mode"),TEXT("操作"));
    Field(TEXT("overallStatus"),TEXT("结果状态"));
    Field(TEXT("operationStatus"),TEXT("操作状态"));
    Field(TEXT("requestId"),TEXT("请求编号"));
    Field(TEXT("assetPath"),TEXT("请求目标"));
    Field(TEXT("failedStage"),TEXT("失败阶段"));
    Field(TEXT("errorCode"),TEXT("错误编号"));
    const TSharedPtr<FJsonObject>* Preflight=nullptr;
    if(Report->TryGetObjectField(TEXT("preflightSummary"),Preflight) && Preflight && Preflight->IsValid())
    {
        double Nodes=0,Edges=0,Defaults=0,Positions=0;
        if((*Preflight)->TryGetNumberField(TEXT("plannedNodeCount"),Nodes)
            && (*Preflight)->TryGetNumberField(TEXT("plannedEdgeCount"),Edges))
            Text+=FString::Printf(TEXT("输入计划：%.0f个节点，%.0f条连接（尚未生成）\n"),Nodes,Edges);
        if((*Preflight)->TryGetNumberField(TEXT("explicitDefaultCount"),Defaults)
            && (*Preflight)->TryGetNumberField(TEXT("explicitPositionCount"),Positions))
            Text+=FString::Printf(TEXT("显式配置：%.0f项输入默认值，%.0f个节点坐标\n"),Defaults,Positions);
        FString Json;FJsonSerializer::Serialize(Preflight->ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
        Text+=TEXT("生成前摘要（报告原文）：\n")+Json+TEXT("\n");
    }
    const TSharedPtr<FJsonObject>* Checks=nullptr;
    if(Report->TryGetObjectField(TEXT("checks"),Checks) && Checks && Checks->IsValid())
    {
        for(const TCHAR* Stage : {TEXT("validation"),TEXT("build"),TEXT("initialReadback"),TEXT("compile"),TEXT("finalReadback"),TEXT("publish"),TEXT("save"),TEXT("persistence"),TEXT("runtime")})
        {
            const TSharedPtr<FJsonObject>* Check=nullptr;
            if(!(*Checks)->TryGetObjectField(Stage,Check) || !Check || !Check->IsValid())continue;
            FString Status,Reason;
            if((*Check)->TryGetStringField(TEXT("status"),Status))
                Text+=FString::Printf(TEXT("%s：%s\n"),Stage,*Status);
            if((*Check)->TryGetStringField(TEXT("reason"),Reason) && !Reason.IsEmpty())
                Text+=TEXT("  ")+Reason+TEXT("\n");
        }
    }
    Field(TEXT("nextAction"),TEXT("后续操作 / 原始诊断"));
    for(const TCHAR* Key : {TEXT("compilation"),TEXT("residue")})
    {
        const TSharedPtr<FJsonObject>* Detail=nullptr;
        if(Report->TryGetObjectField(Key,Detail) && Detail && Detail->IsValid())
        {
            FString Json;
            if(FJsonSerializer::Serialize(Detail->ToSharedRef(),TJsonWriterFactory<>::Create(&Json)))
                Text+=FString::Printf(TEXT("%s（报告原文）：\n%s\n"),Key,*Json);
        }
    }
    return Text.IsEmpty()?TEXT("报告未提供可识别的状态；不能确认成功。\n"):Text;
}

bool Read(const FString& Path,FString& OriginalJson,FString& Description)
{
    OriginalJson.Empty();Description.Empty();
    FString Full=FPaths::ConvertRelativePathToFull(Path);FPaths::NormalizeFilename(Full);
    const FString Reports=FPaths::ConvertRelativePathToFull(NodeForgePrototype::WorkspaceDirectory()/TEXT("Reports"));
    if(!FPaths::CollapseRelativeDirectories(Full) || !FPaths::IsUnderDirectory(Full,Reports) ||
        !FPaths::GetExtension(Full).Equals(TEXT("json"),ESearchCase::IgnoreCase))
    {Description=TEXT("结果报告必须位于本项目 Reports 内。" );return false;}
    TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*Full));
    const int64 Size=Reader?Reader->TotalSize():-1;
    FString Json;TSharedPtr<FJsonObject> Report;
    if(Size<1 || Size>1024*1024)
    {Description=TEXT("结果报告缺失、无效或超过1 MiB，不能复制旧结果。" );return false;}
    TArray<uint8> Bytes;Bytes.SetNumZeroed(static_cast<int32>(Size)+2);
    Reader->Serialize(Bytes.GetData(),Size);
    const bool Complete=!Reader->IsError() && Reader->TotalSize()==Size;
    const bool Closed=Reader->Close();
    if(!Complete || !Closed)
    {Description=TEXT("结果报告未完整读取或大小发生变化，不能复制旧结果。");return false;}
    FFileHelper::BufferToString(Json,Bytes.GetData(),static_cast<int32>(Size));
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Report) || !Report.IsValid())
    {Description=TEXT("结果报告内容无效，不能确认成功。");return false;}
    OriginalJson=MoveTemp(Json);Description=Describe(Report.ToSharedRef());return true;
}
}
