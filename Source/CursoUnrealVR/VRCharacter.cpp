// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "InputCoreTypes.h"
#include "NavigationSystem.h"
#include "Curves/CurveFloat.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Blueprint/UserWidget.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());
	DestinationMarker->SetVisibility(false);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(VRRoot);

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (BlinkerMaterialBase != nullptr) {
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);

		BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Enabled"), BlinkersEnabled ? 1 : 0);
	}

	LeftController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (LeftController != nullptr)
	{
		LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		LeftController->SetHand(EControllerHand::Left);
		LeftController->SetOwner(this);
	}

	RightController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (RightController != nullptr)
	{
		RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		RightController->SetHand(EControllerHand::Right);
		RightController->SetOwner(this);
	}

	if (RightController != nullptr && LeftController != nullptr) { RightController->PairController(LeftController); }

	/*ConfigMenu = GetWorld()->SpawnActor<AActor>(ConfigMenuClass);
	if (RightController != nullptr)
	{
		ConfigMenu->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		ConfigMenu->SetOwner(this);
	}*/
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCammeraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCammeraOffset.Z = 0;
	AddActorWorldOffset(NewCammeraOffset);
	VRRoot->AddWorldOffset(-NewCammeraOffset);

	if (TeleportEnabled) { UpdateDestinationMarker(); }
	if (BlinkersEnabled) { UpdateBlinkers(); }
}

void AVRCharacter::UpdateBlinkers()
{
	float Speed = GetVelocity().Size();
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);
	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

	FVector2D Center = GetBlinkerCenter();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0));
}

FVector2D AVRCharacter::GetBlinkerCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	if (MovementDirection.IsNearlyZero()) {
		return FVector2D(0.5, 0.5);
	}

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0) {
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else {
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr) {
		return FVector2D(0.5, 0.5);
	}

	FVector2D ScreenStationaryLocation;
	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	ScreenStationaryLocation.X /= SizeX;
	ScreenStationaryLocation.Y /= SizeY;

	return ScreenStationaryLocation;
}

bool AVRCharacter::FindTeleportDestination(TArray<FVector>& Path, FVector& OutLocation)
{
	FVector Start = RightController->GetActorLocation();
	FVector Look = RightController->GetActorForwardVector();

	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,
		Start, 
		Look * TeleportProjectileSpeed, 
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this
	);
	Params.bTraceComplex = true; // Evita algunos errores de detección de colisiones con meshes que no están demasiado bien
	//Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	FPredictProjectilePathResult Result;
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	if (!bHit) { return false; }

	for (FPredictProjectilePathPointData PointData : Result.PathData) {
		Path.Add(PointData.Location);
	}

	FNavLocation NavLocation;
	bool bOnNavMesh = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent);

	if (!bOnNavMesh) { return false; }

	OutLocation = NavLocation.Location;

	return true;
}

void AVRCharacter::UpdateDestinationMarker()
{
	TArray<FVector> Path;
	FVector Location;
	bool bDestinationFounded = FindTeleportDestination(Path, Location);

	DestinationMarker->SetVisibility(bDestinationFounded);
	if (bDestinationFounded) {
		DestinationMarker->SetWorldLocation(Location);
		UpdatePathMesh(Path);
	} else {
		TArray<FVector> EmptyPath;
		UpdatePathMesh(EmptyPath);
	}
}

void AVRCharacter::UpdatePathSpline(const TArray<FVector>& Path)
{
	TeleportPath->ClearSplinePoints(false);
	for (int i = 0; i < Path.Num(); i++) {
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}
	TeleportPath->UpdateSpline();
}

void AVRCharacter::UpdatePathMesh(const TArray<FVector>& Path)
{
	UpdatePathSpline(Path);

	for (USplineMeshComponent* SplineMesh : TeleportPathMeshPool) { SplineMesh->SetVisibility(false); }

	for (int i = 0; i < Path.Num(); i++) {
		if (TeleportPathMeshPool.Num() == i) {
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMesh->SetMobility(EComponentMobility::Movable);
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);
			SplineMesh->SetStaticMesh(TeleportArchMesh);
			SplineMesh->SetMaterial(0, TeleportArchMaterial);
			SplineMesh->RegisterComponent();

			TeleportPathMeshPool.Add(SplineMesh);
		}
		USplineMeshComponent* SplineMesh = TeleportPathMeshPool[i];
		SplineMesh->SetVisibility(true);

		FVector StartPos, StartTan, EndPos, EndTan;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTan);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i+1, EndPos, EndTan);
		SplineMesh->SetStartAndEnd(StartPos, StartTan, EndPos, EndTan);
	}
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);

	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);

	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Pressed, this, &AVRCharacter::GripLeft);
	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Released, this, &AVRCharacter::ReleaseLeft);

	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Pressed, this, &AVRCharacter::GripRight);
	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Released, this, &AVRCharacter::ReleaseRight);

	PlayerInputComponent->BindAction(TEXT("Menu"), IE_Pressed, this, &AVRCharacter::MenuButton);
}

void AVRCharacter::MoveForward(float throttle) 
{
	AddMovementInput(throttle * Camera->GetForwardVector() * this->GetWorld()->GetDeltaSeconds() * WalkingSpeed);
}

void AVRCharacter::MoveRight(float throttle) 
{
	AddMovementInput(throttle * Camera->GetRightVector() * this->GetWorld()->GetDeltaSeconds() * WalkingSpeed);
}

void AVRCharacter::BeginTeleport()
{
	if (!TeleportEnabled) { return; }

	StartTeleportFade(0, 1);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportTime/2);
}

void AVRCharacter::FinishTeleport()
{
	FVector Destination = DestinationMarker->GetComponentLocation();
	Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(Destination);

	StartTeleportFade(1, 0);
}

void AVRCharacter::StartTeleportFade(float FromAlpha, float ToAlpha)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr) {
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportTime / 2, FLinearColor::Black);
	}
}

void AVRCharacter::GripLeft() 
{ 
	LeftController->Grip(); 
}
void AVRCharacter::ReleaseLeft() 
{
	LeftController->Release(); 
}

void AVRCharacter::GripRight() 
{
	RightController->Grip();
}

void AVRCharacter::ReleaseRight() 
{
	RightController->Release();
}

void AVRCharacter::MenuButton()
{
	this->SwitchTeleportEnabled();
}

void AVRCharacter::SwitchTeleportEnabled() {
	TeleportEnabled = !TeleportEnabled;

	DestinationMarker->SetVisibility(TeleportEnabled);
	for (USplineMeshComponent* SplineMesh : TeleportPathMeshPool) { SplineMesh->SetVisibility(false); }
}

void AVRCharacter::SwitchBlinkersEnabled() {
	BlinkersEnabled = !BlinkersEnabled;

	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Enabled"), BlinkersEnabled ? 1 : 0); 
}
