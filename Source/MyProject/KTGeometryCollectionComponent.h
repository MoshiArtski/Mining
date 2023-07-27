// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "KTGeometryCollectionComponent.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API UKTGeometryCollectionComponent : public UGeometryCollectionComponent
{
	GENERATED_BODY()

	public:
    void PublicResetDynamicCollection();

	virtual void NotifyPhysicsCollision(const FChaosPhysicsCollisionInfo & CollisionInfo) override;

	
};

