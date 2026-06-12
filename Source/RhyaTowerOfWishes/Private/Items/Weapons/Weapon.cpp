
// Weapon.cpp
#include "Items/Weapons/Weapon.h"
#include "Characters/CombatPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Interfaces/HitInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AC_HitStop.h"
#include "Components/AttributeComponent.h"
#include "Enemy/Enemy.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

AWeapon::AWeapon()
{
    // --- Class-owned Components ---
    WeaponBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Box Collider"));
    WeaponBoxComponent->SetupAttachment(RootComponent);
    WeaponBoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    WeaponBoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);


    BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
    BoxTraceStart->SetupAttachment(RootComponent);

    BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
    BoxTraceEnd->SetupAttachment(RootComponent);

}

void AWeapon::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
    UE_LOG(LogTemp, Log, TEXT("AWeapon::Equip called on %s, Owner=%s, Instigator=%s"),
        *GetName(), *GetNameSafe(GetOwner()), *GetNameSafe(GetInstigator()));

    SetOwner(NewOwner);
    SetInstigator(NewInstigator);

    const FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);

    AttachToComponent(InParent, TransformRules, InSocketName);

    // Maintain existing behavior: sphere off in-hand; hit box gated by anim notifies
    if (sphereCollider)
    {
        sphereCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (WeaponBoxComponent)
    {
        WeaponBoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ignoreActors.Empty();
    }
}


void AWeapon::SetEquipped(bool bInEquipped)
{
    bIsEquipped = bInEquipped;
    OnEquipped.Broadcast(bIsEquipped);
}

void AWeapon::BeginPlay()
{
    Super::BeginPlay();

    if (WeaponBoxComponent)
    {

        WeaponBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnBoxOverlap);
    }
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AWeapon::OnEndSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    Super::OnEndSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
}

void AWeapon::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    //GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Debug hit")));


    // Use the trace anchors for start/end
    const FVector Start = BoxTraceStart ? BoxTraceStart->GetComponentLocation() : FVector::ZeroVector;
    const FVector End = BoxTraceEnd ? BoxTraceEnd->GetComponentLocation() : FVector::ZeroVector;

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(this);
    for (AActor* Actor : ignoreActors)
    {
        ActorsToIgnore.AddUnique(Actor);
    }

    FHitResult BoxHit;
    FVector HalfSize = WeaponBoxComponent->GetScaledBoxExtent();
    HalfSize.Z = 10.f;

     UKismetSystemLibrary::BoxTraceSingle(
        this,
        Start,
        End,
        HalfSize,
        BoxTraceStart ? BoxTraceStart->GetComponentRotation() : FRotator::ZeroRotator,
        ETraceTypeQuery::TraceTypeQuery1,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        BoxHit,
        true
    );

    if (AActor* HitActor = BoxHit.GetActor())
    {
        // Read liveness before ApplyDamage: a killing blow still awards magic.
        AEnemy* HitEnemy = Cast<AEnemy>(HitActor);
        const bool bTargetWasAlive =
            HitEnemy && HitEnemy->GetAttributes() && HitEnemy->GetAttributes()->IsAlive();

        // Apply damage
        UGameplayStatics::ApplyDamage(
            HitActor,
            damage,
            GetInstigator() ? GetInstigator()->GetController() : nullptr,
            this,
            UDamageType::StaticClass()
        );

        // ignoreActors limits this to one award per enemy per swing.
        bool bAwardedMagic = false;
        bool bMagicChanged = false;
        if (bTargetWasAlive)
        {
            if (ACombatPlayerCharacter* PlayerOwner = Cast<ACombatPlayerCharacter>(GetOwner()))
            {
                if (UAttributeComponent* PlayerAttributes = PlayerOwner->GetAttributes())
                {
                    const float MagicPctBefore = PlayerAttributes->GetMagicPercentage();
                    PlayerAttributes->AddMagic(MagicGainPerHit);
                    bAwardedMagic = true;
                    // AddMagic clamps — don't display a gain that didn't happen.
                    bMagicChanged = PlayerAttributes->GetMagicPercentage() != MagicPctBefore;
                }
            }
        }

        // Rhya.Debug.Combat is registered in CombatPlayerCharacter.cpp.
        static IConsoleVariable* CombatDebugCVar =
            IConsoleManager::Get().FindConsoleVariable(TEXT("Rhya.Debug.Combat"));
        if (CombatDebugCVar && CombatDebugCVar->GetInt() != 0)
        {
            FString Line = FString::Printf(TEXT("%.1f dmg"), damage);
            if (bAwardedMagic && bMagicChanged)
            {
                Line += FString::Printf(TEXT("  +%.0f MP"), MagicGainPerHit);
            }
            else if (bAwardedMagic)
            {
                Line += TEXT("  (MP full)");
            }
            else if (HitEnemy && !bTargetWasAlive)
            {
                Line += TEXT("  (corpse - no MP)");
            }
            DrawDebugString(GetWorld(), BoxHit.ImpactPoint, Line, nullptr,
                FColor::Orange, 1.8f, /*bDrawShadow*/ true, /*FontScale*/ 1.25f);
        }

        bool hitInterfaceImplemented =
            HitActor->Implements<UHitInterface>();
        // Post-damage hit reaction
        if (hitInterfaceImplemented)
        {
            IHitInterface::Execute_GetHit(HitActor,BoxHit.ImpactPoint,BoxHit.ImpactNormal);
            
            if (Cast<ACharacter>(HitActor))
            {
                ACharacter* HitCharcter = Cast<ACharacter>(HitActor);

                FVector DirectionFromWeaponHolder = HitActor->GetActorLocation() - GetOwner()->GetActorLocation();
                DirectionFromWeaponHolder.Z = 0;
                DirectionFromWeaponHolder *= KnockBackAmount;
                //Removed pushback for characters being hit 
                //HitCharcter->LaunchCharacter(DirectionFromWeaponHolder,false,false);
            }

            if (Cast<ACombatPlayerCharacter>(GetOwner()))
            {
                Cast<ACombatPlayerCharacter>(GetOwner())->hitStopComponent->BeginHitStop(.15f,.05,30,1,false);
            }
        }

        ignoreActors.AddUnique(HitActor);

        if (HitSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, HitSound, BoxHit.ImpactPoint, 1.f, 1.f, 0.1f);
        }


    }
    else
    {
        if (MissSoundEffect)
        {
            UGameplayStatics::PlaySoundAtLocation(this, MissSoundEffect, GetActorLocation(), 1.f, 1.f, 0.1f);
        }
    }
}

