#include "buildables.h"

TYPEDESCRIPTION CBuildable::m_SaveData[] =
	{
		DEFINE_FIELD(CBuildable, m_Activity, FIELD_INTEGER),
		DEFINE_FIELD(CBuildable, m_hBuilder, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CBuildable, CBaseAnimating);

bool CBuildable::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (flDamage > 0)
	{
		pev->health -= flDamage;
		if (pev->health <= 0)
		{
			pev->takedamage = DAMAGE_NO;

			ClearBits(pev->flags, FL_MONSTER); // why does this need reset, it's getting deleted?

			SetUse(NULL);
			SetThink(NULL);
			DetonateBuilding();

			return false;
		}

		return true;
	}

	return false;
}

void CBuildable::Spawn()
{
	Precache();

	// set needed flags and stuff

	// NPCs attack it
	SetBits(pev->flags, FL_MONSTER);
	pev->flags |= FL_MONSTER; // extraneous?
	pev->takedamage = DAMAGE_YES;
	pev->solid = SOLID_BBOX;

	m_bloodColor = DONT_BLEED;

    SpawnBuildable();
}
void CBuildable::Precache()
{
	m_idShard = PRECACHE_MODEL("models/metalplategibs.mdl");
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