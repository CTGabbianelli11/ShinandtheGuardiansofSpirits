#include "Enemy/Enemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "RhyaTowerOfWishes/DebugMacros.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/AttributeComponent.h"
#include "NiagaraComponent.h"
#include "Components/AC_HitStop.h"
#include "NiagaraFunctionLibrary.h"
#include "Misc/ConfigCacheIni.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	/*
	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	*/

	// [TwstdTree] Set mesh/capsule collision profile using config-driven name from DefaultGame.ini.
	// Prevents errors caused by preset name changes in Project Settings.
	FString MeshProfileName, CapsuleProfileName;
	GConfig->GetString(TEXT("Game.CollisionProfiles"), TEXT("EnemyMeshProfile"), MeshProfileName, GGameIni);
	GetMesh()->SetCollisionProfileName(FName(*MeshProfileName));
	GetMesh()->SetGenerateOverlapEvents(true);
	GConfig->GetString(TEXT("Game.CollisionProfiles"), TEXT("EnemyCapsuleProfile"), CapsuleProfileName, GGameIni);
	GetCapsuleComponent()->SetCollisionProfileName(*CapsuleProfileName);

	hitStopComponent = CreateDefaultSubobject<UAC_HitStop>(TEXT("HitStop"));

	attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));
}

void AEnemy::PlayHitReactMontage(const FName& sectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);

		//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, sectionName.ToString(), false);
		AnimInstance->Montage_JumpToSection(sectionName, HitReactMontage);
	}
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed * attributes->GetSpeed();

	CustomTimeDilation = attributes->GetSpeed();

	RootComponent->SetWorldScale3D(FVector::One() * attributes->GetSize());

	if (hitStopComponent)
		hitStopComponent->SetStartTimeDilation(CustomTimeDilation);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemy::GetHit(const FVector& impactPoint, const FVector& impactDirection)
{
	//DRAW_SPHERE_COLOR(impactPoint,FColor::Orange);

	const FVector impactLowered(impactPoint.X, impactPoint.Y, GetActorLocation().Z);
	const FVector ToHit = (impactLowered - GetActorLocation()).GetSafeNormal();

	if (hitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, hitSound, GetActorLocation(), 1, 1, .1f);
	}

	if (HitSystem)
	{
		const UWorld* World = GetWorld();

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitSystem, impactPoint, ToHit.Rotation());
	}
	OnCharacterHit(impactPoint, ToHit);

	if (attributes && attributes->IsAlive())
	{
		//hitStopComponent->BeginHitStop(.2f,0,30,20,true);
		DirectionalHitReact(impactPoint, ToHit);
	}
}

void AEnemy::CharacterDied()
{
	if (GetController())
		GetController()->UnPossess();

	StopAnimMontage();

	EnableRagdoll();

	DropCurrency();

	OnCharacterDied();

	GetMesh()->SetAnimInstanceClass(nullptr);
}

void AEnemy::DropCurrency()
{
	if (attributes->GetCurrency() <= 0)
		return;

	UWorld* World = GetWorld();
	if (World && CurrencyToDrop)
	{
		FVector location = GetActorLocation();
		location.Z += 25.f;

		attributes->AddCurrency(-1);

		World->SpawnActor<AActor>(CurrencyToDrop, location, GetActorRotation());
	}
}

void AEnemy::DirectionalHitReact(const FVector& impactPoint, const FVector impactDirection)
{
	const FVector Forward = GetActorForwardVector();

	//Forard * To hit = |Forward||ToHit| * cos(theta)
	//Forward
	const double CosTheta = FVector::DotProduct(Forward, impactDirection);
	//take inverse cos (arccosin) of cos(theta) to get theta
	double Theta = FMath::Acos(CosTheta);
	//convert from radians to degrees
	Theta = FMath::RadiansToDegrees(Theta);

	//if cross product points down theta is negative
	FVector CrossProduct = FVector::CrossProduct(Forward, impactDirection);
	if (CrossProduct.Z < 0)
	{
		Theta *= -1.f;
	}
	//UKismetSystemLibrary::DrawDebugArrow(this, GetActorLocation(), GetActorLocation() + CrossProduct * 100.f, 5.f, FColor::Blue, 5.f);

	FName Section("FromLeft");
	//if (Theta >= -45.f && Theta < 45.f)
	//{
	//Section = FName("FromFront");
	//}
	//Note : Uncomment when animations in montage are added
	if (Theta >= -135.f && Theta < 45.f)
	{
		Section = FName("FromRight");
	}
	//else if( Theta >= 45.f && Theta < 135.f)
	//{
	//	Section = FName("FromRight");
	//}
	//else
	//{
	//	Section = FName("FromBack");
	//}

	//GetMesh()->AddImpulseAtLocation(ToHit*100000.f,GetActorLocation(),FName("Root"));

	PlayHitReactMontage(Section);

	//NOTE: Move to health logic later

	//if (GEngine)
	//{
	//	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, FString::Printf(TEXT("Theta: %f"), Theta), false);

	//}
	//UKismetSystemLibrary::DrawDebugArrow(this, GetActorLocation(), GetActorLocation() + Forward * 60.f, 5.f, FColor::Red, 5.f);
	//UKismetSystemLibrary::DrawDebugArrow(this, GetActorLocation(), GetActorLocation() + ToHit * 60.f, 5.f, FColor::Green, 5.f);
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (attributes)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Example text that prints a float: %f"), DamageAmount), false);

		attributes->RecieveDamage(DamageAmount);

		if (!attributes->IsAlive())
		{
			CharacterDied();
		}
	}
	return DamageAmount;
}

void AEnemy::EnableRagdoll()
{
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	UWorld* world = GetWorld();
	if (world)
	{
		ACharacter* character = UGameplayStatics::GetPlayerCharacter(world, 0);

		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

		// Prevent ragdolled enemy meshes from interfering with the player camera by setting their response to the camera collision channel to 'ignore'.
		// This ensures the camera can move freely near ragdoll meshes without being blocked.
		GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

		if (character)
			GetMesh()->AddImpulse((character->GetActorForwardVector() * 200000.f) + FVector::UpVector * 20000.f);
	}
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
