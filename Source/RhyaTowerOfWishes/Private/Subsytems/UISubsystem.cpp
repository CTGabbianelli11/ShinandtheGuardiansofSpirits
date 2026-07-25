#include "Subsytems/UISubsystem.h"

void UUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //Fire blueprint init event
    K2_Initialize();
}

void UUISubsystem::Deinitialize()
{
    Super::Deinitialize();
}