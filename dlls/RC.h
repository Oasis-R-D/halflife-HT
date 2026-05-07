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
	RC_GUN,			// shoots (almost) straight
};

class CRCcamera : public CBaseEntity
{
public:
	void Spawn() override;
};

class CRC : public CBaseMonster
{
public:
	static CRC* RC_Create(unsigned int RCDamage, Vector VecSpawnPos, Vector vecDir, int RCType); // add damage, spread and owner so entities calling this can give it the proper stuff
	void Spawn() override;
	void Precache() override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	int Classify() override;

	int BloodColor() override { return DONT_BLEED; } // don't bleed
	void GibMonster() override {} // don't gib

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
	float m_fExplodeDelay;
	int m_idShard;
	int m_iDamage;

protected:
	CBasePlayer* m_pController;
	EHANDLE m_pCamera;
	bool m_bCamConnected;
};