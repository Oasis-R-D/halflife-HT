#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"

// ROCKET CLASS DEFINED IN 'weapons.h' ABOVE 'CRpgRocket'

#define SENTRYGUN_MINS			Vector(-20, -20, 0)
#define SENTRYGUN_MAXS			Vector(20,  20, 66)

class CActAnimatingSentry : public CBaseMonster
{
public:
	void SetActivity(Activity act);
	inline Activity GetActivity() { return m_Activity; }

	int ObjectCaps() override { return CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

protected:
	Activity m_Activity;
};

class CTFSentryBase : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache();

	static CTFSentryBase* Sentry_Create(Vector VecSpawnPos, Vector vecDir, CBaseEntity* pOwner = NULL, int colormap = 0);

	void EXPORT FinishConstruction();

	bool m_bMapPlaced = true;

	EHANDLE m_pSentry;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
};

// --------------------------------------------------------------
// AI sentry gun
// -------------------------------------------------------------
class CTFSentry : public CActAnimatingSentry
{
public:
	void Spawn();
	void Precache();
	int Classify() override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void ExplodeSentry();

	void SetModel(const char *pModel);
	bool OnWrenchHit(CBasePlayer* pPlayer);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hBase;
	EHANDLE m_hBuilder;

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
	void SentryRotate(void);
	bool MoveTurret(void);

	// Attack
	void Attack(void);
	bool Fire(void);
	//void MakeTracer(const Vector &vecTracerSrc, const TraceResult &tr, int iTracerType);
	
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

	float m_flLastAttackedTime;

	int m_idShard;
};