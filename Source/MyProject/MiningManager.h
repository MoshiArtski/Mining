// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

#include "ChaosMiningActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "MiningManager.generated.h"

class AMiningActor;
class AFieldSystemActor;

UENUM(BlueprintType)
enum class EMineralType : uint8 {
  Gold UMETA(DisplayName = "Gold"),
  Silver UMETA(DisplayName = "Silver"),
  // Add more types here as needed
};

USTRUCT()
struct FMineralSpots : public FTableRowBase {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FTransform MineralSpotTransform;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TMap<EMineralType, float> MineralTypePercentage;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 MineralSpotSeed;
};

USTRUCT(BlueprintType)
struct FMineralTypeData : public FTableRowBase {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  EMineralType MineralType;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  UStaticMesh *MineralMesh;
};

USTRUCT(BlueprintType)
struct FStaticMeshGroupData : public FTableRowBase {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TObjectPtr<UStaticMesh> KeyMesh;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TArray<TObjectPtr<UStaticMesh>> GroupMeshes;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TSubclassOf<AChaosMiningActor> MiningGeometryCollection;
};

UCLASS()
class MYPROJECT_API AMiningManager : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  AMiningManager();

  UPROPERTY(EditAnywhere, Category = "Mineral")
  TObjectPtr<UDataTable> MineralSpotsData;

  UPROPERTY(EditAnywhere, Category = "Mineral")
  TObjectPtr<UDataTable> MineralMeshData;

  UPROPERTY(EditAnywhere, Category = "MeshGroupData")
  TObjectPtr<UDataTable> MeshGroupData;

  UPROPERTY(EditAnywhere, Category = "Mineral")
  TArray<FMineralSpots> MineralSpots;

  UPROPERTY(EditAnywhere, Category = "Mineral")
  bool bUseChaos = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mineral")
  TSubclassOf<AFieldSystemActor> AnchorFieldActor;

  UFUNCTION(BlueprintCallable, Category = "Spawn")
  void SpawnGroupMeshes(UStaticMesh *KeyMesh, const FTransform &SpawnTransform);

  UFUNCTION()
  void ConvertMineralInstance(UInstancedStaticMeshComponent *MeshComponent,
                              int32 InstanceIndex);

  UFUNCTION()
  void SpawnRandomizedMineralSpot(const FTransform &NewSpotTransform);

  UPROPERTY(EditAnywhere, Category = "Spawning")
  TSubclassOf<AChaosMiningActor> ChaosMiningActorClass;

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;

  UFUNCTION(BlueprintCallable)
  void SetMineralTypesAll();

private:
  UFUNCTION() EMineralType PickMineralType(const FMineralSpots &MineralSpot);

  UFUNCTION()
  UStaticMesh *GetMineralStaticMesh(EMineralType MineralType);

  UFUNCTION()
  FString MineralTypeToString(EMineralType MineralType);

  UFUNCTION() void FillDestructableMeshMap();

  TMap<TObjectPtr<UStaticMesh>, TArray<TObjectPtr<UStaticMesh>>>
      DestructableMeshMap;

  TMap<TObjectPtr<UStaticMesh>, TSubclassOf<AChaosMiningActor>>
      DestructableChaosMeshMap;

  TMap<FString, TArray<AMiningActor *>> SpawnedActorGroups;

  TMap<FString, TArray<AChaosMiningActor *>> SpawnedChaosActorGroups;

  void SpawnChaos(UStaticMesh *KeyMesh, const FTransform &SpawnTransform);
};
