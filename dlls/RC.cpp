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
#define RC_SPEED_DRIVE 1600
#define RC_SPEED_TURN 800
#define RC_SPEED_JUMP 256

#define RC_ATTACK_DELAY 0.1
#define RC_JUMP_DELAY 0.25

#define RC_TURN_FRICTION 2

// values clamped to this
#define RC_MAX_TURNSPEED 256
#define RC_MAX_DRIVESPEED 256

#define RC_CAM_OFFSET 10

LINK_ENTITY_TO_CLASS(pl_rc_cam, CRCcamera);
void CRCcamera::Spawn()
{
	SET_MODEL(ENT(pev), "models/rc.mdl"); // placeholder
	pev->classname = MAKE_STRING("pl_rc_cam");
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin(pev, pev->origin);
}

void CRCcamera::FireSound()
{
	char wpnsnd2[256];
	sprintf(wpnsnd2, "weapons/hks%d.wav", RANDOM_LONG(1, 3));
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, wpnsnd2, 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 10));
}

// controllable RC car
LINK_ENTITY_TO_CLASS(pl_rc, CRC);
CRC* CRC::RC_Create(unsigned int RCDamage, Vector VecSpawnPos, Vector vecDir, int RCType)
{
	// spawn the camera
	CRCcamera* pRCcam = GetClassPtr((CRCcamera*)NULL);
	pRCcam->pev->classname = MAKE_STRING("pl_rc_cam");
	pRCcam->pev->origin = VecSpawnPos+Vector(0, 0, RC_CAM_OFFSET);
	pRCcam->pev->angles = vecDir;
	pRCcam->Spawn();

	// Create a new entity with CRC private data
	CRC* pRC = GetClassPtr((CRC*)NULL);
	pRC->pev->classname = MAKE_STRING("pl_rc");
	pRC->m_iDamage = RCDamage;
	pRC->m_SpawnPos = VecSpawnPos;
	pRC->m_direction = vecDir;
	pRC->m_Flare = RCType; // tracer type
	pRC->pev->owner = NULL;
	pRC->m_pCamera = pRCcam;
	pRC->Spawn();
	
	return pRC;
}

void CRC::Precache()
{
	PRECACHE_SOUND("weapons/hks1.wav");
	PRECACHE_SOUND("weapons/hks3.wav");
	PRECACHE_SOUND("weapons/hks2.wav");

	PRECACHE_SOUND("rc/drive_loop2.wav");
	PRECACHE_SOUND("rc/power_jump.wav");

	PRECACHE_MODEL("models/rc.mdl");

	m_idShard = PRECACHE_MODEL("models/metalplategibs.mdl");
}

int CRC::Classify()
{
	return CLASS_MACHINE; // TO-DO: this prob doesn't anger friendly NPCs when friendly fire occurs
}

void CRC::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_STEP;

	pev->solid = SOLID_BBOX;
	pev->angles = m_direction;

	SET_MODEL(ENT(pev), "models/rc.mdl"); // placeholder
	UTIL_SetSize(pev, Vector(-4, -10, 0), Vector(4, 10, 8));

	UTIL_SetOrigin(pev, m_SpawnPos);

	pev->sequence = 0;
	pev->frame = 0;

	// NPCs attack it
	SetBits(pev->flags, FL_MONSTER);
	pev->flags |= FL_MONSTER; // extraneous?

	pev->takedamage = DAMAGE_YES;
	pev->health = 20;

	SetTouch(&CRC::Impact);

	// ready attacks
	m_fAttackDelay = gpGlobals->time;
	m_fExplodeDelay = gpGlobals->time + 1;
}

void CRC::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pActivator->Classify() != CLASS_PLAYER)
	{
		ExplodeThink(); // detonate when used by nonplayers)
		return;
	}

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
		ALERT(at_console, "unused\n");
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

	m_pController->m_bNoMove_RC = true;

	if (m_pCamera)
	{
		SET_VIEW(m_pController->edict(), m_pCamera->edict());
		m_pController->m_hViewEntity = m_pCamera;
		m_bCamConnected = true;
	}
	return true;
}

bool CRC::AttackThink()
{
	if (((m_pController->m_afButtonPressed & IN_ATTACK2) != 0 && m_fExplodeDelay < gpGlobals->time) || pev->waterlevel == 3)
	{	// detonate if underwater or told to
		ALERT(at_console, "detonated\n");
		ExplodeThink();
		return true;
	}
	else if (m_Flare == RC_GUN)
	{
		if ((m_pController->pev->button & IN_ATTACK) != 0 && m_fAttackDelay < gpGlobals->time)
		{
			// shoot
			UTIL_MakeVectors(pev->angles); // has to be called or else it follows mouse (which the camera doesn't)

			Vector vecSrc = pev->origin + gpGlobals->v_up * 4 + gpGlobals->v_right * 4;
			Vector vecEnd = vecSrc + gpGlobals->v_forward * 32 + gpGlobals->v_up * 2; // angle up
			Vector vecAiming = (vecEnd - vecSrc).Normalize();

			FireBullets(RANDOM_LONG(1, 2), vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 1, 0, m_pController->pev);
			
			m_pCamera.Entity<CRCcamera>()->FireSound();
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

			m_fAttackDelay = gpGlobals->time + RC_ATTACK_DELAY;
		}
	}

	return false;
}

template <typename T>
static T clamp(T &x, double value) {
	if (x > value)
		x = value;
	else if (x < -value)
		x = -value;

	return x;
}

void CRC::DriveThink()
{
	if (!m_pController)
	{
		ALERT(at_console, "no controller!\n");
		ExplodeThink();
		return;
	}

	// check attacks
	if (AttackThink() == true) 
		return; // was detonated by the player or water

	if (!m_pCamera)
	{
		ALERT(at_console, "no camera!\n");
		return;
	}
	
	//---------------------------------
	//	INPUT HANDLING
	//---------------------------------

	const bool onGround = FBitSet(pev->flags, FL_ONGROUND);

	int ft = 0, bk = 0, rt = 0, lf = 0, jmp = 0;

	if (onGround)
	{
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
			rt = -1;
		}
		
		if ((m_pController->pev->button & IN_MOVELEFT) != 0)
		{
			lf = 1;
		}

		if (m_fJumpDelay == -1)
			m_fJumpDelay = gpGlobals->time + RC_JUMP_DELAY;
		else if ((m_pController->m_afButtonPressed & IN_JUMP) != 0 && m_fJumpDelay < gpGlobals->time)
		{
			m_fJumpDelay = -1; // set once we are on ground
			jmp = 1;
		}
	}

	//---------------------------------
	//	APPLY TURNING FRICTION
	//---------------------------------

	float movespeed = pev->velocity.Length();
	clamp(movespeed, RC_MAX_DRIVESPEED);

	float turnfric = (RC_TURN_FRICTION + (1 - (movespeed / RC_MAX_DRIVESPEED))) * DT;
	pev->avelocity = pev->avelocity - (pev->avelocity * clamp(turnfric, 1.0));

	pev->angles = pev->angles + pev->avelocity * DT; // avelocity doesn't work in step movetype
	UTIL_MakeVectors(pev->angles);

	//---------------------------------
	//	PLAY DRIVE SOUNDS
	//---------------------------------
		
	if (abs(movespeed) > 0 && onGround)
	{
		EMIT_SOUND_DYN(edict(), CHAN_ITEM, "rc/drive_loop2.wav", movespeed / RC_MAX_DRIVESPEED, ATTN_NORM, SND_CHANGE_PITCH | SND_CHANGE_VOL, 75 + (movespeed / RC_MAX_DRIVESPEED) * 0.25);
	}
	else if (abs(movespeed) == 0 || !onGround)
	{
		STOP_SOUND(edict(), CHAN_ITEM, "rc/drive_loop2.wav");
	}

	//---------------------------------
	//	MOVE BASED ON INPUTS
	//---------------------------------

	int turn = lf + rt; 		// make it into 1 value, -1 if left, 1 if right, 0 if none or both
	const int drive = ft + bk;	// make it into 1 value, -1 if reverse, 1 if forward, 0 if none or both (const because it can be)
	
	// simulate front wheel turning
	if (drive == -1)
		turn *= -1;

	// move the RC car
	if (turn != 0)
	{
		pev->avelocity.y = pev->avelocity.y + (turn * RC_SPEED_TURN * DT * (movespeed / RC_MAX_DRIVESPEED));
		clamp(pev->avelocity.y, RC_MAX_TURNSPEED);
	}

	if (drive != 0)
	{
		pev->velocity = pev->velocity + (drive * gpGlobals->v_forward * RC_SPEED_DRIVE * DT);
	}

	if (jmp != 0)
	{
		pev->velocity.z += RC_SPEED_JUMP;
		EMIT_SOUND(edict(), CHAN_ITEM, "rc/power_jump.wav", 1.0, ATTN_NORM); 
	}

	//---------------------------------
	//	KEEP CAMERA UPDATED
	//---------------------------------

	m_pCamera->pev->origin = pev->origin + Vector(0, 0, RC_CAM_OFFSET);
	m_pCamera->pev->angles = pev->angles;
	// report status
	ALERT(at_console, "DRIVE: %d, TURN %d, JUMP %d, DT %f, CAM %f %f %f\n", drive, turn, jmp, DT, m_pCamera->pev->origin.x, m_pCamera->pev->origin.y, m_pCamera->pev->origin.z);
}

void CRC::ExplodeThink()
{
	pev->model = iStringNull; //invisible
	pev->solid = SOLID_NOT;	  // intangible

	pev->takedamage = DAMAGE_NO;

	STOP_SOUND(edict(), CHAN_ITEM, "rc/drive_loop2.wav");

	if (m_Flare == RC_EXPLODE && m_pController)
	{
		int iContents = UTIL_PointContents(pev->origin);

		// VFX
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
			WRITE_BYTE(TE_EXPLOSION);	// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD(pev->origin.x); // Send to PAS because of the sound
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			if (iContents != CONTENTS_WATER)
			{
				WRITE_SHORT(g_sModelIndexFireball);
			}
			else
			{
				WRITE_SHORT(g_sModelIndexWExplosion);
			}
			WRITE_BYTE((m_iDamage - 50) * .60); // scale * 10
			WRITE_BYTE(15);					   // framerate
			WRITE_BYTE(TE_EXPLFLAG_NONE);
		MESSAGE_END();

		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);
			break;
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
		
		pev->owner = NULL; // can't traceline attack owner if this is set
		
		RadiusDamage(pev->origin + Vector(0, 0, 4), pev, m_pController->pev, m_iDamage, CLASS_NONE, DMG_BLAST);
		//ExplosionCreate(pev->origin+Vector(0, 0, 4), gpGlobals->v_forward, m_pController->edict(), m_iDamage, true);
	}
	else // don't do both (optimization)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
			WRITE_BYTE(TE_BREAKMODEL);
			// position
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z+4);
			// size
			WRITE_COORD(8);
			WRITE_COORD(8);
			WRITE_COORD(8);
			// velocity
			WRITE_COORD(pev->velocity.x);
			WRITE_COORD(pev->velocity.y);
			WRITE_COORD(pev->velocity.z);
			WRITE_BYTE(25); // randomization
			// Model
			WRITE_SHORT(m_idShard); // model id#
			// # of shards
			WRITE_BYTE(5);
			// duration
			WRITE_BYTE(30); // 3.0 seconds
			WRITE_BYTE(BREAK_SMOKE); // flags
		MESSAGE_END();
	}

	UTIL_Remove(this);
	
	if (!m_pController)
		return;

	m_pController->m_bNoMove_RC = false;

	SET_VIEW(m_pController->edict(), m_pController->edict());
	m_pController->EquipWeapon();
	m_pController->m_hViewEntity = NULL;

	ALERT(at_console, "stopped using RC\n");

	m_pController->m_iHideHUD &= ~HIDEHUD_WEAPONS;
	UTIL_Remove(m_pCamera);
}

void CRC::Impact(CBaseEntity* pOther)
{ // extra collision stuff
	return;
}

void CRC::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	/*
	if (ptr->iHitgroup == 10)
	{
		// hit armor
		if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 10) < 1))
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
			pev->dmgtime = gpGlobals->time;
		}

		flDamage = 0.1; // don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}
	*/
	
	if (0 == pev->takedamage)
		return;

	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
}

// take damage. bitsDamageType indicates type of damage sustained, ie: DMG_BULLET

bool CRC::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (0 == pev->takedamage)
		return false;

	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		ALERT(at_console, "killed\n");
		pev->takedamage = DAMAGE_NO;

		ClearBits(pev->flags, FL_MONSTER); // why are they set in the first place???

		SetUse(NULL);
		ExplodeThink();

		return false;
	}

	return true;
}