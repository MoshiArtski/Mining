#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "ChaosMiningActor.generated.h"



class UGeometryCollectionComponent;
class UKTGeometryCollectionComponent;
class AFieldSystemActor;

UCLASS()
class MYPROJECT_API AChaosMiningActor : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  AChaosMiningActor();

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;

  // Getter and Setter for GeometryCollectionComponent
  UGeometryCollection *GetGeometryCollectionComponent() const {
    return MiningGeometryCollection;
  }
  void SetGeometryCollectionComponent(
      UGeometryCollection *NewGeometryCollectionComponent) {
    UE_LOG(LogTemp, Warning, TEXT("Rest collection Added."));
    MiningGeometryCollection = NewGeometryCollectionComponent;
  }

  TSubclassOf<AFieldSystemActor> GetFieldSystemActorClass() const {
    return FieldSystemActorClass;
  }

  void SetFieldSystemActorClass(
      TSubclassOf<AFieldSystemActor> NewFieldSystemActorClass) {
    UE_LOG(LogTemp, Warning, TEXT("Field System Actor Class Added."));
    FieldSystemActorClass = NewFieldSystemActorClass;
  }

  UPROPERTY(EditAnywhere)
  TObjectPtr<UGeometryCollection> MiningGeometryCollection;

  UPROPERTY(EditAnywhere)
  TSubclassOf<AFieldSystemActor> FieldSystemActorClass;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  UKTGeometryCollectionComponent *MiningGeometryCollectionComponent;

  UFUNCTION()
  void OnChaosRemoval(const FChaosRemovalEvent &RemovalEvent);

  FTransform GetLocation() const { return ActorLocation; }

  void SetActorLocation(const FTransform &NewLocation) {
    ActorLocation = NewLocation;
  }

  UFUNCTION(BlueprintCallable)
  void SetUpChaosMesh(UGeometryCollection *GeometryRestCollection);
  UPROPERTY(BlueprintReadWrite)
  float TotalMassCrumbled;

  UPROPERTY(BlueprintReadWrite)
  float TotalMass;

private:
  UPROPERTY()
  TObjectPtr<USceneComponent> DefaultSceneRoot;

  void SpawnFieldSystemActor();

  AFieldSystemActor *AnchorFieldActor;

  FBox Extents;

  void DelayedDestroy();

  FTransform ActorLocation;
};
