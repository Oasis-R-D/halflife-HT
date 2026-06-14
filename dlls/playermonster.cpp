//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: New version of the slider bar
//
// $NoKeywords: $
//=============================================================================

//=========================================================
// playermonster - for scripted sequence use.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_MONSTERPLAYER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CPlayerMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	bool StartCopying(CBasePlayer* pController);
	int ISoundMask ( void );
	void MoveThink();
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

protected:
	CBasePlayer* m_pController;
};
LINK_ENTITY_TO_CLASS(monster_player, CPlayerMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CPlayerMonster :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPlayerMonster :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CPlayerMonster :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - player monster can't hear.
//=========================================================
int CPlayerMonster :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CPlayerMonster :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/player.mdl");
	UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	pev->takedamage = DAMAGE_NO;
	m_flFieldOfView = 0.5f;
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPlayerMonster :: Precache()
{
	PRECACHE_MODEL("models/player.mdl");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
bool CPlayerMonster::StartCopying(CBasePlayer* pController)
{
	if (m_pController != NULL)
		return false;

	m_pController = pController;
	SetThink(&CPlayerMonster::MoveThink);
	pev->nextthink = gpGlobals->time + 0.01
	return true;

	
}

void CPlayerMonster::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	CBaseMonster::HandleAnimEvent(pEvent);
}

void CPlayerMonster::MoveThink()
{
	if (!m_pController)
	{
		ALERT(at_console, "Player is dead!\n");
		return;
	}

	pev->nextthink = gpGlobals->time + 0.01 // NOTE: this would be better called in the player's post/pre frame instead of on a think
	
	pev->origin = m_pController->pev->origin
	pev->angles = m_pController->pev->angles
	pev->sequence = m_pController->pev->sequence
	// TO-DO: copy model blending and weapon model (if we do have this used where a player has a weapon
}