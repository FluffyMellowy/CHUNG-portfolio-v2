#include "Enemies/KillSequence.h"

FTransform UKillSequence::SampleCamera(float Time) const
{
	if (CameraTrack.Num() == 0)
	{
		return FTransform::Identity;
	}
	const float Frame = FMath::Max(0.0f, Time * FrameRate);
	const int32 Index = FMath::Min(FMath::FloorToInt32(Frame), CameraTrack.Num() - 1);
	const int32 Next = FMath::Min(Index + 1, CameraTrack.Num() - 1);
	const float Alpha = FMath::Clamp(Frame - Index, 0.0f, 1.0f);
	const FTransform& A = CameraTrack[Index];
	const FTransform& B = CameraTrack[Next];
	return FTransform(FQuat::Slerp(A.GetRotation(), B.GetRotation(), Alpha), FMath::Lerp(A.GetLocation(), B.GetLocation(), Alpha));
}
