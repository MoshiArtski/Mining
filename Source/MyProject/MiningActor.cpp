// Fill out your copyright notice in the Description page of Project Settings.

#include "MiningActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MiningActorComponent.h"
#include "MiningManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values
AMiningActor::AMiningActor() {
  // Set this actor to call Tick() every frame.  You can turn this off to
  // improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMiningActor::BeginPlay() {
  Super::BeginPlay();

  SpawnMeshes();

  APlayerController *PlayerController =
      UGameplayStatics::GetPlayerController(this, 0);
  if (!PlayerController) {
    UE_LOG(LogTemp, Error, TEXT("Player controller not found."));
    return;
  }

  APawn *ControlledPawn = PlayerController->GetPawn();
  if (!ControlledPawn) {
    UE_LOG(LogTemp, Error, TEXT("Player controller does not have a pawn."));
    return;
  }

  UMiningActorComponent *MiningActorComponent =
      ControlledPawn->FindComponentByClass<UMiningActorComponent>();
  if (!MiningActorComponent) {
    UE_LOG(LogTemp, Error,
           TEXT("MiningActorComponent not found on player pawn."));
    return;
  }

  MiningActorComponent->OnHitResults.AddDynamic(
      this, &AMiningActor::EnablePhysicsOnHits);
}

// Called every frame
void AMiningActor::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

void AMiningActor::SpawnMeshes() {
  for (UStaticMesh *Mesh : MeshesToSpawn) {
    if (Mesh) {
      UStaticMeshComponent *StaticMeshComponent =
          NewObject<UStaticMeshComponent>(this,
                                          UStaticMeshComponent::StaticClass());
      StaticMeshComponent->SetStaticMesh(Mesh);
      StaticMeshComponent->SetSimulatePhysics(false);
      StaticMeshComponent->SetMassOverrideInKg(NAME_None, 10000.0f, true);
      StaticMeshComponent->RegisterComponent();
      StaticMeshComponent->AttachToComponent(
          GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
      SpawnedMeshes.Add(StaticMeshComponent);
      SpawnedMeshesSet.Add(StaticMeshComponent);
    }
  }

  if (SpawnedMeshes.Num() == 0)
    return;

  TotalMeshes = SpawnedMeshes.Num();

  for (UStaticMeshComponent *MeshComponent : SpawnedMeshes) {
    if (MeshComponent) {
      // Accumulate the positions of all meshes
      Centroid += MeshComponent->GetComponentLocation();
    }
  }

  // Calculate the average position (centroid)
  Centroid /= SpawnedMeshes.Num();
}

void AMiningActor::ApplyImpulseToAllMeshes() {

  for (UStaticMeshComponent *MeshComponent : SpawnedMeshes) {
    if (MeshComponent) {
      // Add a radial impulse at the centroid
      MeshComponent->AddRadialImpulse(
          Centroid,
          ImpulseRadius, // Radius of the impulse
          ImpulseStrength,
          ERadialImpulseFalloff::RIF_Constant, // No falloff
          true                                 // Velocity change
      );
    }
  }
}

void AMiningActor::EnablePhysicsOfMesh(int32 MeshIndex) {
  if (MeshIndex >= 0 && MeshIndex < SpawnedMeshes.Num()) {
    UStaticMeshComponent *MeshComponent = SpawnedMeshes[MeshIndex];
    if (MeshComponent) {
      // Enable physics
      MeshComponent->SetSimulatePhysics(true);
    }
  }
}

void AMiningActor::EnablePhysicsOnHits(const TArray<FHitResult> &HitResults) {
  for (const FHitResult &HitResult : HitResults) {
    // Get the static mesh component
    UStaticMeshComponent *HitMesh =
        Cast<UStaticMeshComponent>(HitResult.Component.Get());

    if (HitMesh && SpawnedMeshesSet.Contains(HitMesh)) {
      // Enable physics on the hit mesh
      HitMesh->SetSimulatePhysics(true);
      PhysicsEnabledMeshes++;

      // Apply an impulse at the hit location in the opposite direction of the
      // impact normal
      HitMesh->AddImpulseAtLocation(HitResult.ImpactNormal * ImpactForce,
                                    HitResult.ImpactPoint);
      ApplyImpulseToAllMeshes();
      CheckAndEnablePhysicsForAll();

      // Start a timer to gradually scale down the mesh
      FTimerHandle ScaleTimerHandle;
      TWeakObjectPtr<AMiningActor> WeakSelf(this);
      WeakSelf->GetWorld()->GetTimerManager().SetTimer(
          ScaleTimerHandle,
          FTimerDelegate::CreateLambda([WeakSelf, HitMesh,
                                        &ScaleTimerHandle]() {
            if (!WeakSelf.IsValid() || !HitMesh ||
                !HitMesh->IsValidLowLevel()) {
              return;
            }
            AMiningActor *Self = WeakSelf.Get();
            if (HitMesh) {
              FVector Scale = HitMesh->GetComponentScale();
              Scale *= 0.92f; // Decrease the scale by 1%
              HitMesh->SetWorldScale3D(Scale);

              // If the scale is small enough, stop the timer and remove the
              // mesh
              if (Scale.GetMin() < 0.01f) {
                Self->GetWorld()->GetTimerManager().ClearTimer(
                    ScaleTimerHandle);
                HitMesh->SetVisibility(false);
                HitMesh->SetSimulatePhysics(false);
                HitMesh->Deactivate();
                HitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                HitMesh->UnregisterComponent();
                HitMesh->DestroyComponent();
                FVector MeshLocation = HitMesh->GetComponentLocation();
                Self->SpawnEmitterAndSound(MeshLocation);
              }
            }
          }),
          0.1f, // Time period
          true  // Repeat
      );

    } else {
      // Apply radial damage at the impact location
      UGameplayStatics::ApplyRadialDamage(
          this,                       // WorldContextObject
          3000.0f,                    // BaseDamage
          HitResult.ImpactPoint,      // DamageOrigin
          500.0f,                     // DamageRadius
          UDamageType::StaticClass(), // DamageTypeClass
          TArray<AActor *>(),         // IgnoreActors
          this,                       // DamageCauser
          nullptr,                    // InstigatedByController
          true                        // bDoFullDamage
      );
    }
  }
}

void AMiningActor::CheckAndEnablePhysicsForAll() {
  if (TotalMeshes > 0 && ((float)PhysicsEnabledMeshes / TotalMeshes) >= 0.9f) {
    EnablePhysicsOfAllMesh();
  }
}

void AMiningActor::RemoveMesh(int32 MeshIndex) {
  if (MeshIndex >= 0 && MeshIndex < SpawnedMeshes.Num()) {
    UStaticMeshComponent *MeshComponent = SpawnedMeshes[MeshIndex];
    if (MeshComponent) {
      // Unregister and destroy the mesh component
      MeshComponent->UnregisterComponent();
      MeshComponent->DestroyComponent();

      // Remove the mesh from the SpawnedMeshes array
      SpawnedMeshes.RemoveAt(MeshIndex);
    }
  }
}

void AMiningActor::RemovePhysicsEnabledMeshes() {
  for (int32 i = SpawnedMeshes.Num() - 1; i >= 0; i--) {
    UStaticMeshComponent *MeshComponent = SpawnedMeshes[i];

    // Check if the component is valid and is simulating physics
    if (MeshComponent && MeshComponent->IsSimulatingPhysics()) {

      // Get the location of the mesh to be removed
      FVector MeshLocation = MeshComponent->GetComponentLocation();

      // Scale down and remove the mesh
      ScaleDownAndRemoveMesh(MeshComponent, MeshLocation);
    }
  }
}

void AMiningActor::IncrementDissolveAmount() {
  // Increment the dissolve amount
  CurrentDissolveAmount += GetWorld()->GetDeltaSeconds();

  // Clamp the dissolve amount between 0 and 1
  CurrentDissolveAmount = FMath::Clamp(CurrentDissolveAmount, 0.0f, 1.0f);

  // Update the dissolve amount in the material parameter collection
  UMaterialParameterCollectionInstance *DissolveParametersInstance =
      GetWorld()->GetParameterCollectionInstance(DissolveParameters);
  if (DissolveParametersInstance) {
    DissolveParametersInstance->SetScalarParameterValue(TEXT("DissolveAmount"),
                                                        CurrentDissolveAmount);
  }

  // If the dissolve effect is complete, stop the timer
  if (CurrentDissolveAmount >= 1.0f) {
    GetWorld()->GetTimerManager().ClearTimer(DissolveTimerHandle);
  }
}

void AMiningActor::MiningActorDepleted() {

  UE_LOG(LogTemp, Warning, TEXT("AMiningActor::MiningActorDepleted called"));

  if (TotalMeshes > 0 && ((float)PhysicsEnabledMeshes / TotalMeshes) >= 0.9f) {
    TArray<AActor *> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(), AMiningManager::StaticClass(), FoundActors);

    if (FoundActors.Num() > 0) {
      AMiningManager *MiningManager = Cast<AMiningManager>(FoundActors[0]);
      if (MiningManager) {
        MiningManager->SpawnRandomizedMineralSpot(ActorLocation);
      }
    }

    Destroy();
  }
}

void AMiningActor::EnablePhysicsOfAllMesh() {
  for (UStaticMeshComponent *MeshComponent : SpawnedMeshes) {
    if (MeshComponent) {
      // Enable physics
      MeshComponent->SetSimulatePhysics(true);
    }
  }

  RemovePhysicsEnabledMeshes();
  ApplyImpulseToAllMeshes();

  // Define a timer handle
  FTimerHandle TimerHandle;

  // Set the timer to call MiningActorDepleted after 10 seconds
  GetWorld()->GetTimerManager().SetTimer(
      TimerHandle, this, &AMiningActor::MiningActorDepleted, 10.0f, false);
}

void AMiningActor::SpawnEmitterAndSound(FVector SpawnLocation) {

  if (EmitterToSpawn && DissapearSoundEffect) {
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(), EmitterToSpawn, SpawnLocation,
        FRotator::ZeroRotator,  // Default rotation
        FVector(1.f, 1.f, 1.f), // Default scale
        true                    // Auto destroy
    );

    UGameplayStatics::PlaySoundAtLocation(this, DissapearSoundEffect,
                                          SpawnLocation);
  }
}

void AMiningActor::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  Super::EndPlay(EndPlayReason);

  // Ensure that the player's pawn and MiningActorComponent exist
  APlayerController *PlayerController =
      UGameplayStatics::GetPlayerController(this, 0);
  if (PlayerController) {
    APawn *ControlledPawn = PlayerController->GetPawn();
    if (ControlledPawn) {
      UMiningActorComponent *MiningActorComponent =
          ControlledPawn->FindComponentByClass<UMiningActorComponent>();
      if (MiningActorComponent) {
        // Unbind the delegate
        MiningActorComponent->OnHitResults.RemoveDynamic(
            this, &AMiningActor::EnablePhysicsOnHits);
      }
    }
  }
}

void AMiningActor::ScaleDownAndRemoveMesh(UStaticMeshComponent *MeshComponent,
                                          FVector MeshLocation) {
  FTimerHandle ScaleTimerHandle;
  TWeakObjectPtr<AMiningActor> WeakSelf(this);

  // Set up a repeating timer that fires every 0.1 seconds
  WeakSelf->GetWorldTimerManager().SetTimer(
      ScaleTimerHandle,
      FTimerDelegate::CreateLambda(
          [WeakSelf, MeshComponent, MeshLocation,
           &ScaleTimerHandle]() { // capture ScaleTimerHandle by reference
            if (!WeakSelf.IsValid()) {
              return;
            }
            AMiningActor *Self = WeakSelf.Get();
            if (MeshComponent) {
              // Gradually decrease the scale of the mesh
              FVector Scale = MeshComponent->GetComponentScale();
              Scale *= 0.92f; // Decrease the scale by 10%
              MeshComponent->SetWorldScale3D(Scale);

              // If the scale is small enough, stop the timer and remove the
              // mesh
              if (Scale.GetMin() < 0.01f) {
                Self->GetWorldTimerManager().ClearTimer(ScaleTimerHandle);
                MeshComponent->SetVisibility(false);
                MeshComponent->SetSimulatePhysics(false);
                MeshComponent->Deactivate();
                MeshComponent->SetCollisionEnabled(
                    ECollisionEnabled::NoCollision);
                MeshComponent->UnregisterComponent();
                MeshComponent->DestroyComponent();
                Self->SpawnEmitterAndSound(MeshLocation);
              }
            }
          }),
      0.1f, // Time period
      true  // Repeat
  );
}
