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

#pragma once

enum ac_e
{
	AC_IDLE1 = 0,
	AC_IDLE2 = 1,
	AC_SPINUP = 2,
	AC_SPINDOWN = 3,
	AC_FIRE = 4,
	AC_DEPLOY = 5,
	AC_HOLSTER = 6
};

class CM249 : public CBasePlayerWeapon
{
public:
	using BaseClass = CBasePlayerWeapon;

#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];
#endif

public:
	void Spawn();
	void Precache();
	bool GetItemInfo(ItemInfo *p);
	bool Deploy();
	void Holster() override;
	void WeaponIdle() override;
	void Fire();
	void PrimaryAttack() override;
	void StartSpin();
	void Spin();
	void WindUp();
	void WindDown(bool bFromHolster);
	int iItemSlot() { return 4; };
	bool ShouldWeaponIdle() { return true; };

	void IncrementAmmo(CBasePlayer* pPlayer) override;

	bool UseDecrement() override
	{
#if defined(CLIENT_WEAPONS)
		return UTIL_DefaultUseDecrement();
#else
		return false;
#endif
	}

private:
	unsigned short m_usWindUp;
	unsigned short m_usWindDown;
	unsigned short m_usFire;
	unsigned short m_usStartSpin;
	unsigned short m_usSpin;
	unsigned short m_usACStart;
	unsigned short m_usFireM249;

	int m_iShell;
};
