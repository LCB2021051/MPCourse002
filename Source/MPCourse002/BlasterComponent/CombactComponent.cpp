// Fill out your copyright notice in the Description page of Project Settings.


#include "CombactComponent.h"
#include "MPCourse002/Character/BlasterCharacter.h"
#include "MPCourse002/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombactComponent::UCombactComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;
	// ...
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

	if(EquippedWeapon == nullptr) return;
	if(BlasterCharacter && bFireButtonPressed)
	{
		BlasterCharacter->PlayFireMontage(bAiming);
		EquippedWeapon->Fire();
	}
}

void UCombactComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UCombactComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombactComponent,EquippedWeapon);
	DOREPLIFETIME(UCombactComponent,bAiming);
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
