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
#include "gamerules.h"
#include "UserMessages.h"

#include "CM249.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CM249::m_SaveData[] =
	{
		DEFINE_FIELD(CM249, m_iShell, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CM249, CM249::BaseClass);
#endif

LINK_ENTITY_TO_CLASS(weapon_m249, CM249);

void CM249::Precache()
{
	PRECACHE_MODEL( "models/v_tfac.mdl" );
	PRECACHE_MODEL( "models/w_saw.mdl" );
	PRECACHE_MODEL( "models/p_mini.mdl" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/asscan1.wav" );
	PRECACHE_SOUND( "weapons/asscan2.wav" );
	PRECACHE_SOUND( "weapons/asscan3.wav" );
	PRECACHE_SOUND( "weapons/asscan4.wav" );
	m_iShell =		PRECACHE_MODEL( "models/shell.mdl" );
	m_usWindUp =	PRECACHE_EVENT( 1, "events/wpn/tf_acwu.sc" );
	m_usWindDown =	PRECACHE_EVENT( 1, "events/wpn/tf_acwd.sc" );
	m_usFire =		PRECACHE_EVENT( 1, "events/wpn/tf_acfire.sc" );
	m_usStartSpin = PRECACHE_EVENT( 1, "events/wpn/tf_acsspin.sc" );
	m_usSpin =		PRECACHE_EVENT( 1, "events/wpn/tf_acspin.sc" );
	m_usACStart =	PRECACHE_EVENT( 1, "events/wpn/tf_acstart.sc" );
}

void CM249::Spawn()
{
	pev->classname = MAKE_STRING("weapon_m249");

	Precache();

	m_iId = WEAPON_M249;

	SET_MODEL(edict(), "models/w_saw.mdl");

	m_iDefaultAmmo = M249_DEFAULT_GIVE;

	m_iWeaponState = 0;

	FallInit(); // get ready to fall down.
}

bool CM249::Deploy()
{
	return DefaultDeploy( "models/v_tfac.mdl", "models/p_mini.mdl", AC_DEPLOY, "ac", 1 );
}

void CM249::Holster()
{
	WindDown( true );
	m_fInReload = false;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1f;
	SendWeaponAnim( AC_HOLSTER );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10.0, 15.0);
}

void CM249::WeaponIdle()
{
	ResetEmptySound();

	//Update auto-aim
	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		if ( m_iWeaponState )
		{
			WindDown( false );
		}
		else
		{
			SendWeaponAnim( UTIL_SharedRandomLong( m_pPlayer->random_seed, AC_IDLE1, AC_IDLE2 ) );
			m_flTimeWeaponIdle = 12.5f;
		}
	}
}

void CM249::Fire()
{
	if ( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() )
	{
		Vector p_vecSrc, p_VecDirShooting, p_vecSpread;

		PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_UPDATE, m_pPlayer->edict(), m_usFire, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] & 1, 0 );
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		p_vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_up * -4.0f + gpGlobals->v_right * 2.0f;
		p_VecDirShooting = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
		p_vecSpread = Vector( 0.1f, 0.1f, 0.0f );
		//m_pPlayer->FireBullets( 5, p_vecSrc, p_VecDirShooting, p_vecSpread, 8192.0f, BULLET_PLAYER_TF_ASSAULT, 8, 7, NULL );
		m_pPlayer->FireBullets( 5, p_vecSrc, p_VecDirShooting, p_vecSpread, 8192.0f, BULLET_PLAYER_M249, 8, 7, NULL );
		//DB_LogShots( 1 );
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	}
}

void CM249::PrimaryAttack()
{
	switch ( m_iWeaponState )
	{
	case 1:
		if ( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() )
		{
			PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usACStart, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );
			m_iWeaponState = 2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
		}
		return;
	case 2:
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		{
			StartSpin();
		}
		else
		{
			Fire();
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
		return;
	case 3:
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		{
			Spin();
		}
		else
		{
			m_iWeaponState = 1;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
		return;
	default:
		break;
	}

	if ( (m_pPlayer->pev->button & IN_ATTACK) != 0 )
	{
		WindUp();
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6f;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CM249::StartSpin( void )
{
	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usStartSpin, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );
	m_iWeaponState = 3;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pev->effects &= ~EF_MUZZLEFLASH;
}

void CM249::Spin( void )
{
	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usSpin, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

void CM249::WindUp( void )
{
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usWindUp, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0 );
	m_iWeaponState = 1;
	//m_pPlayer->tfstate |= TFSTATE_AIMING;
	//m_pPlayer->TeamFortress_SetSpeed();
}

void CM249::WindDown( bool bFromHolster )
{
	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usWindDown, 0.0f, g_vecZero, g_vecZero, 0.0f, 0.0f, 0, 0, bFromHolster, 0 );
	m_iWeaponState = 0;
	//m_pPlayer->tfstate &= ~TFSTATE_AIMING;
	//m_pPlayer->TeamFortress_SetSpeed();
	m_flTimeWeaponIdle = 2.0f;
}

bool CM249::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = M249_MAX_CARRY;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M249;
	p->iWeight = M249_WEIGHT;

	return true;
}

void CM249::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "556", M249_MAX_CARRY) >= 0)
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

class CAmmo556 : public CBasePlayerAmmo
{
public:
	using BaseClass = CBasePlayerAmmo;

	void Precache() override
	{
		PRECACHE_MODEL("models/w_saw_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}

	void Spawn() override
	{
		Precache();

		SET_MODEL(edict(), "models/w_saw_clip.mdl");

		BaseClass::Spawn();
	}

	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(AMMO_M249_GIVE, "556", M249_MAX_CARRY) != -1)
		{
			EMIT_SOUND(edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);

			return true;
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, CAmmo556);
