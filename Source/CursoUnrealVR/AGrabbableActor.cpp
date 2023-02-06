// Fill out your copyright notice in the Description page of Project Settings.


#include "AGrabbableActor.h"

// Sets default values
AAGrabbableActor::AAGrabbableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject< UStaticMeshComponent>(TEXT("StaticMesh"));
	SetRootComponent(StaticMesh);
}

// Called when the game starts or when spawned
void AAGrabbableActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAGrabbableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

