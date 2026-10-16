#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GolfSimulation.h"
#include "HighlandsRound.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class PUTTPOP_API AHighlandsRound : public APawn
{
    GENERATED_BODY()
public:
    AHighlandsRound();
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void Tick(float DeltaSeconds) override;
    std::vector<Putt::Course> Courses;
    Putt::Ball Ball;
    int Hole=0,Strokes=0,Best=0;
    TArray<int> Scores;
    bool Complete=false,Scenic=false,Ready=false;
    float Power=0,Zoom=1;
    FString Problem;
    FVector2D Aim=FVector2D(0,-1);
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BallMesh;
    void LoadHole(int Index);
private:
    struct FCoursePart { UStaticMeshComponent* Mesh=nullptr;int Hole=-1,Obstacle=-1;FVector Base=FVector::ZeroVector; };
    TArray<FCoursePart> CourseParts;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> ForestGroups;
    float Accumulator=0,SimTime=0,SinkTime=0;
    bool Charging=false,MouseCharging=false;
    FVector2D DragStart=FVector2D::ZeroVector;
    void BuildWorld();
    bool MousePoint(FVector2D& Out) const;
    void SaveBest();
};

UCLASS()
class PUTTPOP_API AHighlandsHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};

UCLASS()
class PUTTPOP_API AHighlandsGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AHighlandsGameMode();
};

UCLASS()
class PUTTPOP_API AHighlandsPreview : public AActor
{
    GENERATED_BODY()
public:
    AHighlandsPreview();
    virtual void BeginPlay() override;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Putt Pop") void BuildPreview();
};
