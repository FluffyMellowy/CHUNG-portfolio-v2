#include "AnimScriptingLibrary.h"
#include "Animation/BlendSpace.h"

void UAnimScriptingLibrary::RebuildBlendSpace(UBlendSpace* BlendSpace)
{
	if (!BlendSpace)
	{
		return;
	}
	BlendSpace->Modify();
	BlendSpace->ResampleData();
	BlendSpace->MarkPackageDirty();
}
