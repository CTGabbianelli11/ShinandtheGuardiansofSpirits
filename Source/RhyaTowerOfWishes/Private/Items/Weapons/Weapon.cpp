
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

AWeapon::AWeapon()
{
    // --- Class-owned Components ---
    WeaponBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Box Collider"));
    WeaponBoxComponent->SetupAttachment(RootComponent);
    WeaponBoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // [TwstdTree] set collisions to Enemy Collision Channel only
    WeaponBoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    WeaponBoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Overlap);
    WeaponBoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);

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

    // [TwstdTree] Convert ECC_GameTraceChannel1 to EObjectTypeQuery
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1));
     UKismetSystemLibrary::BoxTraceSingleForObjects(
        this,
        Start,
        End,
        HalfSize,
        BoxTraceStart ? BoxTraceStart->GetComponentRotation() : FRotator::ZeroRotator,
        ObjectTypes,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        BoxHit,
        true
    );

    if (AActor* HitActor = BoxHit.GetActor())
    {
        // Apply damage
        UGameplayStatics::ApplyDamage(
            HitActor,
            damage,
            GetInstigator() ? GetInstigator()->GetController() : nullptr,
            this,
            UDamageType::StaticClass()
        );

        // Post-damage hit reaction
        if (IHitInterface* HitInterface = Cast<IHitInterface>(HitActor))
        {
            HitInterface->GetHit(BoxHit.ImpactPoint,BoxHit.ImpactNormal); 
            
            if (Cast<ACharacter>(HitActor))
            {
                ACharacter* HitCharcter = Cast<ACharacter>(HitActor);

                FVector DirectionFromWeaponHolder = HitActor->GetActorLocation() - GetOwner()->GetActorLocation();
                DirectionFromWeaponHolder.Z = 0;
                DirectionFromWeaponHolder *= KnockBackAmount;

                HitCharcter->LaunchCharacter(DirectionFromWeaponHolder,false,false);
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

