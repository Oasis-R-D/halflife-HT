/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"

#ifndef CLIENT_DLL
#define NAIL_AIR_VELOCITY 2000
#define NAIL_WATER_VELOCITY 1000

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CNail : public CBaseEntity
{
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	void EXPORT BubbleThink();
	void EXPORT NailTouch(CBaseEntity* pOther);

	int m_iTrail;

public:
	Vector n_startpos;
	static CNail* NailCreate();
};
LINK_ENTITY_TO_CLASS(nailgun_nail, CNail);

CNail* CNail::NailCreate()
{
	// Create a new entity with CNail private data
	CNail* pNail = GetClassPtr((CNail*)NULL);
	pNail->pev->classname = MAKE_STRING("nail");
	pNail->Spawn();

	return pNail;
}

void CNail::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/nail.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CNail::NailTouch);
	SetThink(&CNail::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;
	n_startpos = pev->origin;
}


void CNail::Precache()
{
	PRECACHE_MODEL("models/nail.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int CNail::Classify()
{
	return CLASS_NONE;
}

void CNail::NailTouch(CBaseEntity* pOther)
{
	SetTouch(NULL);
	SetThink(NULL);
	float n_dist = (pev->origin - n_startpos).Length();
	float n_damage = (-0.1 * n_dist + pev->dmg);

	if (n_damage < 0)
	{
		// override the nail damage if it doesn't do anything
		ALERT(at_error, "CNail: Nail doing absolutely no fucking damage. Defaulting to 3.\n");
		n_damage = 3;
	}
	else if (n_damage > pev->dmg || n_dist <= 30)
	{
		ALERT(at_error, "CNail: Nail doing more damage than defined!\n");
		n_damage = pev->dmg;
	}

	ALERT(at_console, "damage: %f\n", n_damage);
	ALERT(at_console, "length: %f\n", n_dist);

	if (0 != pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t* pevOwner;

		pevOwner = VARS(pev->owner);

		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, n_damage, pev->velocity.Normalize(), &tr, DMG_NEVERGIB, true);
		ApplyMultiDamage(pev, pevOwner);

		// play body "thwack" sound
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM);
			break;
		}

		if (!g_pGameRules->IsMultiplayer())
		{
			Killed(pev, GIB_NEVER);
		}
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));

		SetThink(&CNail::SUB_Remove);
		pev->nextthink = gpGlobals->time; // this will get changed below if the bolt is allowed to stick in what it hit.

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 12);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}
}

void CNail::BubbleThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

#endif

LINK_ENTITY_TO_CLASS(weapon_nailgun, CNailgun);

//=========================================================
//=========================================================
void CNailgun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_nailgun"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_nailgun.mdl");
	m_iId = WEAPON_NAILGUN;

	m_iDefaultAmmo = NAIL_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}

void CNailgun::Precache()
{
	PRECACHE_MODEL("models/v_nailgun.mdl");
	PRECACHE_MODEL("models/w_nailgun.mdl");
	PRECACHE_MODEL("models/p_nailgun.mdl");
	PRECACHE_MODEL("models/nail.mdl");

	PRECACHE_MODEL("models/w_nailclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/nailgun.wav"); // H to the K

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usNailgun = PRECACHE_EVENT(1, "events/nailgun.sc");
}

bool CNailgun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "nails";
	p->iMaxAmmo1 = NAIL_MAX_CARRY;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = NAILGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_NAILGUN;
	p->iWeight = NAILGUN_WEIGHT;

	return true;
}

void CNailgun::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "nails", NAIL_MAX_CARRY) >= 0)
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

bool CNailgun::Deploy()
{
	return DefaultDeploy("models/v_nailgun.mdl", "models/p_nailgun.mdl", NAILGUN_DEPLOY, "nailgun");
	m_bIsAuto = false;
}


void CNailgun::PrimaryAttack()
{
	TraceResult tr;

	m_bIsAuto = false;

	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	m_iClip--;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = UTIL_DefaultPlaybackFlags();
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usNailgun, 0.0, g_vecZero, g_vecZero, 0, 0, m_iClip, 0, 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CNail* pNail = CNail::NailCreate();
	pNail->pev->origin = vecSrc;
	pNail->n_startpos = vecSrc;
	pNail->pev->angles = anglesAim;
	pNail->pev->owner = m_pPlayer->edict();
	pNail->pev->dmg = gSkillData.plrDmgNail;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pNail->pev->velocity = vecDir * NAIL_WATER_VELOCITY;
		pNail->pev->speed = NAIL_WATER_VELOCITY;
	}
	else
	{
		pNail->pev->velocity = vecDir * NAIL_AIR_VELOCITY;
		pNail->pev->speed = NAIL_AIR_VELOCITY;
	}
	pNail->pev->avelocity.z = 10;
#endif

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.75);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.39;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
}

void CNailgun::SecondaryAttack()
{
	TraceResult tr;

	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}

	m_bIsAuto = true;

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	m_iClip--;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = UTIL_DefaultPlaybackFlags();
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usNailgun, 0.0, g_vecZero, g_vecZero, 0, 0, m_iClip, 0, 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CNail* pNail = CNail::NailCreate();
	pNail->pev->origin = vecSrc;
	pNail->n_startpos = vecSrc;
	pNail->pev->angles = anglesAim;
	pNail->pev->owner = m_pPlayer->edict();
	pNail->pev->dmg = gSkillData.plrDmgNail/2.5;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pNail->pev->velocity = vecDir * NAIL_WATER_VELOCITY;
		pNail->pev->speed = NAIL_WATER_VELOCITY;
	}
	else
	{
		pNail->pev->velocity = vecDir * NAIL_AIR_VELOCITY;
		pNail->pev->speed = NAIL_AIR_VELOCITY;
	}
	pNail->pev->avelocity.z = 10;
#endif

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.39;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
}

void CNailgun::Reload()
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	if (m_iClip <= 0)
	{
		DefaultReload(NAILGUN_MAX_CLIP, NAILGUN_RELOADEMPTY, 2.66f);
	}
	else
	{
		DefaultReload(NAILGUN_MAX_CLIP, NAILGUN_RELOAD, 1.4f);
	}
}

void CNailgun::WeaponIdle()
{
	ResetEmptySound();

	m_bIsAuto = false;

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		iAnim = NAILGUN_IDLE1;
		break;

	case 1:
		iAnim = NAILGUN_IDLE2;
		break;
	case 2:
		iAnim = NAILGUN_IDLE3;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}

class CNailgunClip : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_nailclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_nailclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_NAILGUN_GIVE, "nails", NAIL_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_nailgunclip, CNailgunClip);
LINK_ENTITY_TO_CLASS(ammo_nails, CNailgunClip);