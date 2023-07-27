// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MiningActorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMeshHit,
                                             UInstancedStaticMeshComponent *,
                                             HitMesh, int32, InstanceIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHitResults,
                                            const TArray<FHitResult> &,
                                            HitResults);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UMiningActorComponent : public UActorComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UMiningActorComponent();

protected:
  // Called when the game starts
  virtual void BeginPlay() override;

public:
  // Called every frame
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  UFUNCTION(BlueprintCallable)
  void RayCast();

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
  TSubclassOf<AActor> ActorToSpawn;

  UFUNCTION(BlueprintCallable)
  TArray<FHitResult> LineTraceAndSphereTrace();

  UPROPERTY(BlueprintAssignable)
  FOnMeshHit OnMeshHit;

  UPROPERTY(BlueprintAssignable)
  FOnHitResults OnHitResults;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
  float SphereTraceRadius = 30.0f;
};
