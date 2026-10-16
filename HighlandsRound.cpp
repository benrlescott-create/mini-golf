#include "HighlandsRound.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"

namespace
{
TSharedPtr<FJsonValue> ReadJson(const FString& Path)
{
    FString Text;if(!FFileHelper::LoadFileToString(Text,*Path))return nullptr;
    TSharedPtr<FJsonValue> Result;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Result);return Result;
}
double Number(const TSharedPtr<FJsonObject>& O,const TCHAR* Key,double Default=0)
{double V=Default;O->TryGetNumberField(Key,V);return V;}
FVector ArrayVector(const TArray<TSharedPtr<FJsonValue>>& A)
{return FVector(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber());}
UStaticMesh* Asset(const FString& Name)
{return LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Highlands/Meshes/")+Name));}
}

AHighlandsRound::AHighlandsRound()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Close overhead camera"));Camera->SetupAttachment(RootComponent);
    Camera->FieldOfView=52;Camera->bConstrainAspectRatio=false;
    BallMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Golf ball"));BallMesh->SetupAttachment(RootComponent);
    BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHighlandsRound::BeginPlay()
{
    Super::BeginPlay();
    auto Data=ReadJson(FPaths::ProjectContentDir()/TEXT("Data/courses.json"));
    if(!Data.IsValid()||Data->Type!=EJson::Array){Problem=TEXT("Course data missing. Run Prepare Unreal Project.cmd.");return;}
    for(auto Value:Data->AsArray())
    {
        auto O=Value->AsObject();Putt::Course C;
        C.name=TCHAR_TO_UTF8(*O->GetStringField(TEXT("name")));C.tip=TCHAR_TO_UTF8(*O->GetStringField(TEXT("tip")));
        C.par=int(Number(O,TEXT("par"),2));O->TryGetBoolField(TEXT("boost"),C.boost);
        auto Start=O->GetArrayField(TEXT("start")),Cup=O->GetArrayField(TEXT("cup"));
        C.start={Start[0]->AsNumber(),Start[1]->AsNumber()};C.cup={Cup[0]->AsNumber(),Cup[1]->AsNumber()};
        for(auto P:O->GetArrayField(TEXT("polygon"))){auto A=P->AsArray();C.polygon.push_back({A[0]->AsNumber(),A[1]->AsNumber()});}
        for(auto P:O->GetArrayField(TEXT("elevation"))){auto A=P->AsArray();C.elevation.push_back({A[0]->AsNumber(),A[1]->AsNumber()});}
        for(auto P:O->GetArrayField(TEXT("obstacles")))
        {
            auto A=P->AsObject();Putt::Obstacle Ob;
            Ob.circle=A->GetStringField(TEXT("type"))==TEXT("circle");
            Ob.x=Number(A,TEXT("x"));Ob.z=Number(A,TEXT("z"));Ob.r=Number(A,TEXT("r"));
            Ob.w=Number(A,TEXT("w"));Ob.d=Number(A,TEXT("d"));Ob.angle=Number(A,TEXT("angle"));Ob.spin=Number(A,TEXT("spin"));
            C.obstacles.push_back(Ob);
        }
        Courses.push_back(C);
    }
    BuildWorld();if(!Problem.IsEmpty()||Courses.empty())return;
    Scores.Init(0,int(Courses.size()));FString BestText;
    if(FFileHelper::LoadFileToString(BestText,*(FPaths::ProjectSavedDir()/TEXT("best-score.txt"))))Best=FCString::Atoi(*BestText);
    if(auto* PC=Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor=true;PC->SetViewTarget(this);
        FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);
    }
    Ready=true;LoadHole(0);
}

void AHighlandsRound::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if(auto* PC=Cast<APlayerController>(NewController))
    {
        PC->bShowMouseCursor=true;PC->SetViewTarget(this);
        FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);
    }
}

void AHighlandsRound::BuildWorld()
{
    auto Data=ReadJson(FPaths::ProjectContentDir()/TEXT("Data/highlands.json"));
    if(!Data.IsValid()){Problem=TEXT("Landscape assets have not been prepared. Run Prepare Unreal Project.cmd.");return;}
    TMap<FString,UHierarchicalInstancedStaticMeshComponent*> Groups;
    for(auto Value:Data->AsObject()->GetArrayField(TEXT("placements")))
    {
        auto O=Value->AsObject();FString Name=O->GetStringField(TEXT("asset"));
        if(Name==TEXT("SM_GolfBall"))continue;
        FVector Location=ArrayVector(O->GetArrayField(TEXT("location"))),Scale=ArrayVector(O->GetArrayField(TEXT("scale")));
        float Yaw=Number(O,TEXT("yaw"));int Course=int(Number(O,TEXT("course"),-1)),Obstacle=int(Number(O,TEXT("obstacle"),-1));
        UStaticMesh* Mesh=Asset(Name);
        if(!Mesh){Problem=TEXT("Native mesh missing: ")+Name;return;}
        if(Course>=0)
        {
            auto* Part=NewObject<UStaticMeshComponent>(this);Part->SetupAttachment(RootComponent);
            Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Mesh);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->RegisterComponent();
            Part->SetRelativeTransform(FTransform(FRotator(0,Yaw,0),Location,Scale));CourseParts.Add({Part,Course,Obstacle,Location});
        }
        else
        {
            UHierarchicalInstancedStaticMeshComponent* Group=Groups.FindRef(Name);
            if(!Group)
            {
                Group=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);Group->SetupAttachment(RootComponent);
                Group->SetStaticMesh(Mesh);Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Group->SetMobility(EComponentMobility::Movable);Group->RegisterComponent();
                if(Name.Contains(TEXT("Grass"))||Name.Contains(TEXT("Fern")))Group->SetCullDistances(4500,8000);
                Groups.Add(Name,Group);ForestGroups.Add(Group);
            }
            Group->AddInstance(FTransform(FRotator(0,Yaw,0),Location,Scale));
        }
    }
    BallMesh->SetStaticMesh(Asset(TEXT("SM_GolfBall")));
    if(!BallMesh->GetStaticMesh())Problem=TEXT("Dimpled ball mesh missing.");
}

void AHighlandsRound::LoadHole(int Index)
{
    if(Index<0||Index>=int(Courses.size()))return;
    Hole=Index;Strokes=0;Ball=Putt::create(Courses[Hole]);SimTime=Accumulator=SinkTime=Power=0;Charging=MouseCharging=false;Aim=FVector2D(0,-1);
    BallMesh->SetVisibility(true);BallMesh->SetWorldLocation(FVector(Ball.x,Ball.z,Ball.y)*100);
    FVector Target(Ball.x*55,Ball.z*55-35,Putt::height(Courses[Hole],Ball.z*.55-.35)*100+25);
    Camera->SetWorldLocation(Target+FVector(680,1100,1050)/Zoom);
    Camera->SetWorldRotation((Target-Camera->GetComponentLocation()).Rotation());
    for(auto Part:CourseParts)Part.Mesh->SetVisibility(Part.Hole==Hole,true);
}

bool AHighlandsRound::MousePoint(FVector2D& Out) const
{
    auto* PC=Cast<APlayerController>(GetController());if(!PC)return false;
    FVector Origin,Direction;if(!PC->DeprojectMousePositionToWorld(Origin,Direction)||FMath::Abs(Direction.Z)<.001)return false;
    double T=(Ball.y*100-Origin.Z)/Direction.Z;if(T<=0)return false;
    FVector P=Origin+Direction*T;Out=FVector2D(P.X,P.Y)/100;return true;
}

void AHighlandsRound::SaveBest()
{
    int Total=0;for(int Score:Scores)Total+=Score;
    if(Best==0||Total<Best){Best=Total;FFileHelper::SaveStringToFile(FString::FromInt(Best),*(FPaths::ProjectSavedDir()/TEXT("best-score.txt")));}
}

void AHighlandsRound::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);if(!Ready)return;
    float Dt=FMath::Min(DeltaSeconds,.1f);auto* PC=Cast<APlayerController>(GetController());if(!PC)return;
    if(PC->WasInputKeyJustPressed(EKeys::Escape))PC->ConsoleCommand(TEXT("quit"));
    if(PC->WasInputKeyJustPressed(EKeys::C))Scenic=!Scenic;
    if(PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))Zoom=FMath::Clamp(Zoom*1.1f,.65f,1.8f);
    if(PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))Zoom=FMath::Clamp(Zoom/1.1f,.65f,1.8f);
    if(PC->WasInputKeyJustPressed(EKeys::F1))PC->ConsoleCommand(TEXT("sg.GlobalIlluminationQuality 1"));
    if(PC->WasInputKeyJustPressed(EKeys::F2))PC->ConsoleCommand(TEXT("sg.GlobalIlluminationQuality 3"));
    if(PC->WasInputKeyJustPressed(EKeys::T)){Complete=false;Scores.Init(0,int(Courses.size()));LoadHole(0);}
    if(PC->WasInputKeyJustPressed(EKeys::R)&&!Complete)LoadHole(Hole);
    if(!Complete&&!Ball.sunk&&Ball.speed()<.01)
    {
        double Turn=(PC->IsInputKeyDown(EKeys::Right)?1:0)-(PC->IsInputKeyDown(EKeys::Left)?1:0);
        Aim=Aim.GetRotated(Turn*90*Dt);
        if(PC->WasInputKeyJustPressed(EKeys::SpaceBar)){Charging=true;MouseCharging=false;Power=0;}
        if(PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)&&MousePoint(DragStart)){Charging=true;MouseCharging=true;Power=0;}
        if(Charging)
        {
            if(MouseCharging)
            {
                FVector2D P;if(MousePoint(P))
                {auto D=DragStart-P;if(D.Size()>.04){Aim=D.GetSafeNormal();Power=FMath::Clamp(float(D.Size()/3),0.f,1.f);}}
            }
            else Power=FMath::Min(1.f,Power+Dt*.65f);
            bool Release=MouseCharging?PC->WasInputKeyJustReleased(EKeys::LeftMouseButton):PC->WasInputKeyJustReleased(EKeys::SpaceBar);
            if(PC->WasInputKeyJustPressed(EKeys::RightMouseButton)){Charging=false;Power=0;}
            else if(Release)
            {if(Putt::strike(Ball,{Aim.X,Aim.Y},Power))++Strokes;Charging=false;Power=0;}
        }
    }
    if(!Complete&&!Ball.sunk)
    {
        Accumulator+=Dt;
        while(Accumulator>=1.f/120){SimTime+=1.f/120;Putt::step(Ball,Courses[Hole],1.0/120,SimTime);Accumulator-=1.f/120;}
        if(Strokes>=12&&Ball.speed()==0)Ball.sunk=true;
    }
    if(Ball.sunk&&!Complete)
    {
        SinkTime+=Dt;BallMesh->SetWorldLocation(FVector(Courses[Hole].cup.x,Courses[Hole].cup.z,Putt::height(Courses[Hole],Courses[Hole].cup.z)-FMath::Min(SinkTime,.4f))*100);
        if(SinkTime>2||PC->WasInputKeyJustPressed(EKeys::Enter))
        {Scores[Hole]=Strokes;if(Hole+1<int(Courses.size()))LoadHole(Hole+1);else{Complete=true;SaveBest();}}
    }
    else
    {
        FVector Position(Ball.x*100,Ball.z*100,Ball.y*100);
        BallMesh->SetWorldLocation(Position);
        FVector Axis(Ball.vz,-Ball.vx,0);if(!Axis.IsNearlyZero())BallMesh->AddWorldRotation(FQuat(Axis.GetSafeNormal(),Ball.speed()*Dt/Putt::Radius));
    }
    for(auto Part:CourseParts)if(Part.Hole==Hole&&Part.Obstacle>=0)
    {
        const auto& Ob=Courses[Hole].obstacles[Part.Obstacle];
        Part.Mesh->SetRelativeRotation(FRotator(0,FMath::RadiansToDegrees(Ob.angle+Ob.spin*SimTime),0));
    }
    FVector Target(Ball.x*55,Ball.z*55-35,Putt::height(Courses[Hole],Ball.z*.55-.35)*100+25);
    FVector Position=Target+FVector(680,1100,1050)/Zoom;
    if(Scenic){Position=FVector(3200,5800,2400)/Zoom;Target=FVector(0,-800,400);Camera->FieldOfView=65;}
    else Camera->FieldOfView=52;
    Camera->SetWorldLocation(FMath::VInterpTo(Camera->GetComponentLocation(),Position,Dt,4));
    Camera->SetWorldRotation((Target-Camera->GetComponentLocation()).Rotation());
    if(!Complete&&!Scenic&&!Ball.sunk&&Ball.speed()<.01)
    {
        FVector Prev(Ball.x*100,Ball.z*100,Ball.y*100);
        for(int i=1;i<=18;++i)
        {
            double X=Ball.x+Aim.X*i*.17,Z=Ball.z+Aim.Y*i*.17;
            FVector Next(X*100,Z*100,(Putt::height(Courses[Hole],Z)+.1)*100);
            if(i%2)DrawDebugLine(GetWorld(),Prev,Next,FColor(98,245,205),false,0,0,2);Prev=Next;
        }
    }
}

void AHighlandsHUD::DrawHUD()
{
    Super::DrawHUD();auto* G=Cast<AHighlandsRound>(GetOwningPawn());if(!G||!Canvas)return;
    float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Clamp(W/1500,.7f,1.5f);
    auto Text=[&](const FString& T,float X,float Y,float Size=1.f,FLinearColor C=FLinearColor::White)
    {DrawText(T,C,X,Y,GEngine->GetMediumFont(),S*Size,false);};
    DrawRect(FLinearColor(.025,.055,.07,.9),24,22,420*S,112*S);
    Text(TEXT("PUTT POP  /  HIGHLANDS"),42,35,1.25,FLinearColor(.45,1,.8));
    if(!G->Ready){Text(G->Problem,42,78,.8);return;}
    const auto& Course=G->Courses[G->Hole];
    Text(FString::Printf(TEXT("%d / 6   %s"),G->Hole+1,UTF8_TO_TCHAR(Course.name.c_str())),42,70);
    Text(FString::Printf(TEXT("PAR %d     STROKES %d     BEST %s"),Course.par,G->Strokes,G->Best?*FString::FromInt(G->Best):TEXT("--")),42,102,.8);
    DrawRect(FLinearColor(.025,.055,.07,.88),24,H-98,780*S,76);
    Text(TEXT("Drag back & release to putt  |  Arrows + hold Space"),42,H-86,.8);
    Text(TEXT("C landscape   Wheel zoom   R retry   T new round   F1/F2 quality"),42,H-57,.72,FLinearColor(.6,.8,.78));
    if(G->Power>0)
    {DrawRect(FLinearColor(.04,.1,.12,.9),W*.5-150,H-145,300,16);DrawRect(FLinearColor(.45,1,.7),W*.5-150,H-145,300*G->Power,16);}
    if(G->Ball.sunk||G->Complete)
    {
        DrawRect(FLinearColor(.025,.055,.07,.94),W*.5-230,H*.36,460,175);
        Text(G->Complete?TEXT("ROUND COMPLETE"):G->Strokes==1?TEXT("HOLE IN ONE!"):TEXT("NICE PUTT!"),W*.5-195,H*.36+24,1.4,FLinearColor(.5,1,.77));
        if(G->Complete)
        {
            FString Scores;int Total=0;for(int V:G->Scores){Scores+=FString::Printf(TEXT("%d   "),V);Total+=V;}
            Text(Scores,W*.5-195,H*.36+70);Text(FString::Printf(TEXT("TOTAL %d  |  T to play again"),Total),W*.5-195,H*.36+113,.9);
        }
        else Text(TEXT("Next hole in a moment..."),W*.5-195,H*.36+82,.9);
    }
}

AHighlandsGameMode::AHighlandsGameMode()
{DefaultPawnClass=AHighlandsRound::StaticClass();HUDClass=AHighlandsHUD::StaticClass();}

AHighlandsPreview::AHighlandsPreview()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetActorHiddenInGame(true);
#if WITH_EDITORONLY_DATA
    bIsEditorOnlyActor=true;
#endif
}

void AHighlandsPreview::BeginPlay(){Super::BeginPlay();Destroy();}

void AHighlandsPreview::BuildPreview()
{
#if WITH_EDITOR
    TArray<UActorComponent*> Previous=GetInstanceComponents();
    for(auto* Part:Previous)if(Part!=RootComponent)Part->DestroyComponent();
    auto Data=ReadJson(FPaths::ProjectContentDir()/TEXT("Data/highlands.json"));
    if(!Data.IsValid())return;
    TMap<FString,UHierarchicalInstancedStaticMeshComponent*> Groups;
    for(auto Value:Data->AsObject()->GetArrayField(TEXT("placements")))
    {
        auto O=Value->AsObject();int Course=int(Number(O,TEXT("course"),-1));if(Course>0)continue;
        FString Name=O->GetStringField(TEXT("asset"));auto* Mesh=Asset(Name);if(!Mesh)continue;
        auto* Group=Groups.FindRef(Name);
        if(!Group)
        {
            Group=NewObject<UHierarchicalInstancedStaticMeshComponent>(this,NAME_None,RF_Transactional);
            Group->SetupAttachment(RootComponent);Group->SetStaticMesh(Mesh);
            Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);AddInstanceComponent(Group);Group->RegisterComponent();Groups.Add(Name,Group);
        }
        Group->AddInstance(FTransform(FRotator(0,Number(O,TEXT("yaw")),0),ArrayVector(O->GetArrayField(TEXT("location"))),ArrayVector(O->GetArrayField(TEXT("scale")))));
    }
    MarkPackageDirty();
#endif
}
