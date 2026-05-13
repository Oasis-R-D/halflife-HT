#include "buildables.h"

TYPEDESCRIPTION CBuildable::m_SaveData[] =
	{
		DEFINE_FIELD(CBuildable, m_Activity, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CBuildable, CBaseAnimating);

void CBuildable::Spawn()
{
    SpawnBuildable();
}

void CBuildable::SetActivity(Activity act)
{
	int sequence = LookupActivity(act);
	if (sequence != ACTIVITY_NOT_AVAILABLE)
	{
		pev->sequence = sequence;
		m_Activity = act;
		pev->frame = 0;
		ResetSequenceInfo();
	}
}