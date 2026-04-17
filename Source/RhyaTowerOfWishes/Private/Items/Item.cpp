// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Item.h"
#include "RhyaTowerOfWishes/DebugMacros.h"
#include "Components/SphereComponent.h"
#include "Characters/CombatPlayerCharacter.h"

#define THIRTY 30


// Sets default values
AItem::AItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item Mesh Component"));
	RootComponent=ItemMesh;

	sphereCollider = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere Collider Component"));
	sphereCollider->SetupAttachment(ItemMesh);
}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();

	sphereCollider->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap);
	sphereCollider->OnComponentEndOverlap.AddDynamic(this, &AItem::OnEndSphereOverlap);
}

float AItem::TransformSine(float value)
{
	return amplitude * FMath::Sin(value * timeConstant);
}

float AItem::TransformCosine(float value)
{
	return amplitude * FMath::Cos(value * timeConstant);
}

void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (GEngine)
	{
		ACombatPlayerCharacter* character = Cast<ACombatPlayerCharacter>(OtherActor);
		if (character)
			character->SetOverlappingItem(this);
	}
}

void AItem::OnEndSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (GEngine)
	{
		ACombatPlayerCharacter* character = Cast<ACombatPlayerCharacter>(OtherActor);
		if (character)
			character->SetOverlappingItem(nullptr);
	}
}


// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	runningTime += DeltaTime;


}

