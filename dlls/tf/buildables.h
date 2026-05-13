#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "skill.h"

class CBuildable : public CBaseMonster
{
public:
    void Spawn() override;
	void Precache() override;
	int Classify() override;

	virtual void PrecacheBuildable();
    virtual void SpawnBuildable();

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
    bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
    virtual void DetonateBuilding();

	virtual bool OnWrenchHit(CBasePlayer* pPlayer) {return false;};

	void SetActivity(Activity act);
	inline Activity GetActivity() { return m_Activity; }

	int ObjectCaps() override { return CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hBuilder;

protected:
	Activity m_Activity;

private:
	int m_idShard;
};