// Fill out your copyright notice in the Description page of Project Settings.


#include "CombactComponent.h"
#include "MPCourse002/Character/BlasterCharacter.h"
#include "MPCourse002/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UCombactComponent::UCombactComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
	// ...
}

void UCombactComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombactComponent,EquippedWeapon);
	DOREPLIFETIME(UCombactComponent,bAiming);
}

void UCombactComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	if(BlasterCharacter){
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void UCombactComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	if(BlasterCharacter){
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

// Server RPC
void UCombactComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if(BlasterCharacter){
		BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombactComponent::OnRep_EquippedWeapon()
{
	if(EquippedWeapon && BlasterCharacter){
		BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		BlasterCharacter->bUseControllerRotationYaw = true;
	}
}

void UCombactComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if(bFireButtonPressed)
	{
		FHitResult HitResult;
		TraceUnderCrossHairs(HitResult);
		ServerFire(HitResult.ImpactPoint);
	}	
}

// Server RPC
void UCombactComponent::ServerFire_Implementation(const FVector_NetQuantize& TracerHitTarget) 
{
	MulticastFire(TracerHitTarget);
}

// Multicast RPC
void UCombactComponent::MulticastFire_Implementation(const FVector_NetQuantize& TracerHitTarget)
{
	if(EquippedWeapon == nullptr) return;
	if(BlasterCharacter)
	{
		BlasterCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TracerHitTarget);
	}
}

void UCombactComponent::TraceUnderCrossHairs(FHitResult &TraceHitResult)
{
	FVector2D ViewportSize; 
	if(GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	
	FVector2D CrosshairLocation(ViewportSize.X / 2.f , ViewportSize.Y / 2.f); 

	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;

	bool Successful = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this,0),
		CrosshairLocation,
		CrossHairWorldPosition,
		CrossHairWorldDirection
	);

	if(Successful)
	{
		FVector Start = CrossHairWorldPosition;
		FVector End = CrossHairWorldDirection + CrossHairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
	}
}

void UCombactComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}



void UCombactComponent::EquipWeapon(AWeapon *WeaponToEquip)
{
	if(BlasterCharacter == nullptr || WeaponToEquip == nullptr) return; 

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if(HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(BlasterCharacter);
	BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	BlasterCharacter->bUseControllerRotationYaw = true;
}
