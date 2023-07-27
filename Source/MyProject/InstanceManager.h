#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstanceManager.generated.h"

UENUM(BlueprintType)
enum class EInstancedType : uint8 {
  Trees UMETA(DisplayName = "Trees"),
  Minerals UMETA(DisplayName = "Minerals"),
  // Add more types here as needed
};

USTRUCT()
struct FInstancedMeshInfo {
  GENERATED_BODY()

  UPROPERTY()
  UInstancedStaticMeshComponent *MeshComponent;

  UPROPERTY()
  TArray<FTransform> Transforms;

  UPROPERTY()
  EInstancedType InstancedType;
};

UCLASS()
class MYPROJECT_API AInstanceManager : public AActor {
  GENERATED_BODY()

public:
  AInstanceManager();

  virtual void Tick(float DeltaTime) override;

  UFUNCTION(BlueprintCallable)
  void AddInstancedMeshToManager(UInstancedStaticMeshComponent *MeshComponent,
                                 const TArray<FTransform> &Transforms,
                                 EInstancedType InstancedType);

  UFUNCTION(BlueprintCallable)
  void SpawnInstancedMesh(UStaticMesh *Mesh, const FTransform &Transform,
                          EInstancedType InstancedType);

  UFUNCTION(BlueprintCallable)
  bool RemoveInstancedMesh(UStaticMesh *Mesh,
                           const FTransform &InstanceTransform);

  UFUNCTION(BlueprintCallable)
  FTransform
  GetTransformOfInstance(UInstancedStaticMeshComponent *MeshComponent,
                         int32 InstanceIndex);

protected:
  virtual void BeginPlay() override;

private:
  TMap<UInstancedStaticMeshComponent *, FInstancedMeshInfo> InstancedMeshInfos;

  TMap<UStaticMesh *, UInstancedStaticMeshComponent *> MeshToComponent;
};