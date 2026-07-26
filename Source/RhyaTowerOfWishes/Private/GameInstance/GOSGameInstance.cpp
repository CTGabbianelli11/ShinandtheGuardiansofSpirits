// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/GOSGameInstance.h"
#include "Subsytems/UISubsystem.h"

void UGOSGameInstance::Init()
{
    // Force the Blueprint subsystem class into memory
    LoadClass<UUISubsystem>(nullptr, TEXT("/Game/Blueprints/Subsystems/BP_UISubsystem.BP_UISubsystem_C"));

    Super::Init();
}