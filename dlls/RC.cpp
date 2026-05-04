#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "RC.h"

#define DT gpGlobals->frametime
#define RC_SPEED_DRIVE 100
#define RC_SPEED_TURN 100

// controllable RC car
LINK_ENTITY_TO_CLASS(pl_rc, CRC);
CRC* CRC::RC_Create(unsigned int RCDamage, Vector VecSpawnPos, Vector vecDir, int RCType)
{
	// Create a new entity with CRC private data
	CRC* pRC = GetClassPtr((CRC*)NULL);
	pRC->pev->classname = MAKE_STRING("pl_rc");
	pRC->m_BulletDamage = RCDamage;
	pRC->m_SpawnPos = VecSpawnPos;
	pRC->m_direction = vecDir;
	pRC->m_Flare = RCType; // tracer type
	pRC->pev->owner = NULL;
	
	pRC->Spawn();
	
	return pRC;
}

void CRC::Precache()
{
	PRECACHE_SOUND("weapons/hks1.wav");
}

void CRC::Spawn()
{
	Precache();

	pev->velocity = m_direction * 100; // toss forward a 'lil
	pev->movetype = MOVETYPE_TOSS;
	pev->friction = 0;
	pev->solid = SOLID_BBOX;
	pev->angles = m_direction;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 8));
	UTIL_SetOrigin(pev, m_SpawnPos);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_YES;
	pev->health = 20;

	SetTouch(&CRC::Impact);

	pev->nextthink = gpGlobals->time;
	
}

void CRC::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pActivator->Classify() != CLASS_PLAYER)
		return;

	if (value == 2 && useType == USE_SET)
	{
		DriveThink();
	}
	else if (!m_pController && useType != USE_OFF)
	{
		((CBasePlayer*)pActivator)->m_pRCcar = this;
		StartControl((CBasePlayer*)pActivator);
	}
	else
	{
		ExplodeThink();
	}
}

bool CRC::StartControl(CBasePlayer* pController)
{
	if (m_pController != NULL)
		return false;

	ALERT(at_console, "using RC!\n");

	m_pController = pController;
	if (m_pController->m_pActiveItem)
	{
		m_pController->m_pActiveItem->Holster();
		m_pController->pev->weaponmodel = 0;
		m_pController->pev->viewmodel = 0;
	}

	m_pController->m_iHideHUD |= HIDEHUD_WEAPONS;

	pev->nextthink = pev->ltime + 0.1;

	m_pController->pev->maxspeed = 0.00001;

	//SET_VIEW(m_pController->edict(), edict());
	//m_pController->m_hViewEntity = this;

	return true;
}

void CRC::DriveThink()
{ // player controls
	if (!m_pController)
	{
		ALERT(at_console, "RC without controller!\n");
		UTIL_Remove(this);
		return;
	}

	if ((m_pController->m_afButtonPressed & IN_ATTACK2) != 0)
	{
		ExplodeThink();
		return;
	}

	int ft = 0, bk = 0, rt = 0, lf = 0;

	// can't do both
	// braking takes higher priority
	if ((m_pController->pev->button & IN_BACK) != 0)
	{
		bk = -1;
	}
	
	if ((m_pController->pev->button & IN_FORWARD) != 0)
	{
		ft = 1;
	}

	if ((m_pController->pev->button & IN_MOVERIGHT) != 0)
	{
		rt = 1;
	}
	
	if ((m_pController->pev->button & IN_MOVELEFT) != 0)
	{
		lf = -1;
	}

	int turn = lf + rt; // make it into 1 value, -1 if left, 1 if right, 0 if none or both
	int drive = ft + bk;// make it into 1 value, -1 if reverse, 1 if forward, 0 if none or both
	
	if (turn != 0)
	{
		pev->avelocity.y += turn * RC_SPEED_TURN * DT;
	}

	if (drive != 0)
	{
		pev->velocity = pev->velocity + drive * gpGlobals->v_forward * RC_SPEED_DRIVE * DT;
	}

	ALERT(at_console, "DRIVE: %d, TURN %d\n", drive, turn);
}

void CRC::ExplodeThink()
{
	UTIL_Remove(this);
	// TODO: explode big if set to explode, otherwise break apart

	if (!m_pController)
		return;

	SET_VIEW(m_pController->edict(), m_pController->edict());
	m_pController->EquipWeapon();
	m_pController->m_hViewEntity = NULL;

	ALERT(at_console, "stopped using RC\n");

	m_pController->m_iHideHUD &= ~HIDEHUD_WEAPONS;
}

void CRC::Impact(CBaseEntity* pOther)
{ // extra collision stuff
	return;
}