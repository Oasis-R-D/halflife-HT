#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "RC.h"

LINK_ENTITY_TO_CLASS(weapon_rc, CRCWeapon);

//=========================================================
//=========================================================
void CRCWeapon::Spawn()
{
	pev->classname = MAKE_STRING("weapon_rc"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_sqknest.mdl");
	m_iId = WEAPON_RC;

	m_iDefaultAmmo = 1; // to-do: constant

	FallInit(); // get ready to fall down.
}


void CRCWeapon::Precache()
{
	PRECACHE_MODEL("models/w_sqknest.mdl");
	PRECACHE_MODEL("models/v_squeak.mdl");
	PRECACHE_MODEL("models/p_squeak.mdl");
	PRECACHE_SOUND("squeek/sqk_hunt2.wav");
	PRECACHE_SOUND("squeek/sqk_hunt3.wav");
	UTIL_PrecacheOther("pl_rc");
}

bool CRCWeapon::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "rcs";
	p->iMaxAmmo1 = 3; // TO-DO: constant
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_RC;
	p->iWeight = SNARK_WEIGHT; // share
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return true;
}

bool CRCWeapon::Deploy()
{
	const bool result = DefaultDeploy("models/v_squeak.mdl", "models/p_squeak.mdl", SQUEAK_UP, "squeak");

	if (result)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.7;
	}

	return result;
}

void CRCWeapon::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (0 == m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->ClearWeaponBit(m_iId);
		SetThink(&CRCWeapon::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	SendWeaponAnim(SQUEAK_DOWN);
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CRCWeapon::PrimaryAttack()
{
	Throw(RC_EXPLODE);
}

void CRCWeapon::SecondaryAttack()
{
	Throw(RC_GUN); // TO-DO: should probably give the gun turret limited ammo
}

void CRCWeapon::Throw(int type)
{
	if (m_pPlayer->m_pRCcar != NULL) // shouldn't happen
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.15;
		return;
	}

	if (0 >= m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.15;
		return;
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	TraceResult tr;
	Vector trace_origin;

	// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
	// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
	trace_origin = m_pPlayer->pev->origin;
	if ((m_pPlayer->pev->flags & FL_DUCKING) != 0)
	{
		trace_origin = trace_origin - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
	}

	// find place to toss monster
	UTIL_TraceLine(trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * 64, dont_ignore_monsters, NULL, &tr);

	int flags;
#ifdef CLIENT_WEAPONS
	flags = UTIL_DefaultPlaybackFlags();
#else
	flags = 0;
#endif

	if (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25)
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
		CRC* RC = CRC::RC_Create(100, tr.vecEndPos, Vector(0, pev->angles.y, 0), type);
		RC->Use(m_pPlayer, m_pPlayer, USE_SET, 1); // start controlling
		RC->pev->velocity = gpGlobals->v_forward * 200 + m_pPlayer->pev->velocity;
#endif

		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_fJustThrown = true;

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.3);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	}
}

void CRCWeapon::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = false;

		if (0 == m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()])
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim(SQUEAK_UP);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.75)
	{
		iAnim = SQUEAK_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = SQUEAK_FIDGETFIT;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 16.0;
	}
	else
	{
		iAnim = SQUEAK_FIDGETNIP;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 16.0;
	}
	SendWeaponAnim(iAnim);
}