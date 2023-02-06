// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "HandController.h"

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"


UCLASS()
class CURSOUNREALVR_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Constructor
	AVRCharacter();


	// Public state properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool BlinkersEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool TeleportEnabled = true;


protected:
	virtual void BeginPlay() override;


public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


private:
	void MoveForward(float throttle);
	void MoveRight(float throttle);
	FVector2D GetBlinkerCenter();
	void UpdateBlinkers();

	bool FindTeleportDestination(TArray<FVector> &Path, FVector &OutLocation);
	void UpdateDestinationMarker();
	void UpdatePathSpline(const TArray<FVector>& Path);
	void UpdatePathMesh(const TArray<FVector>& Path);
	void BeginTeleport();
	void FinishTeleport();

	void StartTeleportFade(float FromAlpha, float ToAlpha);

	void GripLeft();
	void ReleaseLeft();
	void GripRight();
	void ReleaseRight();
	void MenuButton();

	void SwitchTeleportEnabled();
	void SwitchBlinkersEnabled();


private:
	// Default sub objects
	UPROPERTY()
	class UCameraComponent* Camera;

	UPROPERTY()
	class USceneComponent* VRRoot;


	// References
	UPROPERTY()
	class AHandController* LeftController;

	UPROPERTY()
	class AHandController* RightController;

	//UPROPERTY()
	//class AActor* ConfigMenu;



	UPROPERTY(VisibleAnywhere)
	class USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY()
	class UPostProcessComponent* PostProcessComponent;

	UPROPERTY()
	class UMaterialInstanceDynamic* BlinkerMaterialInstance;

	UPROPERTY()
	TArray<class USplineMeshComponent*> TeleportPathMeshPool;

	
private: 
	// Configuration parameters
	UPROPERTY(EditAnywhere)
		float WalkingSpeed = 2;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileSpeed = 1000;

	UPROPERTY(EditAnywhere)
	float TeleportProjectileRadius = 10;

	UPROPERTY(EditAnywhere)
	float TeleportSimulationTime = 3;

	UPROPERTY(EditAnywhere)
	float TeleportTime = 1;

	UPROPERTY(EditAnywhere)
	FVector TeleportProjectionExtent = FVector(100, 100, 100);

	UPROPERTY(EditAnywhere)
	class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY(EditAnywhere)
	class UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditDefaultsOnly)
	class UStaticMesh* TeleportArchMesh;

	UPROPERTY(EditDefaultsOnly)
	class UMaterialInterface* TeleportArchMaterial;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AHandController> HandControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class AActor> ConfigMenuClass;
};
