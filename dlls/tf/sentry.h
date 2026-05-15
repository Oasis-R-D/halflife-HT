#pragma once
#include "buildables.h"

// ROCKET CLASS DEFINED IN 'weapons.h' ABOVE 'CRpgRocket'

#define SENTRYGUN_MINS			Vector(-20, -20, 0)
#define SENTRYGUN_MAXS			Vector(20,  20, 66)

class CTFSentryBase : public CBuildable
{
public:
	void Spawn() override;
	void PrecacheBuildable() override;

	bool KeyValue(KeyValueData* pkvd) override;

	static CTFSentryBase* Sentry_Create(Vector VecSpawnPos, Vector vecDir, CBaseEntity* pOwner = NULL, int colormap = 0);

	bool OnWrenchHit(CBasePlayer* pPlayer) override { return true; };
	void EXPORT FinishConstruction();

	void DetonateBuilding() override;

	bool m_bMapPlaced = true;
	int m_iMapPlacedLevel;

	EHANDLE m_pSentry;

	int colormap;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
};

// --------------------------------------------------------------
// AI sentry gun
// -------------------------------------------------------------
class CTFSentry : public CBuildable
{
public:
	void SpawnBuildable() override;
	void PrecacheBuildable() override;

	void DetonateBuilding() override;

	void SetModel(const char *pModel);
	bool OnWrenchHit(CBasePlayer* pPlayer) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hBase;

private:
	// Main think
	void EXPORT SentryThink();

	// If the players hit us with a wrench, should we upgrade
	bool CanBeUpgraded(CBasePlayer* pPlayer);
	void Upgrade();

	// Target acquisition
	bool FindTarget(); // find nearest enemy
	bool AcquireEnemyFail(CBaseEntity* target); // check if enemy is right type
	bool ValidTargetEnt(CBaseEntity* pPlayer, const Vector &vecStart, const Vector &vecEnd); // can actually shoot them
	void FoundTarget(CBaseEntity* pTarget, const Vector &vecSoundCenter); // SHOOT IT, DIE DIE!!!!
	bool FInViewCone (CBaseEntity* pEntity ); // UNUSED (should probably use this???)

	// Rotations
	void SentryRotate();
	bool MoveTurret();

	// Attack
	void Attack();
	bool Fire();
	
	int m_iState;

	float m_flNextAttack;

	float m_flFieldOfView;	// width of monster's field of view ( dot product )

	int m_iUpgradeLevel;

	int m_iRightBound;
	int m_iLeftBound;
	int	m_iBaseTurnRate;
	bool m_bTurningRight;

	Vector m_vecCurAngles;
	Vector m_vecGoalAngles;

	float m_flTurnRate;

	int m_iUpgradeMetal;
	int m_iUpgradeMetalRequired;

	int m_iAmmo;
	int m_iMaxAmmo;
	int m_iAmmoRockets;
	int m_iMaxAmmoRockets;

	float m_flNextRocketAttack;

	EHANDLE m_hEnemy;

	int m_iAttachments[4];

	double m_fPitch;
	double m_fYaw;

	friend class CTFSentryBase; 
};