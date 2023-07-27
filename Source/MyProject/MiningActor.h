// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MiningActor.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;

UCLASS() class MYPROJECT_API AMiningActor : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  AMiningActor();

  virtual void Tick(float DeltaTime) override;

  UPROPERTY(EditAnywhere, Category = "Mining")
  TArray<TObjectPtr<UStaticMesh>> MeshesToSpawn;

  UPROPERTY(EditAnywhere, Category = "Emitter")
  TObjectPtr<UNiagaraSystem> EmitterToSpawn;

  UPROPERTY(EditAnywhere, Category = "Build Mode")
  TObjectPtr<USoundBase> DissapearSoundEffect;

  UFUNCTION(BlueprintCallable, Category = "Mining")
  void ApplyImpulseToAllMeshes();

  UPROPERTY(EditAnywhere, Category = "Mining")
  float ImpulseStrength = 300.0f;

  UPROPERTY(EditAnywhere, Category = "Mining")
  float ImpulseRadius = 200.0f;

  UFUNCTION(BlueprintCallable, Category = "Mining")
  void EnablePhysicsOfMesh(int32 MeshIndex);

  UFUNCTION(BlueprintCallable, Category = "Mining")
  void EnablePhysicsOfAllMesh();

  UPROPERTY(EditAnywhere, Category = "Materials")
  UMaterialInterface *DissolveMaterial;

  // Material Parameter Collection for the dissolve effect
  UPROPERTY(EditAnywhere, Category = "Materials")
  UMaterialParameterCollection *DissolveParameters;

  void RemovePhysicsEnabledMeshes();

  void SpawnMeshes();

  void EnablePhysicsOnHit(const FHitResult &HitResult);

  UFUNCTION(BlueprintCallable)
  void RemoveMesh(int32 MeshIndex);

  UPROPERTY(EditAnywhere, Category = "Materials")
  float ImpactForce = 1000.0f;

  void IncrementDissolveAmount();

  FTransform GetActorLocation() const { return ActorLocation; }

  void SetActorLocation(const FTransform &NewLocation) {
    ActorLocation = NewLocation;
  }

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

private:
  TArray<TObjectPtr<UStaticMeshComponent>> SpawnedMeshes;

  UFUNCTION()
  void EnablePhysicsOnHits(const TArray<FHitResult> &HitResults);

  UFUNCTION()
  void CheckAndEnablePhysicsForAll();

  // Current dissolve amount
  float CurrentDissolveAmount;

  // Timer handle for the dissolve effect
  FTimerHandle DissolveTimerHandle;

  FVector Centroid = FVector::ZeroVector;

  FTransform ActorLocation;

  int32 TotalMeshes;

  int32 PhysicsEnabledMeshes = 0;

  void MiningActorDepleted();

  bool bDepleted = false;

  TSet<UStaticMeshComponent *> SpawnedMeshesSet;

  TArray<UStaticMeshComponent *> PhysicsEnabledMeshesStack;

  void SpawnEmitterAndSound(FVector SpawnLocation);

  void EndPlay(const EEndPlayReason::Type EndPlayReason);

  void ScaleDownAndRemoveMesh(UStaticMeshComponent *MeshComponent,
                              FVector MeshLocation);
};
