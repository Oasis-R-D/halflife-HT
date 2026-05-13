//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "gamerules.h"
#include "skill.h"
#include "buildables.h"

//-----------------------------------------------------------------------------
// Purpose: Defines an area where objects cannot be built
//-----------------------------------------------------------------------------
class CFuncNoBuild : public CBaseToggle
{
public:
	void Spawn() override;
    bool KeyValue(KeyValueData* pkvd) override;
    void Touch(CBaseEntity* pOther) override;
    void EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void InitTrigger();

private:
	bool	m_bAllowSentry;
	bool	m_bAllowDispenser;
	bool	m_bAllowTeleporters;
	bool	m_bDestroyBuildingsOnActive = false;
};

LINK_ENTITY_TO_CLASS(func_no_build, CFuncNoBuild);

/*
================
InitTrigger
================
*/
void CFuncNoBuild::InitTrigger()
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (pev->angles != g_vecZero)
		SetMovedir(pev);
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model)); // set size and link into world
	if (CVAR_GET_FLOAT("showtriggers") == 0)
		SetBits(pev->effects, EF_NODRAW);
}

void CFuncNoBuild::Spawn()
{
	InitTrigger();


	SetUse(&CFuncNoBuild::ToggleUse);
	SetTouch(&CFuncNoBuild::Touch);
}

//
// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
//
void CFuncNoBuild::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->solid == SOLID_NOT)
	{ // if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;

		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{ // turn the trigger off
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin(pev, pev->origin);
}

void CFuncNoBuild::Touch(CBaseEntity* pOther)
{
	if (m_bDestroyBuildingsOnActive)
	{
		if ((pOther->pev->flags & FL_BUILDING) != 0)
		{
		 	CBuildable* building = dynamic_cast<CBuildable*>(pOther);
			building->DetonateBuilding();
		}
	}
	// TO-DO: flag to detect a buildable, baseclass for buildable to make detonation easier
}

bool CFuncNoBuild::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sentries"))
	{
		m_bAllowSentry = atoi(pkvd->szValue) > 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "dispensers"))
	{
		m_bAllowDispenser = atoi(pkvd->szValue) > 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "teleporters"))
	{
		m_bAllowTeleporters = atoi(pkvd->szValue) > 0;
		return true;
	}
    else if (FStrEq(pkvd->szKeyName, "destroyactive"))
	{
		m_bDestroyBuildingsOnActive = atoi(pkvd->szValue) > 0;
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}