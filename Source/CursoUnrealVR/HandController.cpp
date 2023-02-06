// Fill out your copyright notice in the Description page of Project Settings.


#include "HandController.h"

#include "MotionControllerComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"

#include "ATimeGranade.h"

// Sets default values
AHandController::AHandController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	MotionController->SetupAttachment(GetRootComponent());
	MotionController->bDisplayDeviceModel = true;
}

void AHandController::SetHand(EControllerHand Hand)
{
	MotionController->SetTrackingSource(Hand);
}

void AHandController::PairController(AHandController* Controller)
{
	OtherController = Controller;
	OtherController->OtherController = this;
}

void AHandController::Grip()
{
	if (bCanClimb) {
		OtherController->bIsClimbing = false;
		bIsClimbing = true;
		ClimbingStartLocation = GetActorLocation();
	} else {
		AAGrabbableActor* Target = GetGrabTarget();
		if (Target != nullptr) {
			Target->StaticMesh->SetSimulatePhysics(false);
			Target->StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Target->StaticMesh->AttachToComponent(MotionController, FAttachmentTransformRules::KeepWorldTransform);
			GrabbedActor = Target;
		}
	}
}

void AHandController::Release()
{
	ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
	if (bIsClimbing && Character != nullptr) {
		Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}
	bIsClimbing = false;

	if (GrabbedActor != nullptr) {
		GrabbedActor->StaticMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		GrabbedActor->StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GrabbedActor->StaticMesh->SetSimulatePhysics(true);
		GrabbedActor->StaticMesh->SetPhysicsLinearVelocity(Velocity * this->GetWorld()->GetWorldSettings()->TimeDilation);

		if (GrabbedActor->GetClass()->IsChildOf(AATimeGranade::StaticClass())) {
			Cast<AATimeGranade>(GrabbedActor)->Activate();
		}

		GrabbedActor = nullptr;
	}
}

// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();
	
	OnActorBeginOverlap.AddDynamic(this, &AHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AHandController::ActorEndOverlap);
}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
	if (bIsClimbing && Character != nullptr) {
		FVector Delta = ClimbingStartLocation - GetActorLocation();
		Character->AddActorWorldOffset(Delta);
		Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	}

	FVector CurrPosition = GetActorLocation();
	Velocity = (CurrPosition - PrevPosition) / DeltaTime;
	PrevPosition = CurrPosition;
}

void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bool bNewCanClimb = CanClimb();
	if (!bCanClimb && bNewCanClimb) {
		APawn* Pawn = Cast<APawn>(GetAttachParentActor());
		if (Pawn != nullptr) {
			APlayerController* Controller = Cast<APlayerController>(Pawn->GetController());
			if (Controller != nullptr) {
				Controller->PlayHapticEffect(HapticEffect, MotionController->GetTrackingSource());
			}
		}
	}
	bCanClimb = CanClimb();
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bCanClimb = CanClimb();
}

bool AHandController::CanClimb() const {
	TArray<AActor*> OverlappingActors;
	this->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors) {
		if (Actor->ActorHasTag(TEXT("Climbable"))) { 
			return true; 
		}
	}
	return false;
}

AAGrabbableActor* AHandController::GetGrabTarget()
{
	TArray<UPrimitiveComponent*> OverlappingComponents;
	this->GetOverlappingComponents(OverlappingComponents);
	for (UPrimitiveComponent* Component : OverlappingComponents) {
		if (Component->ComponentHasTag(TEXT("Grabbable")) && Component->GetOwner()->GetClass()->IsChildOf(AAGrabbableActor::StaticClass())) {
			return Cast< AAGrabbableActor>(Component->GetOwner());
		}
	}
	return nullptr;
}
