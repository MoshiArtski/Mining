// Fill out your copyright notice in the Description page of Project Settings.

#include "MiningManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMeshActor.h"
#include "Field/FieldSystemActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "InstanceManager.h"
#include "Kismet/GameplayStatics.h"
#include "MiningActor.h"
#include "MiningActorComponent.h"

// Sets default values
AMiningManager::AMiningManager() {
  // Set this actor to call Tick() every frame.  You can turn this off to
  // improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMiningManager::BeginPlay() {
  Super::BeginPlay();

  if (MineralSpotsData) {
    // Get the row names of the data table
    TArray<FName> RowNames = MineralSpotsData->GetRowNames();

    // Iterate over each row
    for (auto &RowName : RowNames) {
      // Retrieve each row as FMineralSpots
      FMineralSpots *MineralSpot =
          MineralSpotsData->FindRow<FMineralSpots>(RowName, "");

      // Check if the MineralSpot pointer is not null
      if (MineralSpot) {
        // Add the retrieved FMineralSpots data to the MineralSpots array
        MineralSpots.Add(*MineralSpot);
      }
    }
  } else {
    UE_LOG(LogTemp, Error, TEXT("MineralSpotsData not found."));
  }

  SetMineralTypesAll();
  FillDestructableMeshMap();

  APlayerController *PlayerController =
      UGameplayStatics::GetPlayerController(this, 0);
  if (PlayerController) {
    APawn *ControlledPawn = PlayerController->GetPawn();
    if (ControlledPawn) {
      UMiningActorComponent *MiningActorComponent =
          ControlledPawn->FindComponentByClass<UMiningActorComponent>();
      if (MiningActorComponent) {
        MiningActorComponent->OnMeshHit.AddDynamic(
            this, &AMiningManager::ConvertMineralInstance);
      } else {
        UE_LOG(LogTemp, Error,
               TEXT("MiningActorComponent not found on player pawn."));
      }
    } else {
      UE_LOG(LogTemp, Error, TEXT("Player controller does not have a pawn."));
    }
  } else {
    UE_LOG(LogTemp, Error, TEXT("Player controller not found."));
  }
}

// Called every frame
void AMiningManager::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

EMineralType AMiningManager::PickMineralType(const FMineralSpots &MineralSpot) {
  // Hash the seed to get a new seed
  uint32 HashedSeed = GetTypeHash(MineralSpot.MineralSpotSeed);

  FDateTime CurrentDateTime = FDateTime::UtcNow();
  uint32 TimeSeed = CurrentDateTime.ToUnixTimestamp();

  FRandomStream RandomStream(HashedSeed + TimeSeed);

  const float RandomValue = RandomStream.FRandRange(0.f, 1.f);
  float CurrentPercentage = 0.f;

  // Iterate over the mineral type percentages
  for (const auto &MineralTypePercentagePair :
       MineralSpot.MineralTypePercentage) {
    EMineralType MineralType = MineralTypePercentagePair.Key;
    float Percentage = MineralTypePercentagePair.Value;

    CurrentPercentage += Percentage;

    // Check if the random value is within the current percentage range
    if (RandomValue <= CurrentPercentage) {
      return MineralType;
    }
  }

  // Return a default mineral type if no type is selected
  return EMineralType::Gold; // Change this to your desired default mineral type
}

void AMiningManager::SetMineralTypesAll() {
  // Get all instance manager actors once
  TArray<AActor *> FoundActors;
  UGameplayStatics::GetAllActorsOfClass(
      GetWorld(), AInstanceManager::StaticClass(), FoundActors);

  for (const FMineralSpots &MineralSpot : MineralSpots) {
    EMineralType SelectedMineralType = PickMineralType(MineralSpot);

    for (AActor *Actor : FoundActors) {
      AInstanceManager *InstanceManager = Cast<AInstanceManager>(Actor);
      if (InstanceManager) {
        InstanceManager->SpawnInstancedMesh(
            GetMineralStaticMesh(SelectedMineralType),
            MineralSpot.MineralSpotTransform, EInstancedType::Minerals);

        // Get the up vector of the mineral spot transform
        FVector UpVector =
            MineralSpot.MineralSpotTransform.GetUnitAxis(EAxis::Z);

        // Create an offset along the up vector
        FVector Offset = 7 * UpVector;

        // Add the offset to the mineral spot transform
        FTransform OffsetTransform = MineralSpot.MineralSpotTransform;
        OffsetTransform.AddToTranslation(Offset);

        // Spawn the anchor field actor at the offset transform
        if (AnchorFieldActor) {
          GetWorld()->SpawnActor<AFieldSystemActor>(AnchorFieldActor,
                                                    OffsetTransform);
        }
      }
    }
  }
}

UStaticMesh *AMiningManager::GetMineralStaticMesh(EMineralType MineralType) {
  const FString ContextString(TEXT("GetMineralStaticMesh"));
  const FMineralTypeData *MineralMeshDataRow =
      MineralMeshData->FindRow<FMineralTypeData>(
          FName(*MineralTypeToString(MineralType)), ContextString);

  if (MineralMeshDataRow) {
    return MineralMeshDataRow->MineralMesh;
  }

  return nullptr;
}

FString AMiningManager::MineralTypeToString(EMineralType MineralType) {
  switch (MineralType) {
  case EMineralType::Gold:
    return "Gold";
  case EMineralType::Silver:
    return "Silver";
  // Add more cases as needed
  default:
    return "Unknown";
  }
}

void AMiningManager::FillDestructableMeshMap() {
  if (MeshGroupData) {
    const FString ContextString(TEXT("DestructableMeshMap Parsing"));
    TArray<FName> RowNames = MeshGroupData->GetRowNames();

    for (auto &Name : RowNames) {
      FStaticMeshGroupData *Row =
          MeshGroupData->FindRow<FStaticMeshGroupData>(Name, ContextString);
      if (Row) {
        if (bUseChaos) {
          DestructableChaosMeshMap.Add(Row->KeyMesh,
                                       Row->MiningGeometryCollection);
          UE_LOG(LogTemp, Warning,
                 TEXT("Adding KeyMesh: %s to DestructableChaosMeshMap"),
                 *Row->KeyMesh->GetName());
        } else {
          DestructableMeshMap.Add(Row->KeyMesh, Row->GroupMeshes);
          UE_LOG(LogTemp, Warning,
                 TEXT("Adding KeyMesh: %s to DestructableMeshMap"),
                 *Row->KeyMesh->GetName());
        }
      } else {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find row for Name: %s"),
               *Name.ToString());
      }
    }
  } else {
    UE_LOG(LogTemp, Warning, TEXT("MeshGroupData is null."));
  }
}

void AMiningManager::SpawnGroupMeshes(UStaticMesh *KeyMesh,
                                      const FTransform &SpawnTransform) {
  TArray<TObjectPtr<UStaticMesh>> *MeshGroup =
      DestructableMeshMap.Find(KeyMesh);
  if (MeshGroup) {
    // Generate a unique key for this group (could also be an int, UUID, etc.)
    FString GroupKey = FGuid::NewGuid().ToString();

    UWorld *World = GetWorld();
    if (World) {
      FActorSpawnParameters SpawnParams;
      AMiningActor *NewActor = World->SpawnActor<AMiningActor>(
          AMiningActor::StaticClass(), SpawnTransform, SpawnParams);
      if (NewActor) {
        // Here you set ActorLocation for the newly spawned actor
        NewActor->SetActorLocation(SpawnTransform);

        for (TObjectPtr<UStaticMesh> MeshObjectPtr : *MeshGroup) {
          UStaticMesh *Mesh = MeshObjectPtr.Get();
          NewActor->MeshesToSpawn.Add(Mesh);
        }

        NewActor->SpawnMeshes();

        // Add the newly spawned actor to the current group
        SpawnedActorGroups.FindOrAdd(GroupKey).Add(NewActor);
      }
    }
  }
}

void AMiningManager::SpawnChaos(UStaticMesh *KeyMesh,
                                const FTransform &SpawnTransform) {
  TSubclassOf<AChaosMiningActor> *ChaosActorClassPtr =
      DestructableChaosMeshMap.Find(KeyMesh);

  if (ChaosActorClassPtr) {
    TSubclassOf<AChaosMiningActor> ChaosActorClass = *ChaosActorClassPtr;
    FString GroupKey = FGuid::NewGuid().ToString(); // Unique key

    UWorld *World = GetWorld();

    if (World) {
      FActorSpawnParameters SpawnParams;

      if (!ChaosActorClass) {
        return;
      }

      AChaosMiningActor *NewActor = World->SpawnActor<AChaosMiningActor>(
          ChaosActorClass, SpawnTransform, SpawnParams);

      if (NewActor) {
        NewActor->SetActorLocation(SpawnTransform);

        // NewActor->SetFieldSystemActorClass(AnchorFieldActor);

        SpawnedChaosActorGroups.FindOrAdd(GroupKey).Add(NewActor);
      }
    }
  }
}

void AMiningManager::ConvertMineralInstance(
    UInstancedStaticMeshComponent *MeshComponent, int32 InstanceIndex) {

  if (MeshComponent && MeshComponent->GetInstanceCount() > InstanceIndex) {
    FTransform InstanceTransform;
    MeshComponent->GetInstanceTransform(InstanceIndex, InstanceTransform, true);

    TArray<AActor *> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(), AInstanceManager::StaticClass(), FoundActors);

    for (AActor *Actor : FoundActors) {
      AInstanceManager *InstanceManager = Cast<AInstanceManager>(Actor);
      if (InstanceManager) {
        if (!bUseChaos) {
          SpawnGroupMeshes(MeshComponent->GetStaticMesh(), InstanceTransform);
        } else {
          SpawnChaos(MeshComponent->GetStaticMesh(), InstanceTransform);
        }

        bool bRemoved = InstanceManager->RemoveInstancedMesh(
            MeshComponent->GetStaticMesh(), InstanceTransform);

        if (bRemoved) {
        }
      }
    }
  }
}

void AMiningManager::SpawnRandomizedMineralSpot(
    const FTransform &NewSpotTransform) {
  // Define a timer handle

  UE_LOG(LogTemp, Warning, TEXT("SpawnRandomizedMineralSpot called"));

  FTimerHandle TimerHandle;

  // Set a timer that will call the function after 10 seconds
  GetWorld()->GetTimerManager().SetTimer(
      TimerHandle,
      [this, NewSpotTransform]() {
        FMineralSpots NewSpot;

        // Assuming we can identify the mineral spot from its transform
        for (const FMineralSpots &MineralSpot : MineralSpots) {
          if (MineralSpot.MineralSpotTransform.Equals(NewSpotTransform)) {
            NewSpot = MineralSpot;
            break;
          }
        }

        // Randomize the spot
        NewSpot.MineralSpotSeed = FMath::Rand();

        // Pick a new mineral type
        EMineralType NewMineralType = PickMineralType(NewSpot);

        // Find all InstanceManager actors in the scene
        TArray<AActor *> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(
            GetWorld(), AInstanceManager::StaticClass(), FoundActors);

        for (AActor *Actor : FoundActors) {
          AInstanceManager *InstanceManager = Cast<AInstanceManager>(Actor);
          if (InstanceManager) {
            // Spawn the instanced mesh at the new mineral spot
            InstanceManager->SpawnInstancedMesh(
                GetMineralStaticMesh(NewMineralType),
                NewSpot.MineralSpotTransform, EInstancedType::Minerals);
          }
        }

        // Add the new spot to the MineralSpots array
        MineralSpots.Add(NewSpot);
      },
      10.0f, // Delay in seconds
      false  // Don't repeat the timer
  );
}
