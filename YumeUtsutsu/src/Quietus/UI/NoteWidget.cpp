#include "NoteWidget.h"
#include "Components/TextBlock.h"

void UNoteWidget::SetNote(const FText& InTitle, const FText& InBody)
{
	TitleText->SetText(InTitle);
	BodyText->SetText(InBody);
}
