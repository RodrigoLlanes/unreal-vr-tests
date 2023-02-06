// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AGrabbableActor.h"
#include "ATimeGranade.generated.h"

/**
 * 
 */
UCLASS()
class CURSOUNREALVR_API AATimeGranade : public AAGrabbableActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AATimeGranade();

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* ShieldMesh;

	void Activate();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	// Configuration parameters
	UPROPERTY(EditAnywhere)
		float Delay = 2;

	UPROPERTY(EditAnywhere)
		float Duration = 10;

	UPROPERTY(EditAnywhere)
		float MaxScale = 2;

	UPROPERTY(EditAnywhere)
		float TimeDilation = 1;

	UPROPERTY(EditAnywhere)
		float GrowingTime = 1;

	UPROPERTY(EditAnywhere)
		float MaxIntensity = 100;

	UPROPERTY(EditAnywhere)
		float MinIntensity = 1;

	UPROPERTY(EditAnywhere)
		float MaxFlickerSpeed = 2;

	UPROPERTY(EditAnywhere)
		float MinFlickerSpeed = 0.5f;

	void Start();
	void Finish();

	UFUNCTION()
	void BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);
	UFUNCTION()
	void EndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	FVector TargetScale;
	FLinearColor BaseColor;

	UMaterialInstanceDynamic* MShield;
	UMaterialInstanceDynamic* MGranade;

	int State = -1;
	float ElapsedTime = 0;
};
