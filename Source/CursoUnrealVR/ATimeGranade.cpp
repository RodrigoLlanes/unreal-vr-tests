// Fill out your copyright notice in the Description page of Project Settings.


#include "ATimeGranade.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"

AATimeGranade::AATimeGranade() : AAGrabbableActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ShieldMesh = CreateDefaultSubobject< UStaticMeshComponent>(TEXT("ShieldMesh"));
	ShieldMesh->AttachTo(GetRootComponent());
}

void AATimeGranade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float scale;

	ElapsedTime += DeltaTime;
	switch (State)
	{
	case 0:
		MGranade->SetScalarParameterValue(TEXT("FlickingTime"), ElapsedTime);
		break;
	case 1:
		if (ElapsedTime < GrowingTime) {
			scale = ElapsedTime / GrowingTime;
			ShieldMesh->SetWorldScale3D(TargetScale * scale);
			MShield->SetScalarParameterValue(TEXT("Opacity"), 1 - scale);
		}
		else {
			State = -1;
			ElapsedTime = 0;
			ShieldMesh->SetWorldScale3D(TargetScale);
		}
		break;
	case 2:
		if (ElapsedTime < GrowingTime) {
			scale = 1 - (ElapsedTime / GrowingTime);
			ShieldMesh->SetWorldScale3D(TargetScale * scale);
			MShield->SetScalarParameterValue(TEXT("Opacity"), 1 - scale);
		}
		else {
			State = -1;
			ElapsedTime = 0;
			ShieldMesh->SetWorldScale3D(FVector(0, 0, 0));
			ShieldMesh->SetVisibility(false);
			this->GetWorld()->GetWorldSettings()->SetTimeDilation(1);
			MGranade->SetScalarParameterValue(TEXT("Active"), 0);
		}
		break;
	default:
		ElapsedTime = 0;
		break;
	}
}

void AATimeGranade::Activate()
{
	MGranade->SetScalarParameterValue(TEXT("Active"), 1);
	ElapsedTime = 0;
	MGranade->SetScalarParameterValue(TEXT("FlickingTime"), ElapsedTime);
	State = 0;

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AATimeGranade::Start, Delay);
}

void AATimeGranade::BeginPlay()
{
	Super::BeginPlay();

	ShieldMesh->SetVisibility(false);
	TargetScale = ShieldMesh->GetComponentScale() * MaxScale;
	ShieldMesh->SetWorldScale3D(FVector(0, 0, 0));

	MShield = UMaterialInstanceDynamic::Create(ShieldMesh->GetMaterial(0), this);
	ShieldMesh->SetMaterial(0, MShield);

	MGranade = UMaterialInstanceDynamic::Create(StaticMesh->GetMaterial(0), this);
	StaticMesh->SetMaterial(0, MGranade);

	MGranade->SetScalarParameterValue(TEXT("MaxIntensity"), MaxIntensity);
	MGranade->SetScalarParameterValue(TEXT("MinIntensity"), MinIntensity);
	MGranade->SetScalarParameterValue(TEXT("MaxSpeed"), MaxFlickerSpeed);
	MGranade->SetScalarParameterValue(TEXT("MinSpeed"), MaxFlickerSpeed);
	MGranade->SetScalarParameterValue(TEXT("Delay"), Delay);
}

void AATimeGranade::Finish()
{
	State = 2;
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMesh->SetSimulatePhysics(true);
}

void AATimeGranade::BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
}

void AATimeGranade::EndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	OtherActor->CustomTimeDilation = TimeDilation;
	UE_LOG(LogTemp, Warning, TEXT("TimeDilationSetted %s 1"), *OtherActor->GetName());
}

void AATimeGranade::Start()
{
	StaticMesh->SetSimulatePhysics(false);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMesh->SetVisibility(true);

	MGranade->SetScalarParameterValue(TEXT("FlickingTime"), Delay + 1);
	State = 1;
	ElapsedTime = 0;

	this->GetWorld()->GetWorldSettings()->SetTimeDilation(TimeDilation);
	//this->GetWorld()->GetFirstPlayerController()->GetCharacter()->CustomTimeDilation = 1 / TimeDilation;
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AATimeGranade::Finish, Duration * TimeDilation);
}
