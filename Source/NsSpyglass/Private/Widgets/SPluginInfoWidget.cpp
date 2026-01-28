// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Widgets/SPluginInfoWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    bool IsDependencyInclude(const FString& IncludePath, const FString& DependencySourceRoot, const TArray<FString>& DependencyModuleNames)
    {
        if (IncludePath.IsEmpty())
        {
            return false;
        }

        auto FileExists = [&DependencySourceRoot](const FString& Candidate)
        {
            return IFileManager::Get().FileExists(*Candidate);
        };

        if (FileExists(FPaths::Combine(DependencySourceRoot, IncludePath)))
        {
            return true;
        }

        for (const FString& ModuleName : DependencyModuleNames)
        {
            const FString ModuleRoot = FPaths::Combine(DependencySourceRoot, ModuleName);
            if (FileExists(FPaths::Combine(ModuleRoot, IncludePath)))
            {
                return true;
            }
            if (FileExists(FPaths::Combine(ModuleRoot, TEXT("Public"), IncludePath)))
            {
                return true;
            }
            if (FileExists(FPaths::Combine(ModuleRoot, TEXT("Private"), IncludePath)))
            {
                return true;
            }
        }

        return false;
    }

    TMap<FString, TArray<FString>> FindDependencyReferences(const TSharedPtr<IPlugin>& SourcePlugin, const TSharedPtr<IPlugin>& DependencyPlugin)
    {
        TMap<FString, TArray<FString>> Matches;
        if (!SourcePlugin || !DependencyPlugin)
        {
            return Matches;
        }

        TArray<FString> DependencyModuleNames;
        const FPluginDescriptor& DepDesc = DependencyPlugin->GetDescriptor();
        DependencyModuleNames.Reserve(DepDesc.Modules.Num());
        for (const FModuleDescriptor& Mod : DepDesc.Modules)
        {
            DependencyModuleNames.Add(Mod.Name.ToString());
        }

        const FString DependencySourceRoot = FPaths::Combine(DependencyPlugin->GetBaseDir(), TEXT("Source"));
        const FPluginDescriptor& SourceDesc = SourcePlugin->GetDescriptor();
        const FString SourceDir = SourcePlugin->GetBaseDir();
        const FString SourceRoot = FPaths::Combine(SourceDir, TEXT("Source"));
        const TArray<FString> Extensions = {TEXT("*.h"), TEXT("*.hpp"), TEXT("*.cpp"), TEXT("*.inl")};

        for (const FModuleDescriptor& SourceModule : SourceDesc.Modules)
        {
            const FString ModuleName = SourceModule.Name.ToString();
            const FString ModuleRoot = FPaths::Combine(SourceRoot, ModuleName);
            if (!IFileManager::Get().DirectoryExists(*ModuleRoot))
            {
                continue;
            }

            TArray<FString> ModuleFiles;
            for (const FString& Extension : Extensions)
            {
                IFileManager::Get().FindFilesRecursive(ModuleFiles, *ModuleRoot, *Extension, true, false);
            }

            for (const FString& SourceFile : ModuleFiles)
            {
                TArray<FString> Lines;
                if (!FFileHelper::LoadFileToStringArray(Lines, *SourceFile))
                {
                    continue;
                }

                bool bFound = false;
                for (const FString& Line : Lines)
                {
                    const int32 IncludeIndex = Line.Find(TEXT("#include"));
                    if (IncludeIndex == INDEX_NONE)
                    {
                        continue;
                    }

                    int32 Start = Line.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, IncludeIndex);
                    TCHAR Closing = TEXT('"');
                    if (Start == INDEX_NONE)
                    {
                        Start = Line.Find(TEXT("<"), ESearchCase::IgnoreCase, ESearchDir::FromStart, IncludeIndex);
                        Closing = TEXT('>');
                    }

                    if (Start == INDEX_NONE)
                    {
                        continue;
                    }

                    const int32 End = Line.Find(FString::Chr(Closing), ESearchCase::IgnoreCase, ESearchDir::FromStart, Start + 1);
                    if (End == INDEX_NONE)
                    {
                        continue;
                    }

                    const FString IncludePath = Line.Mid(Start + 1, End - Start - 1).TrimStartAndEnd();
                    if (IsDependencyInclude(IncludePath, DependencySourceRoot, DependencyModuleNames))
                    {
                        bFound = true;
                        break;
                    }
                }

                if (bFound)
                {
                    TArray<FString>& ModuleMatches = Matches.FindOrAdd(ModuleName);
                    ModuleMatches.Add(SourceFile);
                }
            }
        }

        return Matches;
    }
}

void SPluginInfoWidget::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SBox)
        .MinDesiredWidth(300)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SAssignNew(NameText, STextBlock)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                [
                    SAssignNew(DescriptionText, STextBlock).WrapTextAt(250.f)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                [
                    SAssignNew(AuthorText, STextBlock)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
                [
                    SAssignNew(DocsLink, SHyperlink)
                    .Text(FText::FromString("Documentation"))
                    .OnNavigate(this, &SPluginInfoWidget::OnDocsClicked)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
                [
                    SNew(SSeparator)
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString("Modules:"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SAssignNew(ModulesBox, SVerticalBox)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f)
                [
                    SNew(SSeparator)
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString("Depends on:"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SAssignNew(DependenciesBox, SVerticalBox)
                ]
            ]
        ]
    ];

    SetPlugin(nullptr);
}

void SPluginInfoWidget::SetPlugin(TSharedPtr<IPlugin> InPlugin)
{
    CurrentPlugin = InPlugin;

    if (!CurrentPlugin)
    {
        NameText->SetText(FText::FromString("Hover a node"));
        DescriptionText->SetText(FText());
        AuthorText->SetText(FText());
        DocsLink->SetVisibility(EVisibility::Collapsed);
        ModulesBox->ClearChildren();
        DependenciesBox->ClearChildren();
        return;
    }

    const FPluginDescriptor& Desc = CurrentPlugin->GetDescriptor();

    NameText->SetText(FText::FromString(CurrentPlugin->GetFriendlyName()));
    DescriptionText->SetText(FText::FromString(Desc.Description));
    AuthorText->SetText(FText::FromString(Desc.CreatedBy));

    DocsURL = Desc.DocsURL;
    DocsLink->SetVisibility(DocsURL.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);

    ModulesBox->ClearChildren();
    for (const FModuleDescriptor& Mod : Desc.Modules)
    {
        FLinearColor Color = (Mod.Type == EHostType::Runtime) ? FLinearColor(0.2f, 0.6f, 1.f) : FLinearColor(1.f, 1.f, 0.1f);
        ModulesBox->AddSlot().AutoHeight()
        [
            SNew(STextBlock).ColorAndOpacity(Color).Text(FText::FromName(Mod.Name))
        ];
    }

    DependenciesBox->ClearChildren();
    for (const FPluginReferenceDescriptor& Ref : Desc.Plugins)
    {
        if (!Ref.bEnabled)
        {
            continue;
        }

        const TSharedPtr<IPlugin> DependencyPlugin = IPluginManager::Get().FindPlugin(Ref.Name);
        const TMap<FString, TArray<FString>> ReferencesByModule = FindDependencyReferences(CurrentPlugin, DependencyPlugin);
        TArray<FString> SummaryLines;
        TArray<FString> FullLines;
        for (const TPair<FString, TArray<FString>>& Pair : ReferencesByModule)
        {
            TArray<FString> FileNames;
            FileNames.Reserve(Pair.Value.Num());
            for (const FString& Reference : Pair.Value)
            {
                FileNames.Add(FPaths::GetCleanFilename(Reference));
            }
            SummaryLines.Add(FString::Printf(TEXT("%s: %s"), *Pair.Key, *FString::Join(FileNames, TEXT(", "))));
            FullLines.Add(FString::Printf(TEXT("%s:"), *Pair.Key));
            for (const FString& Reference : Pair.Value)
            {
                FullLines.Add(FString::Printf(TEXT("  %s"), *Reference));
            }
        }

        const FString ReferenceText = SummaryLines.Num() > 0
            ? FString::Join(SummaryLines, TEXT(" | "))
            : TEXT("Not found in source files");

        DependenciesBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock).Text(FText::FromString(Ref.Name))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(TEXT("Referenced in %s"), *ReferenceText)))
                .ToolTipText(FText::FromString(FullLines.Num() > 0 ? FString::Join(FullLines, TEXT("\n")) : ReferenceText))
                .ColorAndOpacity(FLinearColor(0.65f, 0.65f, 0.65f))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
            ]
        ];
    }
}

void SPluginInfoWidget::OnDocsClicked() const
{
    if (!DocsURL.IsEmpty())
    {
        FPlatformProcess::LaunchURL(*DocsURL, nullptr, nullptr);
    }
}
