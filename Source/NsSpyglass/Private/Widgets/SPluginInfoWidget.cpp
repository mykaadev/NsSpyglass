// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Widgets/SPluginInfoWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

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
    const FString DescriptorPath = CurrentPlugin->GetDescriptorFileName();
    const FString DescriptorFile = DescriptorPath.IsEmpty() ? TEXT("Unknown") : FPaths::GetCleanFilename(DescriptorPath);
    for (const FPluginReferenceDescriptor& Ref : Desc.Plugins)
    {
        if (!Ref.bEnabled)
        {
            continue;
        }

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
                .Text(FText::FromString(FString::Printf(TEXT("Declared in %s"), *DescriptorFile)))
                .ToolTipText(FText::FromString(DescriptorPath))
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
