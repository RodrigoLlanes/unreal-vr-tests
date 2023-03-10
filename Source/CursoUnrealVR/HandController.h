// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AGrabbableActor.h"



#include "MotionControllerComponent.h"
#include "Components/WidgetInteractionComponent.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandController.generated.h"

UCLASS()
class CURSOUNREALVR_API AHandController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHandController();

	void SetHand(EControllerHand Hand);
	void PairController(AHandController* Controller);

	void Grip();
	void Release();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	// Callbacks
	UFUNCTION()
	void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);


	// Helpers
	bool CanClimb() const;
	AAGrabbableActor* GetGrabTarget();


	// Default sub objects
	UPROPERTY(VisibleAnywhere)
	UMotionControllerComponent* MotionController;


	//Parameters
	UPROPERTY(EditDefaultsOnly)
	class UHapticFeedbackEffect_Base* HapticEffect;


	// State
	bool bCanClimb = false;
	bool bIsClimbing = false;
	FVector ClimbingStartLocation;
	FVector Velocity;
	FVector PrevPosition;

	AAGrabbableActor* GrabbedActor;

	AHandController* OtherController;
};
