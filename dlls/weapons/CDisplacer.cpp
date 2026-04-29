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
#include "weapons.h"
#include "player.h"
#include "UserMessages.h"

#ifndef CLIENT_DLL
#include "effects.h"
#include "rope/CRope.h"
#include "ctf/CTFGoal.h"
#include "ctf/CTFGoalFlag.h"

#include "gamerules.h"

extern edict_t* EntSelectSpawnPoint(CBasePlayer* pPlayer);

extern CBaseEntity* g_pLastSpawn;
#endif

#include "CDisplacer.h"

LINK_ENTITY_TO_CLASS(weapon_displacer, CDisplacer);

void CDisplacer::Precache()
{
	PRECACHE_MODEL("models/v_displacer.mdl");
	PRECACHE_MODEL("models/w_displacer.mdl");
	PRECACHE_MODEL("models/p_displacer.mdl");

	PRECACHE_SOUND("weapons/displacer_fire.wav");
	PRECACHE_SOUND("weapons/displacer_self.wav");
	PRECACHE_SOUND("weapons/displacer_spin.wav");
	PRECACHE_SOUND("weapons/displacer_spin2.wav");

	PRECACHE_SOUND("buttons/button11.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");

	m_usFireDisplacer = PRECACHE_EVENT(1, "events/displacer.sc");
}

void CDisplacer::Spawn()
{
	pev->classname = MAKE_STRING("weapon_displacer");

	Precache();

	m_iId = WEAPON_DISPLACER;

	SET_MODEL(edict(), "models/w_displacer.mdl");

	m_iDefaultAmmo = DISPLACER_DEFAULT_GIVE;

	FallInit();
}

bool CDisplacer::Deploy()
{
	return DefaultDeploy("models/v_displacer.mdl", "models/p_displacer.mdl", DISPLACER_DRAW, "egon");
}

void CDisplacer::Holster()
{
	ClearBeams();

	m_fInReload = false;

	STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin.wav");

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 5.0);

	SendWeaponAnim(DISPLACER_HOLSTER1);

	if (m_pfnThink == &CDisplacer::SpinupThink)
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 20;
		SetThink(nullptr);
	}
}

void CDisplacer::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flSoundDelay != 0 && gpGlobals->time >= m_flSoundDelay)
		m_flSoundDelay = 0;

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		const float flNextIdle = RANDOM_FLOAT(0, 1);

		int iAnim;

		if (flNextIdle <= 0.5)
		{
			iAnim = DISPLACER_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		}
		else
		{
			iAnim = DISPLACER_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		}

		SendWeaponAnim(iAnim);
	}
}

void CDisplacer::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= 20)
	{
		SetThink(&CDisplacer::SpinupThink);

		pev->nextthink = gpGlobals->time;

		m_Mode = DisplacerMode::STARTED;

		EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

		m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.5;
	}
	else
	{
		EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "buttons/button11.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CDisplacer::Reload()
{
	//Nothing
}

void CDisplacer::SpinupThink()
{
	if (m_Mode == DisplacerMode::STARTED)
	{
		SendWeaponAnim(DISPLACER_SPINUP);

		m_Mode = DisplacerMode::SPINNING_UP;

		int flags;

		//#if defined( CLIENT_WEAPONS )
		//		flags = FEV_NOTHOST;
		//#else
		flags = 0;
		//#endif

		ArmBeam();
		BeamGlow();

		//PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireDisplacer, 0, g_vecZero, g_vecZero, 0, 0, static_cast<int>(m_Mode), 0, 0, 0);

		m_flStartTime = gpGlobals->time;
		m_iSoundState = 0;
	}

	if (m_Mode <= DisplacerMode::SPINNING_UP)
	{
		if (gpGlobals->time > m_flStartTime + 0.9)
		{
			m_Mode = DisplacerMode::SPINNING;

			SetThink(&CDisplacer::FireThink);

			pev->nextthink = gpGlobals->time + 0.1;
		}
	}

	m_iSoundState = 128;

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CDisplacer::ZapBeam(Vector vecOrg)
{
#ifndef CLIENT_DLL
	Vector vecAim;
	TraceResult tr;
	CBaseEntity* pEntity;

	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	vecOrg = m_pPlayer->pev->origin + gpGlobals->v_up * 36;
	float deflection = 0.01;
	vecAim = gpGlobals->v_forward + gpGlobals->v_right * RANDOM_FLOAT(-deflection, deflection) + gpGlobals->v_up * RANDOM_FLOAT(-deflection, deflection);
	UTIL_TraceLine(vecOrg, vecOrg + vecAim * 1024, dont_ignore_monsters, ENT(pev), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 50);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(1);
	m_pBeam[m_iBeams]->SetColor(180, 255, 96);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(20);
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && 0 != pEntity->pev->takedamage)
	{
		pEntity->TraceAttack(m_pPlayer->pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK);
	}
	UTIL_EmitAmbientSound(ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));
#endif
}

void CDisplacer::FireThink()
{
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 20;

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	SendWeaponAnim(DISPLACER_FIRE);

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	int flags;

#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	/*
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireDisplacer, 0,
		g_vecZero, g_vecZero, 0, 0, static_cast<int>( DisplacerMode::FIRED ), 0, 0, 0 );
		*/

#ifndef CLIENT_DLL
	const Vector vecAnglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

	UTIL_MakeVectors(vecAnglesAim);

	Vector vecSrc = m_pPlayer->GetGunPosition();

	//Update auto-aim
	m_pPlayer->GetAutoaimVectorFromPoint(vecSrc, AUTOAIM_10DEGREES);

	#pragma region firing
	ClearBeams();
	ClearMultiDamage();

	ZapBeam(vecSrc);

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));
	// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
	ApplyMultiDamage(pev, pev);

	#pragma endregion

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0)
	{
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK);
	}
#endif

	SetThink(&CDisplacer::ClearThink);

	pev->nextthink = gpGlobals->time + 0.1;
}

void CDisplacer::ClearThink()
{
	ClearBeams();
	SetThink(nullptr);
}

int CDisplacer::iItemSlot()
{
	return 4;
}

bool CDisplacer::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iFlags = 0;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_DISPLACER;
	p->iWeight = DISPLACER_WEIGHT;
	return true;
}

void CDisplacer::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "uranium", URANIUM_MAX_CARRY) >= 0)
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CDisplacer::ArmBeam()
{
#ifndef CLIENT_DLL
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors(m_pPlayer->pev->angles);
	Vector vecSrc = m_pPlayer->pev->origin + gpGlobals->v_up * 48 + gpGlobals->v_right * 16 + gpGlobals->v_forward * 32; // TO-DO: use view org

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * RANDOM_FLOAT(-1, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(1);
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor(96, 128, 16);
	m_pBeam[m_iBeams]->SetBrightness(64);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;
#endif
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CDisplacer::ClearBeams()
{
#ifndef CLIENT_DLL
	for (int i = 0; i < ISLAVE_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND(ENT(pev), CHAN_WEAPON, "debris/zap4.wav");
#endif
}

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CDisplacer::BeamGlow()
{
#ifndef CLIENT_DLL
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255)
		{
			m_pBeam[i]->SetBrightness(b);
		}
	}
#endif
}