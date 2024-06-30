// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "MPCourse002/BlasterTypes/TurningInPlace.h"

#include "BlasterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UInputMappingContext;
class UInputAction;
class AWeapon;
class UCombactComponent;
class UAnimMontage;

UCLASS()
class MPCOURSE002_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override; // called after construction
	void PlayFireMontage(bool bAiming);

protected:
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& value);
	void Look(const FInputActionValue& value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimingButtonPressed();
	void AimingButtonReleased();
	void AimOffset(float DeltaTime);
	void FireButtonPressed();
	void FireButtonReleased();


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* BlasterMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MovementAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EquipAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* AimingAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* FireAction;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera) 
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera) 
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon; 

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	UCombactComponent* CombatComponent;

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;

public:	
	void SetOverLappingWeapon(AWeapon* Weapon);

	void AddInputMappingContext2Player();

	bool IsWeaponEquipped();

	bool IsAiming();

	FORCEINLINE float GetAO_Yaw() const {return AO_Yaw;}
	FORCEINLINE float GetAO_Pitch() const {return AO_Pitch;}
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const {return TurningInPlace;}
};
