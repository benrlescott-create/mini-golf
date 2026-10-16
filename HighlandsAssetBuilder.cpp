#include "HighlandsAssetBuilder.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#if WITH_EDITOR
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Serialization/MemoryReader.h"
#endif

bool UHighlandsAssetBuilder::BuildAssets(const FString& SourceDirectory)
{
#if WITH_EDITOR
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *(SourceDirectory / TEXT("*.ppmesh")), true, false);
    if (Files.IsEmpty()) { UE_LOG(LogTemp, Error, TEXT("No Blender .ppmesh files in %s"), *SourceDirectory); return false; }
    Files.Sort();
    for (const FString& File : Files)
    {
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *(SourceDirectory/File))) return false;
        FMemoryReader Reader(Bytes, true);
        uint32 Magic=0, Vertices=0, Triangles=0, Slots=0;
        Reader << Magic << Vertices << Triangles << Slots;
        if (Magic != 0x334D5050 || Vertices > 5000000 || Triangles > 5000000 || Slots > 1024) return false;
        TArray<FString> Materials;
        for (uint32 i=0;i<Slots;++i)
        {
            uint32 Length=0; Reader << Length;
            if (Length > 4096 || Reader.Tell()+Length>Bytes.Num()) return false;
            TArray<ANSICHAR> Text;Text.SetNumZeroed(Length+1);Reader.Serialize(Text.GetData(),Length);
            Materials.Add(FString(UTF8_TO_TCHAR(Text.GetData())));
        }
        if (Reader.Tell()+int64(Vertices)*32+int64(Triangles)*16 != Bytes.Num()) return false;
        struct FPoint { FVector3f P,N;FVector2f UV; };
        TArray<FPoint> Points;Points.SetNum(Vertices);
        for (FPoint& P : Points)
            Reader << P.P.X << P.P.Y << P.P.Z << P.N.X << P.N.Y << P.N.Z << P.UV.X << P.UV.Y;
        const FString Name=FPaths::GetBaseFilename(File);
        const FString PackageName=TEXT("/Game/Highlands/Meshes/")+Name;
        UPackage* Package=CreatePackage(*PackageName);
        UStaticMesh* Mesh=FindObject<UStaticMesh>(Package,*Name);
        const bool New=Mesh==nullptr;
        if(New)Mesh=NewObject<UStaticMesh>(Package,*Name,RF_Public|RF_Standalone);
        Mesh->GetStaticMaterials().Reset();
        FMeshDescription Description;FStaticMeshAttributes Attr(Description);Attr.Register();
        auto Positions=Attr.GetVertexPositions();auto Normals=Attr.GetVertexInstanceNormals();
        auto UVs=Attr.GetVertexInstanceUVs();UVs.SetNumChannels(1);
        auto MaterialNames=Attr.GetPolygonGroupMaterialSlotNames();
        TArray<FVertexID> IDs;
        for(const FPoint& P:Points){auto ID=Description.CreateVertex();Positions[ID]=P.P;IDs.Add(ID);}
        TArray<FPolygonGroupID> Groups;
        for(FString MaterialName:Materials)
        {
            for(TCHAR& Ch:MaterialName)
                if(!((Ch>='a'&&Ch<='z')||(Ch>='A'&&Ch<='Z')||(Ch>='0'&&Ch<='9')||Ch=='_'))Ch='_';
            const FName Slot(*MaterialName);
            auto Group=Description.CreatePolygonGroup();Groups.Add(Group);MaterialNames[Group]=Slot;
            UMaterialInterface* Material=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Highlands/Materials/M_")+MaterialName));
            Mesh->GetStaticMaterials().Add(FStaticMaterial(Material,Slot,Slot));
        }
        for(uint32 t=0;t<Triangles;++t)
        {
            uint32 A,B,C,Mat;Reader<<A<<B<<C<<Mat;
            if(A>=Vertices||B>=Vertices||C>=Vertices||Mat>=Slots)return false;
            TArray<FVertexInstanceID> Corners;
            for(uint32 Index:{A,B,C})
            {
                auto VI=Description.CreateVertexInstance(IDs[Index]);
                Normals[VI]=Points[Index].N;UVs.Set(VI,0,Points[Index].UV);Corners.Add(VI);
            }
            Description.CreatePolygon(Groups[Mat],Corners);
        }
        const bool Foliage=Name.Contains(TEXT("Pine"))||Name.Contains(TEXT("Fern"))||Name.Contains(TEXT("Grass"));
        Mesh->SetNumSourceModels(Foliage?3:1);
        auto& Build=Mesh->GetSourceModel(0).BuildSettings;
        Build.bRecomputeNormals=false;Build.bRecomputeTangents=true;
        Build.bGenerateLightmapUVs=false;Build.bRemoveDegenerates=true;
        Mesh->CreateMeshDescription(0,MoveTemp(Description));Mesh->CommitMeshDescription(0);
        if(Foliage)
        {
            Mesh->bAutoComputeLODScreenSize=false;Mesh->GetSourceModel(0).ScreenSize.Default=1.f;
            Mesh->GetSourceModel(1).ReductionSettings.PercentTriangles=.32f;
            Mesh->GetSourceModel(1).ScreenSize.Default=.17f;
            Mesh->GetSourceModel(2).ReductionSettings.PercentTriangles=.08f;
            Mesh->GetSourceModel(2).ScreenSize.Default=.055f;
        }
        Mesh->NaniteSettings.bEnabled=Name.Contains(TEXT("Terrain"))||Name.Contains(TEXT("Boulder"))||Name.Contains(TEXT("Pine"));
        Mesh->NaniteSettings.bPreserveArea=Name.Contains(TEXT("Pine"));
        Mesh->Build(false);Mesh->MarkPackageDirty();
        if(New)FAssetRegistryModule::AssetCreated(Mesh);
        const FString Filename=FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
        if(!UPackage::SavePackage(Package,Mesh,*Filename,Args))return false;
        UE_LOG(LogTemp,Display,TEXT("PuttPop built %s (%u triangles)"),*Name,Triangles);
    }
    return true;
#else
    return false;
#endif
}
