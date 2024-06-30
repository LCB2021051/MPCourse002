// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "MPCourse002/Weapon/Weapon.h"
#include "MPCourse002/BlasterComponent/CombactComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"

#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"


ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	CombatComponent = CreateDefaultSubobject<UCombactComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true; // used to enable crouching
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter,OverlappingWeapon,COND_OwnerOnly); // macro (ThisClass,variable)
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if(CombatComponent)
	{
		CombatComponent->BlasterCharacter = this;
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if(CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// AddInputMappingContext2Player();

	if(!HasAuthority())
	{
		if(const APlayerController* PlayerController = Cast<APlayerController>(Controller))
		{
			if(UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->AddMappingContext(BlasterMappingContext,0);
			}
		}
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);
}
void ABlasterCharacter::AddInputMappingContext2Player()
{
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerController is Valid"));
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(BlasterMappingContext, 0);
			if(HasAuthority())
			{
            	UE_LOG(LogTemp, Error, TEXT("Server: Mapping Context Added"));
			}
			else 
			{
            	UE_LOG(LogTemp, Error, TEXT("Client: Mapping Context Added"));
			}
        }
        else
        {
			if(HasAuthority())
			{
            	UE_LOG(LogTemp, Error, TEXT("Server: Unable to Add MappingContext"));
			}
			else
			{
            	UE_LOG(LogTemp, Error, TEXT("Client: Unable to Add MappingContext"));
			}
        }
    }
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (CombatComponent && CombatComponent->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
    return (CombatComponent && CombatComponent->bAiming);
}

AWeapon *ABlasterCharacter::GetEquippedWeapon()
{
	if(!CombatComponent){
    	return nullptr;
	}
	return CombatComponent->EquippedWeapon;
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (!PlayerInputComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("PlayerInputComponent is null!"));
        return;
    }

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MovementAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Jump);
        EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::EquipButtonPressed);
        EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::CrouchButtonPressed);
        EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Started, this, &ABlasterCharacter::AimingButtonPressed);
        EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Completed, this, &ABlasterCharacter::AimingButtonReleased);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ABlasterCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ABlasterCharacter::FireButtonReleased);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to cast PlayerInputComponent to EnhancedInputComponent!"));
    }
}

void ABlasterCharacter::Move(const FInputActionValue &value)
{
	const FVector2D MovementVector = value.Get<FVector2D>();
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f,Rotation.Yaw,0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, MovementVector.Y);	

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ABlasterCharacter::Look(const FInputActionValue &value)
{
	const FVector2D LookAxisVector = value.Get<FVector2D>();

	AddControllerPitchInput(LookAxisVector.Y);
	AddControllerYawInput(LookAxisVector.X);
}

void ABlasterCharacter::EquipButtonPressed()
{
    // if (GEngine) {
    //     GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Cyan, TEXT("EquipButtonPressed"));
    // }
	if(CombatComponent)
	{
		if(HasAuthority()){
			CombatComponent->EquipWeapon(OverlappingWeapon);
		}
		else {
			ServerEquipButtonPressed(); // sending RPC (Remote Procedure calls)
		}
		
	}
}


void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon) // for clients
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if(LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if(CombatComponent){
		CombatComponent->EquipWeapon(OverlappingWeapon);	
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if(bIsCrouched){
		UnCrouch();
	}
	else{
		Crouch();
	}
}

void ABlasterCharacter::AimingButtonPressed()
{
	if(CombatComponent){
		CombatComponent->SetAiming(true);
	}
}

void ABlasterCharacter::AimingButtonReleased()
{
	if(CombatComponent){
		CombatComponent->SetAiming(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// if no weapon then early return
	if(CombatComponent && CombatComponent->EquippedWeapon == nullptr) return;

	FVector Velocity = GetVelocity();
    Velocity.Z = 0.f;
    float speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if(speed == 0.f && !bIsInAir){ // standin still, not jumping
		FRotator CurrentAimRotation = FRotator(0.f,GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation,StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if(TurningInPlace == ETurningInPlace::ETIP_NotTurning){
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;

		// Turning in place
		TurnInPlace(DeltaTime);
	}
	if(speed > 0.f || bIsInAir){ // running or jumping
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;

		// Not Turning in place
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	AO_Pitch = GetBaseAimRotation().Pitch;
	if(AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch form the range of [270, 360 ) to [-90, 0]
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if(CombatComponent)
	{
		CombatComponent->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if(CombatComponent)
	{
		CombatComponent->FireButtonPressed(false);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if(AO_Yaw > 90.f){
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f){
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if(TurningInPlace != ETurningInPlace::ETIP_NotTurning){
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw,0.f,DeltaTime,4.f);
		AO_Yaw = InterpAO_Yaw;
		if(FMath::Abs(AO_Yaw) < 15.f){
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::SetOverLappingWeapon(AWeapon *Weapon) // for server
{
	if(OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if(IsLocallyControlled()) // if in server
	{
		if(OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::PossessedBy(AController *NewController)
{
	Super::PossessedBy(NewController);

	// AddInputMappingContext2Player();

	if(const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if(UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(BlasterMappingContext,0);
		}

	}
}





