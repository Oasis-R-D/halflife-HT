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

enum TFCRifleAnim
{
	TFCRIFLE_IDLE = 0,
	TFCRIFLE_AIM,
	TFCRIFLE_FIRE,
	TFCRIFLE_DRAW,
	TFCRIFLE_HOLSTER,
};

enum SniperRifleAnim
{
	SNIPERRIFLE_DRAW = 0,
	SNIPERRIFLE_SLOWIDLE,
	SNIPERRIFLE_FIRE,
	SNIPERRIFLE_FIRELASTROUND,
	SNIPERRIFLE_RELOAD1,
	SNIPERRIFLE_RELOAD2,
	SNIPERRIFLE_RELOAD3,
	SNIPERRIFLE_SLOWIDLE2,
	SNIPERRIFLE_HOLSTER
};

/**
*	@brief Identical to CLaserSpot, different class to avoid RPG laser confusion logic. - Solokiller
*/
class CSniperLaser : public CLaserSpot
{
public:
	using BaseClass = CLaserSpot;

	static CSniperLaser* CreateSpot();
};

class CEagleLaser;

/**
*	@brief Opposing force sniper rifle
*/
class CSniperRifle : public CBasePlayerWeapon
{
public:
	using BaseClass = CBasePlayerWeapon;

#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Precache() override;
	void Spawn() override;

	bool Deploy() override;

	void Holster() override;

	void ItemPostFrame() override;
	void WeaponIdle() override;

	void Shoot(double time);
	void Shoot2();

	void SecondaryAttack() override;

	void Reload() override;

	int iItemSlot() override;

	bool GetItemInfo(ItemInfo* p) override;

	void IncrementAmmo(CBasePlayer* pPlayer) override;

	bool UseDecrement() override
	{
#if defined(CLIENT_WEAPONS)
		return UTIL_DefaultUseDecrement();
#else
		return false;
#endif
	}

	void ToggleZoom();

private:
	void UpdateLaser();

	bool m_bSpotVisible;
	bool m_bLaserActive;
	CSniperLaser* m_pLaser;

	unsigned short m_usSniper;
	unsigned short m_usAG36;

	bool m_bReloading;
	float m_flReloadStart;

	double m_flChargeTime;
};
