// Fill out your copyright notice in the Description page of Project Settings.

#include "ChaosMiningActor.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Field/FieldSystemActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GeometryCollection/GeometryCollectionEngineRemoval.h"
#include "KTGeometryCollectionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MiningManager.h"
#include "PhysicsEngine/PhysicsSettings.h"

// Sets default values
AChaosMiningActor::AChaosMiningActor() {

  PrimaryActorTick.bCanEverTick = false;
  // Create a default SceneComponent and set it as the root
  DefaultSceneRoot =
      CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
  RootComponent = DefaultSceneRoot;

  // Initialize GeometryCollectionComponent
  MiningGeometryCollectionComponent =
      CreateDefaultSubobject<UKTGeometryCollectionComponent>(
          TEXT("MiningGeometryCollection"));

  // Attach the GeometryCollectionComponent to the root
  MiningGeometryCollectionComponent->SetupAttachment(RootComponent);

  UE_LOG(LogTemp, Warning, TEXT("ChaosMiningActor constructor called."));

  // Set bAllowRemovalOnSleep to true
  MiningGeometryCollectionComponent->bAllowRemovalOnSleep = true;

  TotalMassCrumbled = 0.f;
}

// Called when the game starts or when spawned
void AChaosMiningActor::BeginPlay() {
  Super::BeginPlay();

  UE_LOG(LogTemp, Warning, TEXT("ChaosMiningActor BeginPlay called."));

  MiningGeometryCollectionComponent->SetNotifyRemovals(true);

  // SpawnFieldSystemActor();

  // MiningGeometryCollectionComponent->SetRestCollection(MiningGeometryCollection);

  for (TActorIterator<AFieldSystemActor> ActorItr(GetWorld()); ActorItr;
       ++ActorItr) {
    AFieldSystemActor *FieldSystemActor = *ActorItr;

    if (FieldSystemActor) {
      UE_LOG(LogTemp, Warning, TEXT("Found FieldSystemActor: %s"),
             *FieldSystemActor->GetName());
      MiningGeometryCollectionComponent->InitializationFields.Add(
          FieldSystemActor);
    }
  }

  // MiningGeometryCollectionComponent->PublicResetDynamicCollection();

  MiningGeometryCollectionComponent->RecreatePhysicsState();

  MiningGeometryCollectionComponent->SetRenderStateDirty();

MiningGeometryCollectionComponent->OnChaosRemovalEvent.AddDynamic(
  this, &AChaosMiningActor::OnChaosRemoval);


  int32 GetTotalMassIndex = 0;

  MiningGeometryCollectionComponent->GetMassAndExtents(GetTotalMassIndex,
                                                       TotalMass, Extents);
}

// Called every frame
void AChaosMiningActor::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

void AChaosMiningActor::SetUpChaosMesh(
    UGeometryCollection *GeometryRestCollection) {

  UE_LOG(LogTemp, Warning, TEXT("Setting up geomrestCOlelction."));

  for (TActorIterator<AFieldSystemActor> ActorItr(GetWorld()); ActorItr;
       ++ActorItr) {
    AFieldSystemActor *FieldSystemActor = *ActorItr;

    if (FieldSystemActor) {
      UE_LOG(LogTemp, Warning, TEXT("Found FieldSystemActor: %s"),
             *FieldSystemActor->GetName());
      MiningGeometryCollectionComponent->InitializationFields.Add(
          FieldSystemActor);
    }
  }

  // MiningGeometryCollectionComponent->SetRestCollection(GeometryRestCollection);

  // MiningGeometryCollectionComponent->PublicResetDynamicCollection();

  MiningGeometryCollectionComponent->RecreatePhysicsState();
  // MiningGeometryCollectionComponent->SetRenderStateDirty();

  // // // Set the component to tick
  // MiningGeometryCollectionComponent->PrimaryComponentTick.bCanEverTick =
  // true; MiningGeometryCollectionComponent->bTickInEditor = true;
  // MiningGeometryCollectionComponent->bAutoActivate = true;
}

void AChaosMiningActor::SpawnFieldSystemActor() {
  FActorSpawnParameters SpawnParams;
  FVector Location = GetActorLocation();
  Location.Z -= 0; // Move 100 units south in the Z direction
  FRotator Rotation = FRotator::ZeroRotator; // No rotation

  // If no FieldSystemActor class is set, don't try to spawn the actor
  if (!FieldSystemActorClass) {
    UE_LOG(LogTemp, Warning, TEXT("FieldSystemActorClass not set."));
    return;
  }

  // Spawn the actor
  AnchorFieldActor = GetWorld()->SpawnActor<AFieldSystemActor>(
      FieldSystemActorClass, Location, Rotation, SpawnParams);
  if (AnchorFieldActor) {
    UE_LOG(LogTemp, Warning, TEXT("AnchorFieldActor spawned."));

    // The spawned actor should already be an AFieldSystemActor, but casting for
    // safety
    AFieldSystemActor *FieldSystemActor =
        Cast<AFieldSystemActor>(AnchorFieldActor);

    if (FieldSystemActor) {
      // Add the spawned actor to InitializationFields
      MiningGeometryCollectionComponent->InitializationFields.Add(
          FieldSystemActor);
    }
  } else {
    UE_LOG(LogTemp, Warning, TEXT("Failed to spawn AnchorFieldActor."));
  }
}

void AChaosMiningActor::OnChaosRemoval(const FChaosRemovalEvent &RemovalEvent) {
  // Assume RemovalEvent contains a "Mass" property
  TotalMassCrumbled += RemovalEvent.Mass;

  float DestroyPercentage = 0.9f;

  if ((TotalMassCrumbled / TotalMass) >= DestroyPercentage) {
    // Remove active clusters
    MiningGeometryCollectionComponent->CrumbleActiveClusters();

    // Set a timer to destroy the actor after a delay
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle, this, &AChaosMiningActor::DelayedDestroy, 3.0f, false);
  }
}


void AChaosMiningActor::DelayedDestroy() {

  TArray<AActor *> FoundActors;
  UGameplayStatics::GetAllActorsOfClass(
      GetWorld(), AMiningManager::StaticClass(), FoundActors);

  if (FoundActors.Num() > 0) {
    AMiningManager *MiningManager = Cast<AMiningManager>(FoundActors[0]);
    if (MiningManager) {
      MiningManager->SpawnRandomizedMineralSpot(ActorLocation);
    }
  }
  // Destroy the actor
  Destroy();
}
