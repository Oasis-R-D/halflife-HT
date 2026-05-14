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
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "skill.h"

#include "CSniperRifle.h"

#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS(sniper_laser, CSniperLaser);
//=========================================================
//=========================================================
CSniperLaser* CSniperLaser::CreateSpot()
{
	auto pSpot = GetClassPtr(reinterpret_cast<CSniperLaser*>(VARS(CREATE_NAMED_ENTITY(MAKE_STRING("sniper_laser")))));
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("eagle_laser");

	return pSpot;
}

TYPEDESCRIPTION CSniperRifle::m_SaveData[] =
	{
		DEFINE_FIELD(CSniperRifle, m_flReloadStart, FIELD_TIME),
		DEFINE_FIELD(CSniperRifle, m_bReloading, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CSniperRifle, CSniperRifle::BaseClass);
#endif

LINK_ENTITY_TO_CLASS(weapon_sniperrifle, CSniperRifle);

void CSniperRifle::Precache()
{
	pev->classname = MAKE_STRING("weapon_sniperrifle");

	BaseClass::Precache();

	m_iId = WEAPON_SNIPERRIFLE;

	PRECACHE_MODEL("models/w_m40a1.mdl");
	PRECACHE_MODEL("models/v_sniper.mdl");
	PRECACHE_MODEL("models/p_m40a1.mdl");

	PRECACHE_SOUND("weapons/sniper_fire.wav");
	PRECACHE_SOUND("weapons/sniper_zoom.wav");
	PRECACHE_SOUND("weapons/sniper_reload_first_seq.wav");
	PRECACHE_SOUND("weapons/sniper_reload_second_seq.wav");
	PRECACHE_SOUND("weapons/sniper_miss.wav");
	PRECACHE_SOUND("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND("weapons/sniper_bolt2.wav");

	m_usSniper = PRECACHE_EVENT(1, "events/sniper.sc");
}

void CSniperRifle::Spawn()
{
	Precache();

	SET_MODEL(edict(), "models/w_m40a1.mdl");

	m_iDefaultAmmo = SNIPERRIFLE_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}

bool CSniperRifle::Deploy()
{
	m_bSpotVisible = true;
	m_flChargeTime = -1;
	return BaseClass::DefaultDeploy("models/v_sniper.mdl", "models/p_m40a1.mdl", TFCRIFLE_DRAW, "bow");
}

void CSniperRifle::Holster()
{
	m_bSpotVisible = false;
	m_flChargeTime = -1;
	m_fInReload = false; // cancel any reload in progress.

	if (m_pPlayer->m_iFOV != 0)
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.25;

	SendWeaponAnim(TFCRIFLE_HOLSTER);
}

void CSniperRifle::WeaponIdle()
{
	//Update autoaim
	m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);

	ResetEmptySound();

	/*
	if (m_bReloading && gpGlobals->time >= m_flReloadStart + 2.324)
	{
		SendWeaponAnim(SNIPERRIFLE_RELOAD2);
		m_bReloading = false;
	}
	*/

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		SendWeaponAnim(TFCRIFLE_IDLE);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.10;
	}
}

bool CanAttackSniper(float attack_time, float curtime, bool isPredicted)
{
#if defined(CLIENT_WEAPONS)
	if (!isPredicted)
#else
	if (1)
#endif
	{
		return (attack_time <= curtime) ? true : false;
	}
	else
	{
		return ((static_cast<int>(std::floor(attack_time * 1000.0)) * 1000.0) <= 0.0) ? true : false;
	}
}

void CSniperRifle::ItemPostFrame()
{
	if (m_flChargeTime < -0.1)
	{
		if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && CanAttackSniper(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
		{
			if ((m_iClip > 0 && m_pPlayer->pev->waterlevel != WATERLEVEL_HEAD))
			{
				SendWeaponAnim(TFCRIFLE_AIM);
				m_flChargeTime = 0;
				return;
			}
		}
	}
	else if (m_flChargeTime >= 0)
	{
		m_flChargeTime += gpGlobals->frametime;

		// reset
		if (m_pPlayer->pev->waterlevel == WATERLEVEL_HEAD)
		{
			m_bLaserActive = false;
			if (m_pLaser)
			{
				m_pLaser->Killed(nullptr, GIB_NEVER);

				m_pLaser = nullptr;

				EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/desert_eagle_sight2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			}

			m_flChargeTime = -1;
			return;
		}

		// Only allow shooting after 0.1 seconds!
		if (m_flChargeTime > 0.1)
		{
			ALERT(at_console, "3\n");
			m_bLaserActive = true;

#ifndef CLIENT_DLL
			UpdateLaser();
#endif

			// SHOOT
			if ((m_pPlayer->pev->button & IN_ATTACK) == 0)
			{
				if (m_iClip > 0)
				{
					Shoot(m_flChargeTime);
				}
				
				m_bLaserActive = false;
				if (m_pLaser)
				{
					m_pLaser->Killed(nullptr, GIB_NEVER);

					m_pLaser = nullptr;

					EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/desert_eagle_sight2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
				}
				m_flChargeTime = -1;
			}
		}
	}
	CBasePlayerWeapon::ItemPostFrame();
	ALERT(at_console, "CHARGE: %f\n", m_flChargeTime);
}

void CSniperRifle::Shoot(double time)
{
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	--m_iClip;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

	UTIL_MakeVectors(vecAngles);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);

	//TODO: 8192 constant should be defined somewhere - Solokiller
	double charge = time;
	if (charge > 5) // don't let it get too high
		charge = 5;

	float damage = pow(1.6 * charge, 3) + gSkillData.plrDmg762;

	Vector vecShot = m_pPlayer->FireBulletsPlayer(1,
		vecSrc, vecAiming, g_vecZero,
		8192, BULLET_PLAYER_223, 0, damage,
		m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(UTIL_DefaultPlaybackFlags(),
		m_pPlayer->edict(), m_usSniper, 0,
		g_vecZero, g_vecZero,
		vecShot.x, vecShot.y,
		m_iClip, m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()],
		0, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
}

void CSniperRifle::SecondaryAttack()
{
	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_ITEM, "weapons/sniper_zoom.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	ToggleZoom();

	//TODO: this doesn't really make sense
	pev->nextthink = 0.1;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CSniperRifle::Reload()
{
	if (m_pPlayer->ammo_223 > 0)
	{
		if (m_pPlayer->m_iFOV != 0)
		{
			ToggleZoom();
		}
		/*
		if (0 != m_iClip)
		{ // tac reload
			if (DefaultReload(SNIPERRIFLE_MAX_CLIP, SNIPERRIFLE_RELOAD3, 2.324))
			{
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.324;
			}
		}
		else if (DefaultReload(SNIPERRIFLE_MAX_CLIP, SNIPERRIFLE_RELOAD1, 2.324))
		{ // empty reload
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 4.102;
			m_flReloadStart = gpGlobals->time;
			m_bReloading = true;
		}
		else
		{
			m_bReloading = false;
		}
		*/

		// PLACEHOLDER SINCE TFC HAS NO RELOADS
		if (DefaultReload(SNIPERRIFLE_MAX_CLIP, TFCRIFLE_DRAW, 2))
		{ // empty reload
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 4.102;
			m_flReloadStart = gpGlobals->time;
			m_bReloading = true;
		}
		else
		{
			m_bReloading = false;
		}
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.102;
}

int CSniperRifle::iItemSlot()
{
	return 4;
}

bool CSniperRifle::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "223";
	p->iMaxAmmo1 = SNIPERRIFLE_MAX_CARRY;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = 0;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = SNIPERRIFLE_MAX_CLIP;
	p->iSlot = 5;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SNIPERRIFLE;
	p->iWeight = SNIPERRIFLE_WEIGHT;
	return true;
}

void CSniperRifle::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "223", SNIPERRIFLE_MAX_CARRY) >= 0)
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

void CSniperRifle::ToggleZoom()
{
	if (m_pPlayer->m_iFOV == 0)
	{
		m_pPlayer->m_iFOV = 18;
	}
	else
	{
		m_pPlayer->m_iFOV = 0;
	}
}

void CSniperRifle::UpdateLaser()
{
#ifndef CLIENT_DLL
	// Don't turn on the laser if we're in the middle of a reload.
	if (m_fInReload)
	{
		return;
	}

	if (m_bLaserActive && m_bSpotVisible)
	{
		if (!m_pLaser)
		{
			m_pLaser = CSniperLaser::CreateSpot();

			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/desert_eagle_sight.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		}

		m_pLaser->pev->renderamt = 25.4 * m_flChargeTime + 128;

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);

		Vector vecSrc = m_pPlayer->GetGunPosition();

		Vector vecEnd = vecSrc + gpGlobals->v_forward * 8192.0;

		TraceResult tr;

		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr);

		UTIL_SetOrigin(m_pLaser->pev, tr.vecEndPos);
	}
#endif
}

class CSniperRifleAmmo : public CBasePlayerAmmo
{
public:
	using BaseClass = CBasePlayerAmmo;

	void Spawn() override
	{
		Precache();
		SET_MODEL(edict(), "models/w_m40a1clip.mdl");
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL("models/w_m40a1clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}

	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(AMMO_SNIPERRIFLE_GIVE, "223", SNIPERRIFLE_MAX_CARRY) != -1)
		{
			EMIT_SOUND(edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);

			return true;
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS(ammo_223, CSniperRifleAmmo);
