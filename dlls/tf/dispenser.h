#pragma once
#include "buildables.h"

#define DISPENSER_MINS			Vector( -20, -20, 0)
#define DISPENSER_MAXS			Vector( 20, 20, 55)	// tweak me

#define DISPENSER_TRIGGER_MINS			Vector(-70, -70, 0)
#define DISPENSER_TRIGGER_MAXS			Vector( 70,  70, 50)	// tweak me

class CTFDispenser : public CBuildable
{
public:
	void SpawnBuildable() override;
	void PrecacheBuildable() override;

	void DetonateBuilding() override;

	//bool OnWrenchHit(CBasePlayer* pPlayer) override;

	static CTFDispenser* Dispenser_Create(Vector VecSpawnPos, Vector vecDir, CBaseEntity* pOwner = NULL);

	bool DispenseAmmo(CBasePlayer* pPlayer);

	void EXPORT FinishConstruction();
	void EXPORT DispenserThink();

	unsigned int m_iAmmoMetal;

	float m_flNextRefill;
	float m_flNextDispense;

	bool m_bMapPlaced = true;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hBuilder;
};