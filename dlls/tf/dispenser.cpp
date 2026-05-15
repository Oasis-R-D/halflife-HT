#include "buildables.h"
#include "dispenser.h"

// Ground placed version
#define DISPENSER_MODEL				"models/dispenser.mdl"

#define DISPENSER_MAX_METAL_AMMO	400

// How much of each ammo gets added per refill
#define DISPENSER_REFILL_METAL_AMMO			40

// How much ammo is given our per use
#define DISPENSER_DROP_PRIMARY		40
#define DISPENSER_DROP_SECONDARY	40
#define DISPENSER_DROP_METAL		40

//================================================
//					DISPENSER
//================================================

LINK_ENTITY_TO_CLASS(tf_dispenser, CTFDispenser);

TYPEDESCRIPTION CTFDispenser::m_SaveData[] =
	{
		DEFINE_FIELD(CTFDispenser, m_iAmmoMetal, FIELD_INTEGER),
		DEFINE_FIELD(CTFDispenser, m_flNextRefill, FIELD_TIME),
		DEFINE_FIELD(CTFDispenser, m_flNextDispense, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CTFDispenser, CBuildable);

CTFDispenser* CTFDispenser::Dispenser_Create(Vector VecSpawnPos, Vector vecDir, CBaseEntity* pOwner)
{
	CTFDispenser* pDispenser = GetClassPtr((CTFDispenser*)NULL);
	pDispenser->pev->classname = MAKE_STRING("tf_dispenser");
	pDispenser->pev->origin = VecSpawnPos;
	pDispenser->pev->angles = vecDir;
	pDispenser->m_hBuilder = pOwner;
	pDispenser->pev->spawnflags |= SF_NORESPAWN | SF_MONSTER_FALL_TO_GROUND;
	pDispenser->m_bMapPlaced = false;
	pDispenser->Spawn();

	return pDispenser;
}

void CTFDispenser::PrecacheBuildable()
{
	PRECACHE_MODEL(DISPENSER_MODEL);

	PRECACHE_SOUND("items/9mmclip1.wav");
}

void CTFDispenser::SpawnBuildable()
{
	Precache();

	pev->health = pev->max_health = 50;

	SET_MODEL(ENT(pev), DISPENSER_MODEL);
	UTIL_SetSize(pev, DISPENSER_MINS, DISPENSER_MAXS);
	pev->classname = MAKE_STRING("tf_dispenser");

	if (m_bMapPlaced)
	{
		FinishConstruction();
	}
	else
	{
		pev->movetype = MOVETYPE_TOSS;
		m_hBuilder.Entity<CBasePlayer>()->m_hBuilding = this;
		SetThink(&CTFDispenser::FinishConstruction);
		pev->nextthink = gpGlobals->time + 2;
	}

	UTIL_SetOrigin(pev, pev->origin);
}

void CTFDispenser::FinishConstruction()
{
	m_flNextRefill = gpGlobals->time + 6; // 12 in TFC with double the given ammo!
	m_flNextDispense = gpGlobals->time + 1.0;

	pev->health = pev->max_health = 150;

	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);

	m_iAmmoMetal = 25;

	if (!m_bMapPlaced)
	{
		m_hBuilder.Entity<CBasePlayer>()->m_hBuilding = nullptr;
		m_hBuilder.Entity<CBasePlayer>()->m_hDispenser = this;
	}

	SetThink(&CTFDispenser::DispenserThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

bool CTFDispenser::DispenseAmmo(CBasePlayer* pPlayer)
{
	int iTotalPickedUp = 0;

	/*
	// primary
	int iPrimary = pPlayer->GiveAmmo( DISPENSER_DROP_PRIMARY, TF_AMMO_PRIMARY );
	iTotalPickedUp += iPrimary;

	// secondary
	int iSecondary = pPlayer->GiveAmmo( DISPENSER_DROP_SECONDARY, TF_AMMO_SECONDARY );
	iTotalPickedUp += iSecondary;
	*/

	// metal
	int iMetal = pPlayer->GiveAmmo(V_min(m_iAmmoMetal, DISPENSER_DROP_METAL), "uranium", METAL_MAX_CARRY);
	m_iAmmoMetal -= iMetal;
	iTotalPickedUp += iMetal;

	if ( iTotalPickedUp > 0 )
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		return true;
	}

	// return false if we didn't pick up anything
	return false;
}

void CTFDispenser::DispenserThink()
{
	if (m_flNextRefill <= gpGlobals->time)
	{
		m_flNextRefill = gpGlobals->time + 12;

		if ( m_iAmmoMetal < DISPENSER_MAX_METAL_AMMO )
		{
			ALERT(at_console, "REFILLED\n");
			m_iAmmoMetal = V_min( m_iAmmoMetal + DISPENSER_MAX_METAL_AMMO * 0.2, DISPENSER_MAX_METAL_AMMO );
		}
	}

	if (m_flNextDispense <= gpGlobals->time)
	{
		int iNumNearbyPlayers = 0;

		// find players in sphere, that are visible
		static float flRadius = 64;
		Vector vecOrigin = pev->origin + Vector(0,0,32);

		CBaseEntity *pListOfNearbyEntities[32];

		// uses sphere in TF2, it's broken however and is actually a box though (in TF2)
		int iNumberOfNearbyEntities = UTIL_EntitiesInBox( pListOfNearbyEntities, 32, vecOrigin-Vector(flRadius, flRadius, flRadius), vecOrigin+Vector(flRadius, flRadius, flRadius), FL_CLIENT );
		for (int i=0; i<iNumberOfNearbyEntities; i++)
		{
			CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>( pListOfNearbyEntities[i] );

			if ( !pPlayer || !pPlayer->IsAlive() )
				continue;
			
			ALERT(at_console, "DISPENSED\n");
			DispenseAmmo( pPlayer );

			iNumNearbyPlayers++;
		}

		// Try to dispense more often when no players are around so we 
		// give it as soon as possible when a new player shows up
		m_flNextDispense = gpGlobals->time + ( ( iNumNearbyPlayers > 0 ) ? 1.0 : 0.1 );
	}
	pev->nextthink = gpGlobals->time + 0.05;
}

//-----------------------------------------------------------------------------
// Purpose: Called when this object is destroyed
//-----------------------------------------------------------------------------
void CTFDispenser::DetonateBuilding()
{
	ClientPrint(m_hBuilder.Entity<CBasePlayer>()->pev, HUD_PRINTNOTIFY, "#Dispenser_destroyed");

	m_hBuilder.Entity<CBasePlayer>()->m_hDispenser = nullptr;
	if (m_hBuilder.Entity<CBasePlayer>()->m_hBuilding == this)
		m_hBuilder.Entity<CBasePlayer>()->m_hBuilding = nullptr;

	Vector pos = Center();

	// breakmodel
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY, pos);
		WRITE_BYTE(TE_BREAKMODEL);
		// position
		WRITE_COORD(pos.x);
		WRITE_COORD(pos.y);
		WRITE_COORD(pos.z-8);
		// size
		WRITE_COORD(32);
		WRITE_COORD(32);
		WRITE_COORD(32);
		// velocity
		WRITE_COORD(0);
		WRITE_COORD(0);
		WRITE_COORD(0);
		WRITE_BYTE(35); // randomization
		// Model
		WRITE_SHORT(m_idShard); // model id#
		// # of shards
		WRITE_BYTE(5);
		// duration
		WRITE_BYTE(30); // 3.0 seconds
		WRITE_BYTE(BREAK_SMOKE); // flags
	MESSAGE_END();

	// explosion
	int iContents = UTIL_PointContents(pev->origin);
	
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);	// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD(pev->origin.x); // Send to PAS because of the sound
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		if (iContents != CONTENTS_WATER)
		{
			WRITE_SHORT(g_sModelIndexFireball);
		}
		else
		{
			WRITE_SHORT(g_sModelIndexWExplosion);
		}
		WRITE_BYTE((100 - 50) * .60); // scale * 10
		WRITE_BYTE(15);					   // framerate
		WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);
		break;
	}

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pos, QUIET_GUN_VOLUME, 3.0);

	// deal more damage if we have tons of ammo
	float flDamage = V_min( 100 + m_iAmmoMetal, 250 );

	//RadiusDamage(pos, pev, m_hBuilder != nullptr ? m_hBuilder->pev : pev, 32+ammoMult+ammoRocketMult, CLASS_NONE, DMG_BLAST);
	RadiusDamage(pos, pev, m_hBuilder != nullptr ? m_hBuilder->pev : pev, flDamage, CLASS_NONE, DMG_BLAST);

	UTIL_Remove(this);
}