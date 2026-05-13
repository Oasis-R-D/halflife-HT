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
class CFuncNoBuild : public CBaseTrigger
{
public:
	void Spawn() override;
    bool KeyValue(KeyValueData* pkvd) override;
    void Touch(CBaseEntity* pOther) override;
    
private:
	bool	m_bAllowSentry;
	bool	m_bAllowDispenser;
	bool	m_bAllowTeleporters;
	bool	m_bDestroyBuildingsOnActive = false;
};

LINK_ENTITY_TO_CLASS( func_nobuild, CFuncNoBuild);
void CFuncNoBuild::Spawn()
{
	InitTrigger();


	SetUse(&CFuncNoBuild::ToggleUse);
	SetTouch(&CFuncNoBuild::Touch);
}

void CFuncNoBuild::Touch(CBaseEntity* pOther)
{
	if (m_bDestroyBuildingsOnActive)
	{
		/*
		if ((pOther->pev->flags & FL_BUILDABLE) != 0)
		{
		 	CBuildable* building = dynamic_cast<CBuildable*>(pOther);
			building->DetonateBuilding();
		}
		*/
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

	return CBaseTrigger::KeyValue(pkvd);
}