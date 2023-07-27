// Fill out your copyright notice in the Description page of Project Settings.


#include "KTGeometryCollectionComponent.h"

void UKTGeometryCollectionComponent::PublicResetDynamicCollection()
{
    ResetDynamicCollection();
}

void UKTGeometryCollectionComponent::NotifyPhysicsCollision(const FChaosPhysicsCollisionInfo & CollisionInfo)
{
    Super::NotifyPhysicsCollision(CollisionInfo);
}
