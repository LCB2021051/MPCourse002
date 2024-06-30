// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"


void AProjectileWeapon::Fire(const FVector &HitTarget)
{
    Super::Fire(HitTarget);
    APawn* InstigatorPawn = Cast<APawn>(GetOwner());
    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));

    if(MuzzleFlashSocket)
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        // from muzzle flase socket to HitLocation from TraceUnderCrossHair
        FVector ToTarget = HitTarget - SocketTransform.GetLocation();
        FRotator TargetRotator = ToTarget.Rotation();
        if(ProjectileClass && InstigatorPawn)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = GetOwner();
            SpawnParams.Instigator = InstigatorPawn;

            UWorld* World = GetWorld();
            World->SpawnActor<AProjectile>(
                ProjectileClass,
                SocketTransform.GetLocation(),
                TargetRotator,
                SpawnParams
            );
        }
    }
}
