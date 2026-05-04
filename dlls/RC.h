#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()

enum
{
	RC_EXPLODE = 0, // moving satchel charge
	RC_GUN,			// shoots straight
	RC_TURRET,		// shoots automatically
};

// OVERLOADS SOME ENTVARS:
// speed - the ideal magnitude of my velocity
class CRC : public CBaseEntity
{
	int m_iTrail;
public:
	static CRC* RC_Create(unsigned int RCDamage, Vector VecSpawnPos, Vector vecDir, int RCType); // add damage, spread and owner so entities calling this can give it the proper stuff
	void Spawn() override;
	void Precache() override;

	void EXPORT Impact(CBaseEntity* pOther);
	//int ShouldCollide(CBaseEntity* pentTouched) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool StartControl(CBasePlayer* pController);
	void StopControl();
	void DriveThink();
	
	bool AttackThink();
	void ExplodeThink();

	int m_Flare;

	Vector m_SpawnPos;
	Vector m_direction;

	float m_fAttackDelay;
	float m_fJumpDelay;
	
	int m_idShard;

protected:
	CBasePlayer* m_pController;
};