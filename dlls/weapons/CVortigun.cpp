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
#include "skill.h"
#include "gamerules.h"

#endif

#include "CVortigun.h"

LINK_ENTITY_TO_CLASS(weapon_displacer, CVortigun);
LINK_ENTITY_TO_CLASS(weapon_vortigun, CVortigun);

void CVortigun::Precache()
{
	PRECACHE_MODEL("models/v_vortigun.mdl");
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

void CVortigun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_displacer"); // Hack to allow for old weapon names

	Precache();

	m_iId = WEAPON_VORTIGUN;

	SET_MODEL(edict(), "models/w_displacer.mdl");

	m_iDefaultAmmo = DISPLACER_DEFAULT_GIVE;

	FallInit();
}

bool CVortigun::Deploy()
{
	return DefaultDeploy("models/v_vortigun.mdl", "models/p_displacer.mdl", DISPLACER_DRAW, "egon");
}

void CVortigun::Holster()
{
	ClearBeams();

	m_fInReload = false;

	STOP_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin.wav");

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 5.0);

	SendWeaponAnim(DISPLACER_HOLSTER1);

	if (m_pfnThink == &CVortigun::SpinupThink)
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 20;
		SetThink(nullptr);
	}
}

void CVortigun::WeaponIdle()
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

void CVortigun::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= 20)
	{
		SetThink(&CVortigun::SpinupThink);

		pev->nextthink = gpGlobals->time;

		m_Mode = DisplacerMode::STARTED;

		//EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_spin.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

		m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.5;
	}
	else
	{
		EMIT_SOUND(m_pPlayer->edict(), CHAN_WEAPON, "buttons/button11.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CVortigun::Reload()
{
	//Nothing
}

void CVortigun::SpinupThink()
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

		Vector vecSrc = m_pPlayer->GetGunPosition();
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(vecSrc.x);			 // X
		WRITE_COORD(vecSrc.y);			 // Y
		WRITE_COORD(vecSrc.z);			 // Z
		WRITE_BYTE(12);					 // radius * 0.1
		WRITE_BYTE(255);				 // r
		WRITE_BYTE(180);				 // g
		WRITE_BYTE(96);					 // b
		WRITE_BYTE(10); 				 // time * 10
		WRITE_BYTE(0);					 // decay * 0.1
		MESSAGE_END();

		//PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireDisplacer, 0, g_vecZero, g_vecZero, 0, 0, static_cast<int>(m_Mode), 0, 0, 0);

		m_flStartTime = gpGlobals->time;
		m_iSoundState = 0;
	}

	// recreates the anim events
	switch (m_iSoundState)
	{
		default:
		break;
		case 0:
		case 3: // not 0.2666!!
		case 8:
		case 12:
		{
			ArmBeam();
			ArmBeam();
			BeamGlow();
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10);
		}
		break;
		case 17: // not 1.4!!
		{
			m_Mode = DisplacerMode::SPINNING;

			SetThink(&CVortigun::FireThink);

			pev->nextthink = gpGlobals->time + 0.1;
		}
		break;
	}
	
	m_iSoundState++;

	pev->nextthink = gpGlobals->time + 0.083333;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CVortigun::ZapBeam()
{
#ifndef CLIENT_DLL
	Vector vecOrg, vecAim;
	TraceResult tr;
	CBaseEntity* pEntity;;

	vecOrg = m_pPlayer->GetGunPosition();
	vecAim = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	UTIL_TraceLine(vecOrg, vecOrg + vecAim * 2048, dont_ignore_monsters, dont_ignore_glass, ENT(m_pPlayer->pev), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 50);
		m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, m_pPlayer->entindex());
		m_pBeam[m_iBeams]->SetEndAttachment(1);
		m_pBeam[m_iBeams]->SetColor(180, 255, 96);
		m_pBeam[m_iBeams]->SetBrightness(255);
		m_pBeam[m_iBeams]->SetNoise(20);
		m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL)
	{
		pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmg762, vecAim, &tr, DMG_SHOCK); // TO-DO: add separate skill value

		TraceResult tr2 = tr;
		CBaseEntity* pTrack = NULL;
		CBaseEntity* pNearest = NULL;
		CBaseEntity* pHit[ISLAVE_MAX_BEAMS];
		int hitAmnt = 0;

		pHit[hitAmnt] = pEntity;

		vecOrg = tr.vecEndPos;
		// only can bounce around ISLAVE_MAX_BEAMS times
		for (int i = 2; i < ISLAVE_MAX_BEAMS; i++)
		{
			float dist, closest;

			closest = 768;

			while ((pTrack = UTIL_FindEntityInSphere(pTrack, pev->origin, 768)) != NULL)
			{
				// only bounce to living things
				if ((pTrack->pev->flags & (FL_CLIENT | FL_MONSTER)) == 0)
					continue;

				// only hit visible enemies
				if (!pTrack->FVisible(vecOrg))
					continue;

				// dont hit an enemy twice
				bool cont = false;
				for (const auto& ent : pHit)
				{
					if (ent == pTrack)
						cont = true;
				}

				if (cont)
					continue;

				// don't hit firer
				if (pTrack == m_pPlayer)
					continue;

				dist = (pev->origin - pTrack->pev->origin).Length();
				if (dist < closest)
				{
					closest = dist;
					pNearest = pTrack;
				}
			}

			if (!pNearest)
				break;

			hitAmnt++;
			pHit[hitAmnt] = pNearest;


			UTIL_TraceLine(vecOrg, pNearest->EyePosition(), dont_ignore_monsters, dont_ignore_glass, ENT(pHit[hitAmnt-1]->pev), &tr2);
			m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 50);
				m_pBeam[m_iBeams]->PointsInit(vecOrg, tr2.vecEndPos);
				m_pBeam[m_iBeams]->SetColor(180, 255, 96);
				m_pBeam[m_iBeams]->SetBrightness(255);
				m_pBeam[m_iBeams]->SetNoise(20);
				m_pBeam[m_iBeams]->pev->spawnflags |= SF_BEAM_TEMPORARY; // Flag these to be destroyed on save/restore or level transition
			m_iBeams++;

			pNearest->TraceAttack(m_pPlayer->pev, gSkillData.plrDmg762, (vecOrg - tr2.vecEndPos).Normalize(), &tr2, DMG_SHOCK); // TO-DO: add separate skill value
			
			vecOrg = tr2.vecEndPos;

			// reset these to prevent hitting same enemy multiple times
			pTrack = NULL;
			pNearest = NULL;
		}
	}
	UTIL_EmitAmbientSound(ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));

	// TO-DO: custom decal?
	DecalGunshot(&tr, BULLET_PLAYER_GLOCK);
#endif
}

void CVortigun::FireThink()
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

	ZapBeam();

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));
	// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
	ApplyMultiDamage(pev, pev);

	#pragma endregion

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0)
	{
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK);
	}
#endif
	pev->nextthink = gpGlobals->time + 0.2;
	SetThink(&CVortigun::ClearThink);
}

void CVortigun::ClearThink()
{
	ClearBeams();
	SetThink(nullptr);
}

int CVortigun::iItemSlot()
{
	return 4;
}

bool CVortigun::GetItemInfo(ItemInfo* p)
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
	p->iId = m_iId = WEAPON_VORTIGUN;
	p->iWeight = DISPLACER_WEIGHT;
	return true;
}

void CVortigun::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "uranium", URANIUM_MAX_CARRY) >= 0)
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CVortigun::ArmBeam()
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
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev),	 &tr1);
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

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, m_pPlayer->entindex());
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
void CVortigun::ClearBeams()
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
void CVortigun::BeamGlow()
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