#include "FontScriptingLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"

UFontFace* UFontScriptingLibrary::ImportFontFace(const FString& SourceFile, const FString& PackagePath, const FString& AssetName)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *SourceFile))
	{
		UE_LOG(LogTemp, Error, TEXT("ImportFontFace: cannot read %s"), *SourceFile);
		return nullptr;
	}

	UPackage* Package = CreatePackage(*(PackagePath / AssetName));
	UFontFace* Face = FindObject<UFontFace>(Package, *AssetName);
	const bool bCreated = Face == nullptr;
	if (bCreated)
	{
		Face = NewObject<UFontFace>(Package, *AssetName, RF_Public | RF_Standalone);
	}

	Face->Modify();
	Face->SourceFilename = SourceFile;
	Face->FontFaceData->SetData(MoveTemp(Data));
	Face->CacheSubFaces();
	Package->MarkPackageDirty();
	if (bCreated)
	{
		FAssetRegistryModule::AssetCreated(Face);
	}
	return Face;
}
