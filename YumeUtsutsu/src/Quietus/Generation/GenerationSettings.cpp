#include "Generation/GenerationSettings.h"

FText FGenItemSettings::GetNoteBody(int32 Index) const
{
	if (NoteTexts.Num() == 0)
	{
		return NSLOCTEXT("Generator", "NoteFallback", "……読み取れない走り書き。");
	}
	return NoteTexts[Index % NoteTexts.Num()];
}
