#pragma once

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()

enum
{
    RC_EXPLODE = 0, // moving satchel charge
    RC_GUN, // shoots straight
    RC_TURRET, // shoots automatically
}

// OVERLOADS SOME ENTVARS:
// speed - the ideal magnitude of my velocity
class CRC : public CBaseEntity
{
	int m_iTrail;
public:
	static CRC* RC_Create(unsigned int RCDamage, Vector VecSpawnPos, Vector vecDir, int RCType = RC_EXPLODE, CBasePlayer* owner); // add damage, spread and owner so entities calling this can give it the proper stuff
	void Spawn() override;
	void Precache() override;
    void EXPORT ExplodeThink();
	void EXPORT AirThink();
	void EXPORT impact(CBaseEntity* pOther);
	//int ShouldCollide(CBaseEntity* pentTouched) override;
	
	static const char* pNearMissSounds[];

	edict_t* Owner;

	CBaseEntity* m_pIgnore;
	int m_Flare;
	unsigned int m_muzzlevelocity;

	Vector m_SpawnPos;
	Vector m_direction;

	unsigned int m_BulletDamage;
};