#include "InstanceManager.h"
#include "Components/InstancedStaticMeshComponent.h"

AInstanceManager::AInstanceManager() { PrimaryActorTick.bCanEverTick = false; }

void AInstanceManager::BeginPlay() { Super::BeginPlay(); }

void AInstanceManager::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

void AInstanceManager::AddInstancedMeshToManager(
    UInstancedStaticMeshComponent *MeshComponent,
    const TArray<FTransform> &Transforms, EInstancedType InstancedType) {
  FInstancedMeshInfo &MeshInfo = InstancedMeshInfos.FindOrAdd(MeshComponent);
  MeshInfo.Transforms.Append(Transforms);
  MeshInfo.InstancedType = InstancedType;
}

void AInstanceManager::SpawnInstancedMesh(UStaticMesh *Mesh,
                                          const FTransform &Transform,
                                          EInstancedType InstancedType) {
  UInstancedStaticMeshComponent *MeshComponent = nullptr;

  // If there's a UInstancedStaticMeshComponent for this UStaticMesh, use it
  if (MeshToComponent.Contains(Mesh)) {
    MeshComponent = MeshToComponent[Mesh];
  }

  // If there isn't a UInstancedStaticMeshComponent for this UStaticMesh yet,
  // create one
  if (!MeshComponent) {
    MeshComponent = NewObject<UInstancedStaticMeshComponent>(this);
    MeshComponent->SetStaticMesh(Mesh);
    MeshComponent->RegisterComponent();
    MeshToComponent.Add(Mesh, MeshComponent);
  }

  // Find or create the FInstancedMeshInfo for the UInstancedStaticMeshComponent
  FInstancedMeshInfo &MeshInfo = InstancedMeshInfos.FindOrAdd(MeshComponent);

  // Add the new instance to the UInstancedStaticMeshComponent
  MeshComponent->AddInstance(Transform, true);
  MeshInfo.Transforms.Add(Transform);
  MeshInfo.InstancedType = InstancedType;
}

bool AInstanceManager::RemoveInstancedMesh(
    UStaticMesh *Mesh, const FTransform &InstanceTransform) {
  UInstancedStaticMeshComponent **MeshComponentPtr = MeshToComponent.Find(Mesh);

  if (!MeshComponentPtr) {
    return false;
  }

  UInstancedStaticMeshComponent *MeshComponent = *MeshComponentPtr;
  FInstancedMeshInfo *MeshInfo = InstancedMeshInfos.Find(MeshComponent);

  if (MeshInfo) {
    int32 InstanceIndex = MeshInfo->Transforms.IndexOfByPredicate(
        [&](const FTransform &Transform) {
          return Transform.Equals(InstanceTransform);
        });

    if (InstanceIndex != INDEX_NONE) {
      if (MeshComponent->RemoveInstance(InstanceIndex)) {
        MeshInfo->Transforms.RemoveAt(InstanceIndex);
        return true;
      }
    }
  }

  return false;
}

FTransform AInstanceManager::GetTransformOfInstance(
    UInstancedStaticMeshComponent *MeshComponent, int32 InstanceIndex) {
  FInstancedMeshInfo *MeshInfo = InstancedMeshInfos.Find(MeshComponent);

  if (MeshInfo && MeshInfo->Transforms.IsValidIndex(InstanceIndex)) {
    return MeshInfo->Transforms[InstanceIndex];
  }

  return FTransform::Identity; // Returns an identity transform if instance not
                               // found
}
