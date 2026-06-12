#include "buildables.h"

TYPEDESCRIPTION CBuildable::m_SaveData[] =
	{
		DEFINE_FIELD(CBuildable, m_Activity, FIELD_INTEGER),
		DEFINE_FIELD(CBuildable, m_hBuilder, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CBuildable, CBaseAnimating);

void CBuildable::Precache()
{
	m_idShard = PRECACHE_MODEL("models/metalplategibs.mdl");

	// precache individual buildable
	PrecacheBuildable();
}

void CBuildable::Spawn()
{
	Precache();

	// NPCs attack it
	SetBits(pev->flags, FL_MONSTER | FL_BUILDING);
	pev->flags |= FL_MONSTER | FL_BUILDING; // extraneous?
	pev->takedamage = DAMAGE_YES;
	pev->solid = SOLID_BBOX;

	m_bloodColor = DONT_BLEED;

	// run individual buildable spawn code
    SpawnBuildable();
}

int CBuildable::Classify()
{
	return CLASS_PLAYER_ALLY;
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

void CBuildable::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType, bool cangib)
{
	if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 10) < 1))
	{
		UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
		pev->dmgtime = gpGlobals->time;
	}

	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
}

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

bool CBuildable::OnWrenchHit(CBasePlayer* pPlayer)
{
	// else repair the building
	return Command_Repair( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Separated so it can be triggered by wrench hit or by vgui screen
//-----------------------------------------------------------------------------
bool CBuildable::Command_Repair(CBasePlayer* pActivator)
{
	int iAmountToHeal = V_min(100, pev->max_health - pev->health);

	// repair the building
	int iRepairCost = ceil( (float)(iAmountToHeal) * 0.2f );

	// What does this do???
	//TRACE_OBJECT( UTIL_VarArgs( "%0.2f CObjectDispenser::Command_Repair ( %d / %d ) - cost = %d\n", gpGlobals->time, GetHealth(), GetMaxHealth(), iRepairCost ) );

	if ( iRepairCost > 0 )
	{
		static int metal_ammo = pActivator->GetAmmoIndex("uranium");
		pActivator->TabulateAmmo();

		if ( iRepairCost > pActivator->ammo_metal )
		{
			iRepairCost = pActivator->ammo_metal;
		}

		pActivator->m_rgAmmo[metal_ammo] -= iRepairCost;

		float flNewHealth = V_min( pev->max_health, pev->health + ( iRepairCost * 5 ) );
		pev->health = flNewHealth;

		return ( iRepairCost > 0 );
	}

	return false;
}