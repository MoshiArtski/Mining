// Fill out your copyright notice in the Description page of Project Settings.

#include "MiningActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Field/FieldSystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/RadialForceComponent.h"

// Sets default values for this component's properties
UMiningActorComponent::UMiningActorComponent() {
  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = true;

  // ...
}

// Called when the game starts
void UMiningActorComponent::BeginPlay() {
  Super::BeginPlay();

  // ...
}

// Called every frame
void UMiningActorComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  // ...
}

void UMiningActorComponent::RayCast() {
  APlayerController *PlayerController =
      UGameplayStatics::GetPlayerController(this, 0);

  if (PlayerController) {
    FVector CamLoc;
    FRotator CamRot;
    PlayerController->GetPlayerViewPoint(CamLoc, CamRot);

    const FVector StartTrace = CamLoc;
    const FVector Direction = CamRot.Vector();
    const FVector EndTrace =
        StartTrace + Direction * 10000; // set this to desired raycast length

    FHitResult HitResult;

    const ECollisionChannel CollisionChannel =
        ECC_Visibility; // or whatever channel you are using
    FCollisionQueryParams TraceParams(FName(TEXT("RayCast")), true, GetOwner());
    TraceParams.bTraceComplex = true;

    // Draw debug line
    DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::Red, false, 1, 0,
                  1);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace,
                                             CollisionChannel, TraceParams)) {

      UInstancedStaticMeshComponent *HitMesh =
          Cast<UInstancedStaticMeshComponent>(HitResult.Component);
      if (HitMesh) {
        int32 InstanceIndex = HitResult.Item;
        OnMeshHit.Broadcast(HitMesh, InstanceIndex); // Broadcasting the event
      }
    }
  }
}

TArray<FHitResult> UMiningActorComponent::LineTraceAndSphereTrace() {
  TArray<FHitResult> SphereTraceHitResults;

  APlayerController *PlayerController =
      UGameplayStatics::GetPlayerController(this, 0);
  if (PlayerController) {
    FVector CamLoc;
    FRotator CamRot;
    PlayerController->GetPlayerViewPoint(CamLoc, CamRot);

    const FVector StartTrace = CamLoc;
    const FVector Direction = CamRot.Vector();
    const FVector EndTrace =
        StartTrace + Direction * 10000; // Set this to desired raycast length

    FHitResult LineTraceHitResult;

    const ECollisionChannel CollisionChannel =
        ECC_Visibility; // Or whatever channel you are using
    FCollisionQueryParams TraceParams(FName(TEXT("RayCast")), true, GetOwner());
    TraceParams.bTraceComplex = true;

    // Draw debug line
    DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::Red, false, 1, 0,
                  1);

    if (GetWorld()->LineTraceSingleByChannel(LineTraceHitResult, StartTrace,
                                             EndTrace, CollisionChannel,
                                             TraceParams)) {

      FVector SpawnLocation =
          LineTraceHitResult.ImpactPoint; // You can adjust this as necessary
      FRotator SpawnRotation =
          FRotator::ZeroRotator; // You can adjust this as necessary

      if (ActorToSpawn) { // Make sure the ActorToSpawn is valid
        AActor *SpawnedActor = GetWorld()->SpawnActor<AActor>(
            ActorToSpawn, SpawnLocation, SpawnRotation);
      }

      // Perform a sphere trace at the hit location of the line trace
      FCollisionShape SphereCollisionShape = FCollisionShape::MakeSphere(
          SphereTraceRadius);            // 100.0f is the radius of the sphere
      TraceParams.bTraceComplex = false; // Simple sphere trace
      TraceParams.TraceTag = TEXT("SphereTrace");

      // Draw debug sphere
      DrawDebugSphere(GetWorld(), LineTraceHitResult.ImpactPoint,
                      SphereCollisionShape.GetSphereRadius(), 12, FColor::Blue,
                      false, 1, 0, 1);

      if (GetWorld()->SweepMultiByChannel(
              SphereTraceHitResults,
              LineTraceHitResult.ImpactPoint, // Start
              LineTraceHitResult.ImpactPoint, // End
              FQuat::Identity,                // Rotation
              ECC_WorldStatic,                // Collision channel
              SphereCollisionShape,           // Shape
              TraceParams                     // Params
              )) {
        // Broadcasting the event
        OnHitResults.Broadcast(SphereTraceHitResults);
      }
    }
  }

  return SphereTraceHitResults;
}