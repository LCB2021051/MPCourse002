// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MPCourse002/Weapon/Weapon.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
    Super::NativeUpdateAnimation(DeltaTime);

    if(BlasterCharacter == nullptr)
    {
        BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
    }
    if(BlasterCharacter == nullptr) return;

    FVector Velocity = BlasterCharacter->GetVelocity();
    Velocity.Z = 0.f;
    speed = Velocity.Size();
    
    bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
    bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false ;
    bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();
    EquippedWeapon = BlasterCharacter->GetEquippedWeapon();
    bIsCrouched = BlasterCharacter->bIsCrouched;
    bAiming = BlasterCharacter->IsAiming();
    TurningInPlace = BlasterCharacter->GetTurningInPlace();

    // offset Yaw for strafing(lean)
    FRotator AimRotation = BlasterCharacter->GetBaseAimRotation(); // camera rotation
    FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity()); // character rotation
    FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation,AimRotation); // character to camera rotation 
    DeltaRotation = FMath::RInterpTo(DeltaRotation,DeltaRot,DeltaTime, 6.f); 
    YawOffset = DeltaRotation.Yaw;

    CharacterRotationLastFrame = CharacterRotation; 
    CharacterRotation = BlasterCharacter->GetActorRotation(); 
    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation,CharacterRotationLastFrame);
    const float Target = Delta.Yaw / DeltaTime;
    const float Interp = FMath::FInterpTo(Lean,Target,DeltaTime,6.f);
    Lean = FMath::Clamp(Interp, -90.f, 90.f);

    AO_Yaw = BlasterCharacter->GetAO_Yaw();
    AO_Pitch = BlasterCharacter->GetAO_Pitch();

    if(bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
    {
        LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"),ERelativeTransformSpace::RTS_World);
        FVector OutPosition;
        FRotator OutRotation;
        // This method transforms the LeftHandTransform location from world space to the bone space of the character’s right hand (hand_r).
        BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
        LeftHandTransform.SetLocation(OutPosition);
        LeftHandTransform.SetRotation(FQuat(OutRotation));
    }
}
