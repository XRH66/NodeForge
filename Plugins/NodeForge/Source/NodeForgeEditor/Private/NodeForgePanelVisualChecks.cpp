// Render the same production panel in owned review windows; never control other applications.
#include "NodeForgePanel.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "ImageUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgePanelVisualTest,"NodeForge.PanelPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgePanelVisualTest::RunTest(const FString& Parameters)
{
    FString Output;
    const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFReport="),Output)
        || IFileManager::Get().FileExists(*Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Root/TEXT("Reports"))
        || IsRunningCommandlet() || !FSlateApplication::IsInitialized())
    {AddError(TEXT("Initialized Editor Slate and a new NFReport in Reports are required."));return false;}
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output),true);
    const FString Preview=Root/TEXT("Reports/UiPreviews/4BBACC094CCB74754E9592BF8B40E68F.json");
    const FString SourceHash=NodeForgePrototype::FileSha256(Preview);
    TArray<TSharedPtr<FJsonValue>> Rows;
    struct FPanelCase { int32 Width; int32 Height; bool Expanded; };
    const TArray<FPanelCase> Cases={{1000,760,false},{620,760,false},{744,624,true},{620,624,true}};
    for(const FPanelCase& Case:Cases)
    {
        const FString Label=FString::Printf(TEXT("Panel%dx%d%s"),Case.Width,Case.Height,Case.Expanded?TEXT("Expanded"):TEXT("Collapsed"));
        const FString Png=FPaths::GetPath(Output)/(Label+TEXT(".png"));
        const FString EndPng=FPaths::GetPath(Output)/(Label+TEXT("ScrollEnd.png"));
        if(IFileManager::Get().FileExists(*Png) || (Case.Expanded && IFileManager::Get().FileExists(*EndPng)))
        {AddError(TEXT("Preserve existing screenshot."));return false;}
        const TSharedRef<SWidget> Panel=NodeForgePanel::CreateForReview(Preview);
        const TSharedRef<SWindow> Window=SNew(SWindow).Title(FText::FromString(TEXT("NodeForge UI review")))
            .ClientSize(FVector2D(Case.Width,Case.Height)).SizingRule(ESizingRule::FixedSize)
            .AutoCenter(EAutoCenter::None).ScreenPosition(FVector2D(0,0))
            .FocusWhenFirstShown(false).ActivationPolicy(EWindowActivationPolicy::Never)
            .SupportsMaximize(false).SupportsMinimize(false)[Panel];
        FSlateApplication::Get().AddWindow(Window,true);
        // Inspect the production tree; SetExpanded is the native no-animation API.
        // No click, OS input, import or clipboard action is synthesized by this test.
        TSharedPtr<SWidget> Input,Result,Generate,Rules,Copy,Advanced;
        TMap<const SWidget*,TArray<TSharedRef<SWidget>>> Paths;
        TFunction<void(const TSharedRef<SWidget>&,TArray<TSharedRef<SWidget>>,int32)> Visit;
        Visit=[&](const TSharedRef<SWidget>& Widget,TArray<TSharedRef<SWidget>> Ancestors,int32 Depth)
        {
            if(Depth>64)return;
            Ancestors.Add(Widget);
            const FName Tag=Widget->GetTag();
            if(Tag==TEXT("NodeForge.Input"))Input=Widget;
            if(Tag==TEXT("NodeForge.ResultSummary"))Result=Widget;
            if(Tag==TEXT("NodeForge.Generate"))Generate=Widget;
            if(Tag==TEXT("NodeForge.RuleSection"))Rules=Widget;
            if(Tag==TEXT("NodeForge.CopySection"))Copy=Widget;
            if(Tag==TEXT("NodeForge.AdvancedScroll"))Advanced=Widget;
            Paths.Add(&Widget.Get(),Ancestors);
            FChildren* Children=Widget->GetChildren();
            for(int32 I=0;Children&&I<Children->Num();++I)Visit(Children->GetChildAt(I),Ancestors,Depth+1);
        };
        Visit(Panel,TArray<TSharedRef<SWidget>>(),0);
        TSharedPtr<SWidget> AdvancedPanel,CopyActions;
        int32 ScrollPanelCount=0,CopyActionsCount=0;
        for(const auto& Entry:Paths)
        {
            const auto& Path=Entry.Value;const auto& Widget=Path.Last();
            if(Advanced&&Widget->GetType()==TEXT("SScrollPanel")&&Path.ContainsByPredicate(
                [&](const TSharedRef<SWidget>& Ancestor){return &Ancestor.Get()==Advanced.Get();}))
            {AdvancedPanel=Widget;++ScrollPanelCount;}
            if(Copy&&Widget->GetType()==TEXT("SWrapBox")&&Path.ContainsByPredicate(
                [&](const TSharedRef<SWidget>& Ancestor){return &Ancestor.Get()==Copy.Get();}))
            {CopyActions=Widget;++CopyActionsCount;}
        }
        const bool AreasFound=Rules&&Copy&&Advanced&&Rules->GetType()==TEXT("SExpandableArea")
            &&Copy->GetType()==TEXT("SExpandableArea")&&Advanced->GetType()==TEXT("SScrollBox")
            &&AdvancedPanel&&CopyActions&&ScrollPanelCount==1&&CopyActionsCount==1;
        if(AreasFound)
        {
            StaticCastSharedPtr<SExpandableArea>(Rules)->SetExpanded(Case.Expanded);
            StaticCastSharedPtr<SExpandableArea>(Copy)->SetExpanded(Case.Expanded);
            StaticCastSharedPtr<SScrollBox>(Advanced)->ScrollToStart();
        }
        auto Settle=[&]()
        {
            // Allotted-size wrapping and expansion require subsequent layout frames.
            for(int32 Frame=0;Frame<6;++Frame){Panel->SlatePrepass();FSlateApplication::Get().Tick();}
        };
        Settle();
        auto RectInPanel=[&](const TSharedRef<SWidget>& Widget)
        {
            const FGeometry& G=Widget->GetCachedGeometry();const FGeometry& P=Panel->GetCachedGeometry();
            const FVector2D Start=P.AbsoluteToLocal(G.GetAbsolutePosition());
            const FVector2D End=P.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
            return FSlateRect(Start.X,Start.Y,End.X,End.Y);
        };
        auto Contains=[](const FSlateRect& Outer,const FSlateRect& Inner)
        {
            return Inner.Left>=Outer.Left-1&&Inner.Top>=Outer.Top-1
                &&Inner.Right<=Outer.Right+1&&Inner.Bottom<=Outer.Bottom+1;
        };
        auto VisibleInPanel=[&](const TSharedPtr<SWidget>& Widget)
        {
            if(!Widget || !Widget->GetVisibility().IsVisible())return false;
            const FVector2D Local=Widget->GetCachedGeometry().GetLocalSize();
            const FVector2D Bounds=Panel->GetCachedGeometry().GetLocalSize();
            const FSlateRect Rect=RectInPanel(Widget.ToSharedRef());
            if(Local.X<=0||Local.Y<=0||!Contains(FSlateRect(0,0,Bounds.X,Bounds.Y),Rect))return false;
            // Allotted height alone is insufficient if a scroll/clip ancestor hides it.
            const auto* Ancestors=Paths.Find(Widget.Get());
            if(!Ancestors)return false;
            for(const auto& Parent:*Ancestors)
            {
                if(!Parent->GetVisibility().IsVisible())return false;
                const EWidgetClipping Clipping=Parent->GetClipping();
                if(Clipping!=EWidgetClipping::Inherit && !Contains(RectInPanel(Parent),Rect))return false;
            }
            return true;
        };
        TSharedPtr<SWidget> MainActions;
        if(Generate)if(const auto* GeneratePath=Paths.Find(Generate.Get()))
            for(int32 I=GeneratePath->Num()-1;I>=0;--I)
                if((*GeneratePath)[I]->GetType()==TEXT("SWrapBox")){MainActions=(*GeneratePath)[I];break;}
        int32 MainButtonCount=0;
        auto MainButtonsVisible=[&]()
        {
            MainButtonCount=0;bool AllVisible=MainActions.IsValid();
            for(const auto& Entry:Paths)
            {
                const auto& Path=Entry.Value;const auto& Widget=Path.Last();
                if(Widget->GetType()!=TEXT("SButton") || !MainActions || !Path.ContainsByPredicate(
                    [&](const TSharedRef<SWidget>& Ancestor){return &Ancestor.Get()==MainActions.Get();}))continue;
                ++MainButtonCount;AllVisible=VisibleInPanel(Widget)&&AllVisible;
            }
            return MainButtonCount==4&&AllVisible;
        };
        auto Capture=[&](const FString& Filename,FIntVector& Size,bool& Saved)
        {
            TArray<FColor> Pixels;Size=FIntVector(0,0,0);Saved=false;
            if(!FSlateApplication::Get().TakeScreenshot(Panel,Pixels,Size)
                ||Size.X<=0||Size.Y<=0||Pixels.Num()!=Size.X*Size.Y)return false;
            TArray64<uint8> Bytes;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Bytes);
            Saved=Bytes.Num()>0&&FFileHelper::SaveArrayToFile(Bytes,*Filename);
            return true;
        };
        FIntVector Size(0,0,0);bool Saved=false;
        const bool Captured=Capture(Png,Size,Saved);
        const double InputHeight=Input?Input->GetCachedGeometry().GetLocalSize().Y:0;
        const bool InputFullyVisible=VisibleInPanel(Input),GenerateFullyVisible=VisibleInPanel(Generate),ResultFullyVisible=VisibleInPanel(Result);
        const bool MainActionsFullyVisible=MainButtonsVisible();
        const bool ExpandedState=AreasFound&&StaticCastSharedPtr<SExpandableArea>(Rules)->IsExpanded()==Case.Expanded
            &&StaticCastSharedPtr<SExpandableArea>(Copy)->IsExpanded()==Case.Expanded;
        bool ScrollEndPassed=false,EndCaptured=false,EndSaved=false;
        double ScrollOffset=0,ScrollEnd=0,ViewFraction=1,InputHeightAtEnd=InputHeight;
        double PhysicalOffset=0,ViewOffsetFraction=0,ContentHeight=0,ViewportTop=0,ViewportBottom=0,TailBottom=0;
        int32 TailButtonCount=0;bool TailButtonsVisible=false;
        TArray<TSharedPtr<FJsonValue>> TailButtons;
        FIntVector EndSize(0,0,0);
        if(Case.Expanded&&AreasFound)
        {
            const auto Scroll=StaticCastSharedPtr<SScrollBox>(Advanced);
            Scroll->EndInertialScrolling();Scroll->ScrollToEnd();Settle();
            EndCaptured=Capture(EndPng,EndSize,EndSaved);
            ScrollOffset=Scroll->GetScrollOffset();ScrollEnd=Scroll->GetScrollOffsetOfEnd();ViewFraction=Scroll->GetViewFraction();
            const FSlateRect View=RectInPanel(AdvancedPanel.ToSharedRef()),Content=RectInPanel(Copy.ToSharedRef());
            ViewportTop=View.Top;ViewportBottom=View.Bottom;TailBottom=Content.Bottom;
            ContentHeight=AdvancedPanel->GetDesiredSize().Y;
            ViewOffsetFraction=Scroll->GetViewOffsetFraction();
            PhysicalOffset=StaticCastSharedPtr<SScrollPanel>(AdvancedPanel)->PhysicalOffset;
            TailButtonsVisible=true;
            for(const auto& Entry:Paths)
            {
                const auto& Path=Entry.Value;const auto& Widget=Path.Last();
                if(Widget->GetType()!=TEXT("SButton")||!Path.ContainsByPredicate(
                    [&](const TSharedRef<SWidget>& Ancestor){return &Ancestor.Get()==CopyActions.Get();}))continue;
                const FSlateRect Rect=RectInPanel(Widget);
                const bool ButtonVisible=VisibleInPanel(Widget)&&Contains(View,Rect);
                ++TailButtonCount;TailButtonsVisible=TailButtonsVisible&&ButtonVisible;
                const auto Button=MakeShared<FJsonObject>();Button->SetBoolField(TEXT("visible"),ButtonVisible);
                Button->SetNumberField(TEXT("left"),Rect.Left);Button->SetNumberField(TEXT("top"),Rect.Top);
                Button->SetNumberField(TEXT("right"),Rect.Right);Button->SetNumberField(TEXT("bottom"),Rect.Bottom);
                TailButtons.Add(MakeShared<FJsonValueObject>(Button));
            }
            InputHeightAtEnd=Input?Input->GetCachedGeometry().GetLocalSize().Y:0;
            // UE5.3.2 Slate/Private/Widgets/Layout/SScrollBox.cpp:332-358,650-675:
            // GetScrollOffset is a one-shot desired offset; Tick clamps the view fraction
            // and applies PhysicalOffset without writing the desired value back after resize.
            // Assert the applied offset AND current geometry, retaining the 1.5-pixel bound.
            ScrollEndPassed=FMath::IsFinite(ScrollOffset)&&FMath::IsFinite(ScrollEnd)&&FMath::IsFinite(ViewFraction)
                &&FMath::IsFinite(PhysicalOffset)&&FMath::IsFinite(ViewOffsetFraction)&&FMath::IsFinite(ContentHeight)
                &&ScrollOffset>=0&&ScrollEnd>=0&&ContentHeight>0&&ViewportBottom>ViewportTop
                &&FMath::Abs(PhysicalOffset-ScrollEnd)<=1.5&&ViewFraction>0&&ViewFraction<=1
                &&ViewOffsetFraction>=0&&ViewOffsetFraction<=1
                &&FMath::Abs(ViewOffsetFraction-(1.0-ViewFraction))*ContentHeight<=1.5
                &&FMath::Abs(TailBottom-ViewportBottom)<=1.5&&TailBottom>=ViewportTop
                &&TailButtonCount==3&&TailButtonsVisible
                &&InputHeightAtEnd>=160&&VisibleInPanel(Input)&&VisibleInPanel(Generate)&&VisibleInPanel(Result)&&MainButtonsVisible();
        }
        FString Text;
        if(Result&&Result->GetType()==TEXT("SMultiLineEditableTextBox"))
            Text=StaticCastSharedPtr<SMultiLineEditableTextBox>(Result)->GetText().ToString();
        const double ResultHeight=Result?Result->GetCachedGeometry().GetLocalSize().Y:0;
        const bool Passed=Captured&&Saved&&AreasFound&&ExpandedState&&InputFullyVisible&&ResultFullyVisible&&GenerateFullyVisible
            &&InputHeight>=160&&MainActionsFullyVisible&&(!Case.Expanded||(EndCaptured&&EndSaved&&ScrollEndPassed))
            && ResultHeight>0&&ResultHeight<=130&&Text.Len()<=1200&&Text.Contains(TEXT("PulseSpeed"))
            && !Text.Contains(TEXT("\"reportVersion\""))&&SourceHash==NodeForgePrototype::FileSha256(Preview);
        TestTrue(Label,Passed);
        const auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),Label);Item->SetBoolField(TEXT("passed"),Passed);
        Item->SetBoolField(TEXT("captured"),Captured);Item->SetStringField(TEXT("png"),Png);
        Item->SetNumberField(TEXT("pixelWidth"),Size.X);Item->SetNumberField(TEXT("pixelHeight"),Size.Y);
        Item->SetNumberField(TEXT("requestedClientWidth"),Case.Width);Item->SetNumberField(TEXT("requestedClientHeight"),Case.Height);
        Item->SetNumberField(TEXT("actualPanelWidth"),Panel->GetCachedGeometry().GetLocalSize().X);
        Item->SetNumberField(TEXT("actualPanelHeight"),Panel->GetCachedGeometry().GetLocalSize().Y);
        Item->SetNumberField(TEXT("inputHeight"),InputHeight);Item->SetNumberField(TEXT("minimumInputHeight"),160);
        Item->SetNumberField(TEXT("inputHeightAtScrollEnd"),InputHeightAtEnd);
        Item->SetBoolField(TEXT("advancedSectionsFound"),AreasFound);Item->SetBoolField(TEXT("expanded"),Case.Expanded);Item->SetBoolField(TEXT("scrollEndExecuted"),Case.Expanded&&AreasFound);
        Item->SetBoolField(TEXT("expansionStateMatched"),ExpandedState);
        Item->SetBoolField(TEXT("scrollEndPassed"),ScrollEndPassed);Item->SetBoolField(TEXT("scrollEndCaptured"),EndCaptured);
        Item->SetBoolField(TEXT("scrollEndSaved"),EndSaved);Item->SetNumberField(TEXT("mainButtonCount"),MainButtonCount);
        Item->SetBoolField(TEXT("mainActionsFullyVisible"),MainActionsFullyVisible);
        Item->SetNumberField(TEXT("scrollOffset"),ScrollOffset);Item->SetNumberField(TEXT("scrollOffsetOfEnd"),ScrollEnd);
        Item->SetNumberField(TEXT("scrollViewFraction"),ViewFraction);
        Item->SetNumberField(TEXT("scrollDesiredOffset"),ScrollOffset);Item->SetNumberField(TEXT("scrollPhysicalOffset"),PhysicalOffset);
        Item->SetNumberField(TEXT("scrollViewOffsetFraction"),ViewOffsetFraction);Item->SetNumberField(TEXT("scrollContentHeight"),ContentHeight);
        Item->SetNumberField(TEXT("scrollViewportTop"),ViewportTop);Item->SetNumberField(TEXT("scrollViewportBottom"),ViewportBottom);
        Item->SetNumberField(TEXT("scrollTailBottom"),TailBottom);Item->SetNumberField(TEXT("tailButtonCount"),TailButtonCount);
        Item->SetBoolField(TEXT("tailButtonsFullyVisible"),TailButtonsVisible);Item->SetArrayField(TEXT("tailButtons"),TailButtons);
        if(Case.Expanded){Item->SetStringField(TEXT("scrollEndPng"),EndPng);Item->SetNumberField(TEXT("scrollEndPixelWidth"),EndSize.X);Item->SetNumberField(TEXT("scrollEndPixelHeight"),EndSize.Y);}
        Item->SetNumberField(TEXT("resultHeight"),ResultHeight);Item->SetNumberField(TEXT("summaryCharacters"),Text.Len());
        Item->SetBoolField(TEXT("inputVisible"),InputFullyVisible);Item->SetBoolField(TEXT("generateVisible"),GenerateFullyVisible);
        Item->SetBoolField(TEXT("resultVisible"),ResultFullyVisible);Item->SetStringField(TEXT("summary"),Text);Rows.Add(MakeShared<FJsonValueObject>(Item));
        // This is the uniquely owned review window, never an existing user editor/window.
        FSlateApplication::Get().RequestDestroyWindow(Window);FSlateApplication::Get().Tick();
    }
    const auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("test"),TEXT("NodeForge.PanelPresentation"));
    Report->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Report->SetArrayField(TEXT("cases"),Rows);
    Report->SetStringField(TEXT("sourceReport"),Preview);Report->SetBoolField(TEXT("sourceReportUnchanged"),SourceHash==NodeForgePrototype::FileSha256(Preview));
    Report->SetStringField(TEXT("moduleFilename"),FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")));
    Report->SetStringField(TEXT("moduleFileSha256"),NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
    Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("scope"),TEXT("Actual native Slate rendering and allotted/clipped geometry of the production factory in collapsed and natively expanded states; input must retain at least 160 logical pixels and advanced content must scroll to its end. No mouse click, import/file dialog/clipboard or human approval implied."));
    return NodeForgePrototype::WriteJson(Report,Output)&&!HasAnyErrors();
}
#endif
