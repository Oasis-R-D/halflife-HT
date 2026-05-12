//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Sentrygun OMG
//
// $NoKeywords: $
//=============================================================================//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "animation.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "sentry.h"

#pragma region ROCKET

TYPEDESCRIPTION CSentryRocket::m_SaveData[] =
	{
		DEFINE_FIELD(CSentryRocket, m_flIgniteTime, FIELD_TIME),
};
IMPLEMENT_SAVERESTORE(CSentryRocket, CGrenade);

LINK_ENTITY_TO_CLASS(sentry_rocket, CSentryRocket);

//=========================================================
//=========================================================
CSentryRocket* CSentryRocket::CreateRpgRocket(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner)
{
	CSentryRocket* pRocket = GetClassPtr((CSentryRocket*)NULL);

	UTIL_SetOrigin(pRocket->pev, vecOrigin);
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch(&CSentryRocket::RocketTouch);
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

//=========================================================
//=========================================================
void CSentryRocket::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/rpgrocket.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin(pev, pev->origin);

	pev->classname = MAKE_STRING("sentry_rocket");

	SetThink(&CSentryRocket::IgniteThink);
	SetTouch(&CSentryRocket::ExplodeTouch);

	pev->angles.x -= 30;
	UTIL_MakeVectors(pev->angles);
	pev->angles.x = -(pev->angles.x + 30);

	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.4;

	pev->dmg = 100; // TO-DO: skill value
}

//=========================================================
//=========================================================
void CSentryRocket::RocketTouch(CBaseEntity* pOther)
{
	STOP_SOUND(edict(), CHAN_VOICE, "weapons/rocket1.wav");
	ExplodeTouch(pOther);
}

//=========================================================
//=========================================================
void CSentryRocket::Precache()
{
	PRECACHE_MODEL("models/rpgrocket.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("weapons/rocket1.wav");
}

void CSentryRocket::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5);

	// rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex()); // entity
	WRITE_SHORT(m_iTrail);	 // model
	WRITE_BYTE(40);			 // life
	WRITE_BYTE(5);			 // width
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(255);		 // r, g, b
	WRITE_BYTE(255);		 // brightness

	MESSAGE_END(); // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink(&CSentryRocket::FollowThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CSentryRocket::FollowThink()
{
	CBaseEntity* pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	TraceResult tr;

	UTIL_MakeAimVectors(pev->angles);

	vecTarget = gpGlobals->v_forward;

	pev->angles = UTIL_VecToAngles(vecTarget);

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0)
	{
		pev->velocity = pev->velocity * 0.2 + vecTarget * (flSpeed * 0.8 + 400);
		if (pev->waterlevel == 3)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 4);
		}
		else
		{
			if (pev->velocity.Length() > 2000)
			{
				pev->velocity = pev->velocity.Normalize() * 2000;
			}
		}
	}
	else
	{
		if ((pev->effects & EF_LIGHT) != 0)
		{
			pev->effects = 0;
			STOP_SOUND(ENT(pev), CHAN_VOICE, "weapons/rocket1.wav");
		}
		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;
		if (pev->waterlevel == 0 && pev->velocity.Length() < 1500)
		{
			Detonate();
		}
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

	pev->nextthink = gpGlobals->time + 0.1;
}

#pragma endregion

// Ground placed version
#define SENTRYGUN_UPGRADE_METAL 125 // around 100 in TFC

#define SENTRY_MODEL_PLACEMENT	"models/base.mdl"
#define SENTRY_MODEL_LEVEL_1	"models/sentry1.mdl"
#define SENTRY_MODEL_LEVEL_2	"models/sentry2.mdl"
#define SENTRY_MODEL_LEVEL_3	"models/sentry3.mdl"

#define SENTRY_ROCKET_MODEL		"models/rpgrocket.mdl"

#define SENTRYGUN_MINS			Vector(-20, -20, 0)
#define SENTRYGUN_MAXS			Vector(20,  20, 66)

#define SENTRYGUN_MAX_ROCKETS	10
#define SENTRYGUN_MAX_HEALTH	150
#define SENTRYGUN_MAX_SHELLS_1	50
#define SENTRYGUN_MAX_SHELLS_2	75
#define SENTRYGUN_MAX_SHELLS_3	100

#define SENTRYGUN_ADD_SHELLS	40
#define SENTRYGUN_ADD_ROCKETS	8
#define SENTRYGUN_ADD_UPGRADE	25
#define SENTRY_THINK_DELAY		0.05

#define SENTRYGUN_RECENTLY_ATTACKED_TIME 2.0

#define BONE_PITCH	1
#define BONE_YAW	0

#define SENTRY_SOUND_IDLE		"sentry/turridle.wav"
#define SENTRY_SOUND_SPOT		"sentry/turrspot.wav"
#define SENTRY_SOUND_SET		"sentry/turrset.wav"
#define SENTRY_SOUND_EMPTY		"weapons/357_cock1.wav"

enum states
{	
	SENTRY_STATE_INACTIVE,
	SENTRY_STATE_SEARCHING,
	SENTRY_STATE_ATTACKING,
};

enum
{	
	SENTRYGUN_ATTACHMENT_MUZZLE = 0,
	SENTRYGUN_ATTACHMENT_MUZZLE_ALT,
	SENTRYGUN_ATTACHMENT_ROCKET_L,
	SENTRYGUN_ATTACHMENT_ROCKET_R,
};

TYPEDESCRIPTION CTFSentry::m_SaveData[] =
	{
		DEFINE_FIELD(CTFSentry, m_iState, FIELD_INTEGER),
		DEFINE_FIELD(CTFSentry, m_iUpgradeLevel, FIELD_INTEGER),

		DEFINE_FIELD(CTFSentry, m_iAttachments, FIELD_INTEGER),

		DEFINE_FIELD(CTFSentry, m_iAmmo, FIELD_INTEGER),
		DEFINE_FIELD(CTFSentry, m_iMaxAmmo, FIELD_INTEGER),
		DEFINE_FIELD(CTFSentry, m_iAmmoRockets, FIELD_INTEGER),
		DEFINE_FIELD(CTFSentry, m_iMaxAmmoRockets, FIELD_INTEGER),

		DEFINE_FIELD(CTFSentry, m_iMaxAmmoRockets, FIELD_INTEGER),

		DEFINE_FIELD(CTFSentry, m_iUpgradeMetal, FIELD_INTEGER),
		DEFINE_FIELD(CTFSentry, m_iUpgradeMetalRequired, FIELD_INTEGER),

		DEFINE_FIELD(CTFSentry, m_iRightBound, FIELD_INTEGER),
		DEFINE_FIELD(CTFSentry, m_iLeftBound, FIELD_INTEGER),
		DEFINE_FIELD(CTFSentry, m_bTurningRight, FIELD_BOOLEAN),
		DEFINE_FIELD(CTFSentry, m_iBaseTurnRate, FIELD_INTEGER),

		DEFINE_FIELD(CTFSentry, m_flNextAttack, FIELD_TIME),
		DEFINE_FIELD(CTFSentry, m_flNextRocketAttack, FIELD_TIME),
		DEFINE_FIELD(CTFSentry, m_flLastAttackedTime, FIELD_TIME),

		DEFINE_FIELD(CTFSentry, m_flFieldOfView, FIELD_FLOAT),

		DEFINE_FIELD(CTFSentry, m_vecCurAngles, FIELD_VECTOR),
		DEFINE_FIELD(CTFSentry, m_vecGoalAngles, FIELD_VECTOR),

		DEFINE_FIELD(CTFSentry, m_fPitch, FIELD_FLOAT),
		DEFINE_FIELD(CTFSentry, m_fYaw, FIELD_FLOAT),

		DEFINE_FIELD(CTFSentry, m_hEnemy, FIELD_EHANDLE),
		DEFINE_FIELD(CTFSentry, m_hBuilder, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CTFSentry, CActAnimatingSentry);

LINK_ENTITY_TO_CLASS(tf_sentry, CTFSentry);

#define SENTRY_GUN_DAMAGE 16

#define SENTRY_METAL_PER_SHELL 1.0
#define SENTRY_METAL_PER_ROCKET 2.0

TYPEDESCRIPTION CActAnimatingSentry::m_SaveData[] =
	{
		DEFINE_FIELD(CActAnimatingSentry, m_Activity, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CActAnimatingSentry, CBaseAnimating);

void CActAnimatingSentry::SetActivity(Activity act)
{
	int sequence = LookupActivity(act);
	if (sequence != ACTIVITY_NOT_AVAILABLE)
	{
		pev->sequence = sequence;
		m_Activity = act;
		pev->frame = 0;
		ResetSequenceInfo();
	}
}

void CTFSentry::Spawn()
{
	m_fPitch = 0;
	m_fYaw = 0;

	SET_MODEL(edict(), SENTRY_MODEL_LEVEL_1);

	pev->takedamage = DAMAGE_YES;

	pev->solid = SOLID_BBOX;

	m_bloodColor = DONT_BLEED;

	m_iUpgradeLevel = 1;
	m_iUpgradeMetal = 0;
	m_iUpgradeMetalRequired = SENTRYGUN_UPGRADE_METAL;

	// NPCs attack it
	SetBits(pev->flags, FL_MONSTER);
	pev->flags |= FL_MONSTER; // extraneous?

	pev->health = SENTRYGUN_MAX_HEALTH; // TO-DO: skill cvar

	// Rotate Details
	m_iRightBound = 45;
	m_iLeftBound = 315;
	m_iBaseTurnRate = 6;
	m_flFieldOfView = VIEW_FIELD_NARROW;

	// Give the Gun some ammo
	m_iAmmo = 0;
	m_iAmmoRockets = 0;
	m_iMaxAmmo = SENTRYGUN_MAX_SHELLS_1;
	m_iMaxAmmoRockets = SENTRYGUN_MAX_ROCKETS;

	// Start searching for enemies
	m_hEnemy = NULL;

	m_flLastAttackedTime = 0;

	pev->view_ofs = Vector(0, 0, 30);

	UTIL_SetSize(pev, SENTRYGUN_MINS, SENTRYGUN_MAXS);

	m_iState = SENTRY_STATE_INACTIVE;

	OnGoActive(); // TO-DO: remove

	SetThink(&CTFSentry::SentryThink);
	pev->nextthink = gpGlobals->time;
}

void CTFSentry::SentryThink()
{
	switch(m_iState)
	{
	case SENTRY_STATE_INACTIVE:
		ALERT(at_console, "INACTIVE\n");
		break;

	case SENTRY_STATE_SEARCHING:
		ALERT(at_console, "SEARCH\n");
		SentryRotate();
		break;

	case SENTRY_STATE_ATTACKING:
		ALERT(at_console, "ATTACK\n");
		Attack();
		break;

	default:
		ASSERT(0);
		break;
	}

	pev->nextthink = gpGlobals->time + SENTRY_THINK_DELAY;
}

/*
//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CTFSentry::StartBuilding(CBaseEntity* pBuilder)
{
	//CreateBuildPoints();

	//SetPoseParameter(m_iPitchPoseParameter, 0.0);
	//SetPoseParameter(m_iYawPoseParameter, 0.0);

	SetBoneController(BONE_PITCH,	0);
	SetBoneController(BONE_YAW,		0);

	m_fYaw = m_fPitch = 0.0;
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSentry::OnGoActive()
{
	SetModel(SENTRY_MODEL_LEVEL_1);

	m_iState = (SENTRY_STATE_SEARCHING);

	// Orient it
	Vector angles = pev->angles;

	m_vecCurAngles.y = UTIL_AngleMod(angles.y);
	m_iRightBound = UTIL_AngleMod((int)angles.y - 50);
	m_iLeftBound = UTIL_AngleMod((int)angles.y + 50);
	if (m_iRightBound > m_iLeftBound)
	{
		m_iRightBound = m_iLeftBound;
		m_iLeftBound = UTIL_AngleMod((int)angles.y - 50);
	}

	// Start it rotating
	m_vecGoalAngles.y = m_iRightBound;
	m_vecGoalAngles.x = m_vecCurAngles.x = 0;
	m_bTurningRight = true;

	EMIT_SOUND(edict(), CHAN_AUTO, SENTRY_SOUND_SET, 1.0, ATTN_IDLE);

	m_iAmmo = m_iMaxAmmo;

	// Init attachments for level 1 sentry gun
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] = 0;
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT] = 1;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L] = 2;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R] = 3;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSentry::Precache()
{
	// Models
	PRECACHE_MODEL(SENTRY_MODEL_PLACEMENT);

	PRECACHE_MODEL(SENTRY_MODEL_LEVEL_1);
	PRECACHE_MODEL(SENTRY_MODEL_LEVEL_2);
	PRECACHE_MODEL(SENTRY_MODEL_LEVEL_3);
	PRECACHE_MODEL(SENTRY_ROCKET_MODEL);

	PRECACHE_SOUND(SENTRY_SOUND_IDLE);
	PRECACHE_SOUND(SENTRY_SOUND_SPOT);
	PRECACHE_SOUND(SENTRY_SOUND_SET);
	PRECACHE_SOUND(SENTRY_SOUND_EMPTY);

	UTIL_PrecacheOther("sentry_rocket");

	m_idShard = PRECACHE_MODEL("models/metalplategibs.mdl");
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CTFSentry::CanBeUpgraded(CBasePlayer* pPlayer)
{
	// only engineers
	/*
	if (!ClassCanBuild(pPlayer->GetPlayerClass()->GetClassIndex(), GetType()))
	{
		return false;
	}*/

	// max upgraded
	if (m_iUpgradeLevel >= 3)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Raises the Sentrygun one level
//-----------------------------------------------------------------------------
void CTFSentry::Upgrade()
{
	m_hEnemy = NULL;

	// Increase level
	m_iUpgradeLevel++;

	// more health
	pev->max_health = pev->max_health * 1.2;
	pev->health = pev->max_health;

	EMIT_SOUND(edict(), CHAN_AUTO, SENTRY_SOUND_SET, 1.0, ATTN_IDLE);

	switch(m_iUpgradeLevel)
	{
	case 2:
		SetModel(SENTRY_MODEL_LEVEL_2);
		m_iMaxAmmo = SENTRYGUN_MAX_SHELLS_2;
		break;
	case 3:
		SetModel(SENTRY_MODEL_LEVEL_3);
		m_iAmmoRockets = SENTRYGUN_MAX_ROCKETS;
		m_iMaxAmmo = SENTRYGUN_MAX_SHELLS_3;
		break;
	default:
		ASSERT(0);
		break;
	}

	// more ammo capability
	m_iAmmo = m_iMaxAmmo;

	m_iState = (SENTRY_STATE_SEARCHING);

	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] = 0;
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT] = 1;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L] = 2;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R] = 3;
}

//-----------------------------------------------------------------------------
// Hit by a friendly engineer's wrench
//-----------------------------------------------------------------------------
bool CTFSentry::OnWrenchHit(CBasePlayer* pPlayer)
{
	bool bDidWork = false;
	bool upgraded = false;

	static int metal_ammo = pPlayer->GetAmmoIndex("uranium");

	// If the player repairs it at all, we're done
	if (pev->health < pev->max_health)
	{
		/*
		if (Command_Repair(pPlayer))
		{
			bDidWork = true;
		}
		*/
	}

	// Don't put in upgrade metal until the sentry is fully healed
	if (!bDidWork && CanBeUpgraded(pPlayer))
	{
		pPlayer->TabulateAmmo();
		int iPlayerMetal = pPlayer->ammo_metal;
		int iAmountToAdd = V_min(SENTRYGUN_ADD_UPGRADE, iPlayerMetal);

		if (iAmountToAdd > (m_iUpgradeMetalRequired - m_iUpgradeMetal))
			iAmountToAdd = (m_iUpgradeMetalRequired - m_iUpgradeMetal);

		//pPlayer->RemoveAmmo(iAmountToAdd, TF_AMMO_METAL);
		pPlayer->m_rgAmmo[metal_ammo] -= iAmountToAdd;

		m_iUpgradeMetal += iAmountToAdd;

		if (iAmountToAdd > 0)
		{
			bDidWork = true;
		}

		if (m_iUpgradeMetal >= m_iUpgradeMetalRequired)
		{
			Upgrade();
			m_iUpgradeMetal = 0;
			upgraded = true;
		}
	}

	if (!upgraded)
	{
		// player ammo into rockets
		//	1 ammo = 1 shell
		//  2 ammo = 1 rocket
		//  only fill rockets if we have extra shells

		pPlayer->TabulateAmmo();
		int iPlayerMetal = pPlayer->ammo_metal;

		// If the sentry has less that 100% ammo, put some ammo in it
		if (m_iAmmo < m_iMaxAmmo && iPlayerMetal > 0)
		{
			int iMaxShellsPlayerCanAfford = (int)((float)iPlayerMetal / SENTRY_METAL_PER_SHELL);

			// cap the amount we can add
			int iAmountToAdd = V_min(SENTRYGUN_ADD_SHELLS, iMaxShellsPlayerCanAfford);

			iAmountToAdd = V_min((m_iMaxAmmo - m_iAmmo), iAmountToAdd);

			//pPlayer->RemoveAmmo(iAmountToAdd * SENTRY_METAL_PER_SHELL, TF_AMMO_METAL);
			pPlayer->m_rgAmmo[metal_ammo] -= iAmountToAdd * SENTRY_METAL_PER_SHELL;
			m_iAmmo += iAmountToAdd;

			if (iAmountToAdd > 0)
			{
				bDidWork = true;
			}
		}

		// One rocket per two ammo
		pPlayer->TabulateAmmo();
		iPlayerMetal = pPlayer->ammo_metal;

		if (m_iAmmoRockets < m_iMaxAmmoRockets && m_iUpgradeLevel == 3 && iPlayerMetal > 0 )
		{
			int iMaxRocketsPlayerCanAfford = (int)((float)iPlayerMetal / SENTRY_METAL_PER_ROCKET);

			int iAmountToAdd = V_min((SENTRYGUN_ADD_ROCKETS), iMaxRocketsPlayerCanAfford);
			iAmountToAdd = V_min((m_iMaxAmmoRockets - m_iAmmoRockets), iAmountToAdd);

			//pPlayer->RemoveAmmo(iAmountToAdd * SENTRY_METAL_PER_ROCKET, TF_AMMO_METAL);
			pPlayer->m_rgAmmo[metal_ammo] -= iAmountToAdd * SENTRY_METAL_PER_ROCKET;
			m_iAmmoRockets += iAmountToAdd;

			if (iAmountToAdd > 0)
			{
				bDidWork = true;
			}
		}
	}

	return bDidWork;
}

int CTFSentry::Classify()
{
	return CLASS_PLAYER_ALLY;
}

bool CTFSentry::AcquireEnemyFail(CBaseEntity* target)
{
	if (target == this || !target->IsAlive() || target->pev->takedamage == DAMAGE_NO)
		return true;

	// must be a fleshy little guy
	if ((target->pev->flags & (FL_CLIENT | FL_MONSTER)) == 0)
		return true;

	if ((target->pev->flags & FL_NOTARGET) != 0)
		return true;

	// TO-DO: fix this function
	//if (!FInViewCone(target))
		//return true;

	if (IRelationship(target) <= R_NO)
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Look for a target
//-----------------------------------------------------------------------------
bool CTFSentry::FindTarget()
{
	// Loop through players within 1100 units (sentry range).
	Vector vecSentryOrigin = EyePosition();

	// If we have an enemy get his minimum distance to check against.
	Vector vecSegment;
	Vector vecTargetCenter;
	float flMinDist2 = 1210000; // 1100 squared
	CBaseEntity* pTargetCurrent = NULL;
	CBaseEntity* pTargetPlayer = NULL;
	CBaseEntity* pTargetOld = CBaseEntity::Instance(m_hEnemy.Get());
	float flOldTargetDist2 = 2147483647.0;

	while ((pTargetPlayer = UTIL_FindEntityInSphere(pTargetPlayer, pev->origin, 1100)) != NULL)
	{
		if (AcquireEnemyFail(pTargetPlayer))
			continue;

		vecTargetCenter = pTargetPlayer->EyePosition(); // FVisible uses eye pos
		VectorSubtract(vecTargetCenter, vecSentryOrigin, vecSegment);
		float flDist2 = vecSegment.LengthSquared();

		// Store the current target distance if we come across it
		if (pTargetPlayer == pTargetOld)
		{
			flOldTargetDist2 = flDist2;
		}

		// Check to see if the target is closer than the already validated target.
		if (flDist2 > flMinDist2)
			continue;

		// It is closer, check to see if the target is valid.
		if (ValidTargetEnt(pTargetPlayer, vecSentryOrigin, vecTargetCenter))
		{
			flMinDist2 = flDist2;
			pTargetCurrent = pTargetPlayer;
		}
	}

	// We have a target.
	if (pTargetCurrent)
	{
		ALERT(at_console, "VALID TARGET!!!!");
		if (pTargetCurrent != pTargetOld)
		{
			// flMinDist2 is the new target's distance
			// flOldTargetDist2 is the old target's distance
			// Don't switch unless the new target is closer by some percentage
			if (flMinDist2 < (flOldTargetDist2 * 0.75f))
			{
				FoundTarget(pTargetCurrent, vecSentryOrigin);
			}
		}
		return true;
	}

	return false;

	// DEBUG
	/*
	CBaseEntity* player = CBaseEntity::Instance(FIND_CLIENT_IN_PVS(edict()));
	if (player)
	{
		if (ValidTargetEnt(player, g_vecZero, g_vecZero))
		{
			if (player == m_hEnemy)
				return true;
			FoundTarget(player, EyePosition());
			return true;
		}
	}
	return false;
	*/
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSentry::ValidTargetEnt(CBaseEntity* pEnt, const Vector &vecStart, const Vector &vecEnd)
{
	/*
	if (pEnt->IsPlayer())
	{
		CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(pEnt);
		// Keep shooting at spies that go invisible after we acquire them as a target.
		if (pPlayer->m_Shared.GetPercentInvisible() > 0.5)
			return false;

		// Keep shooting at spies that disguise after we acquire them as at a target.
		if (pPlayer->m_Shared.InCond(TF_COND_DISGUISED) && pPlayer->m_Shared.GetDisguiseTeam() == GetTeamNumber() && pPlayer != m_hEnemy)
			return false;
	}

	// Not across water boundary. // Done in FVisible
	if ((pev->waterlevel == 0 && pPlayer->pev->waterlevel >= 3) || (pev->waterlevel == 3 && pPlayer->pev->waterlevel <= 0))
		return false;
	*/

	// Ray trace!!!
	return FVisible(pEnt);
}

//-----------------------------------------------------------------------------
// Found a Target
//-----------------------------------------------------------------------------
void CTFSentry::FoundTarget(CBaseEntity* pTarget, const Vector &vecSoundCenter)
{
	m_hEnemy = pTarget;

	if ((m_iAmmo > 0) || (m_iAmmoRockets > 0 && m_iUpgradeLevel == 3))
	{
		EMIT_SOUND(edict(), CHAN_AUTO, SENTRY_SOUND_SPOT, 1.0, ATTN_NORM);
	}

	// Update timers, we are attacking now!
	m_iState = SENTRY_STATE_ATTACKING;
	m_flNextAttack = gpGlobals->time + SENTRY_THINK_DELAY;
	if (m_flNextRocketAttack < gpGlobals->time)
	{
		m_flNextRocketAttack = gpGlobals->time + 0.5;
	}
}

//-----------------------------------------------------------------------------
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//-----------------------------------------------------------------------------
bool CTFSentry::FInViewCone(CBaseEntity* pEntity)
{
	Vector forward, unused, unused2;
	AngleVectors(m_vecCurAngles, &forward, &unused, &unused2);

	Vector2D vec2LOS = (pEntity->pev->origin - pev->origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	float flDot = DotProduct(vec2LOS, (forward.Make2D()));

	if (flDot > m_flFieldOfView)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Make sure our target is still valid, and if so, fire at it
//-----------------------------------------------------------------------------
void CTFSentry::Attack()
{
	StudioFrameAdvance();

	if (!FindTarget())
	{
		m_iState = (SENTRY_STATE_SEARCHING);
		m_hEnemy = NULL;
		return;
	}

	// Track enemy
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = m_hEnemy->EyePosition();
	Vector vecDirToEnemy = vecMidEnemy - vecMid;

	Vector angToTarget;
	VectorAngles(vecDirToEnemy, angToTarget);

	angToTarget.y = UTIL_AngleMod(angToTarget.y);
	if (angToTarget.x < -180)
		angToTarget.x += 360;
	if (angToTarget.x > 180)
		angToTarget.x -= 360;

	// now all numbers should be in [1...360]
	// pin to turret limitations to [-50...50]
	if (angToTarget.x > 50)
		angToTarget.x = 50;
	else if (angToTarget.x < -50)
		angToTarget.x = -50;
	m_vecGoalAngles.y = -angToTarget.y;
	m_vecGoalAngles.x = angToTarget.x;

	MoveTurret();

	// Fire on the target if it's within 10 units of being aimed right at it
	if (m_flNextAttack <= gpGlobals->time && (m_vecGoalAngles - m_vecCurAngles).Length() <= 10)
	{
		Fire();

		if (m_iUpgradeLevel == 1)
		{
			// Level 1 sentries fire slower
			m_flNextAttack = gpGlobals->time + 0.2;
		}
		else
		{
			m_flNextAttack = gpGlobals->time + 0.1;
		}
	}
	else
	{
		// SetSentryAnim(TFTURRET_ANIM_SPIN);
	}
}

//-----------------------------------------------------------------------------
// Fire on our target
//-----------------------------------------------------------------------------
bool CTFSentry::Fire()
{
	Vector vecAimDir;

	// Level 3 Turrets fire rockets every 3 seconds
	if (m_iUpgradeLevel == 3 && m_iAmmoRockets > 0 && m_flNextRocketAttack < gpGlobals->time)
	{
		Vector vecSrc;
		Vector vecAng;

		// alternate between the 2 rocket launcher ports.
		if (m_iAmmoRockets & 1)
		{
			GetAttachment(m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L], vecSrc, vecAng);
		}
		else
		{
			GetAttachment(m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R], vecSrc, vecAng);
		}

		vecAimDir = m_hEnemy->Center() - vecSrc;
		vecAimDir = vecAimDir.Normalize();

		// TO-DO: add rocket fire sound

		Vector angAimDir;
		VectorAngles(vecAimDir, angAimDir);
		
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, LOUD_GUN_VOLUME, 3.0);

		CSentryRocket::CreateRpgRocket(vecSrc, vecAimDir, m_hBuilder != nullptr ? m_hBuilder.Entity<CBaseEntity>() : this);

		m_flNextRocketAttack = gpGlobals->time + 3;

		m_iAmmoRockets--;

		if (m_iAmmoRockets == 10)
			ClientPrint(m_hBuilder.Entity<CBasePlayer>()->pev, HUD_PRINTNOTIFY, "#Sentry_rocketslow");
		if (m_iAmmoRockets == 0)
			ClientPrint(m_hBuilder.Entity<CBasePlayer>()->pev, HUD_PRINTNOTIFY, "#Sentry_rocketsout");
	}

	if (m_iAmmo > 0)
	{
		if (m_Activity != ACT_RANGE_ATTACK1)
		{
			SetActivity(ACT_RANGE_ATTACK1);
		}
		Vector vecSrc;
		Vector vecAng;

		int iAttachment;

		if (m_iUpgradeLevel > 1 && bool(m_iAmmo & 1))
		{
			// level 2 and 3 turrets alternate muzzles each time they fizzy fizzy fire.
			iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT];
		}
		else
		{
			iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE];
		}

		GetAttachment(iAttachment, vecSrc, vecAng);

		Vector vecMidEnemy = m_hEnemy->Center();

		// If we cannot see their Center (possible, as we do our target finding based
		// on the eye position of the target) then fire at the eye position
		TraceResult tr;
		UTIL_TraceLine(vecSrc, vecMidEnemy, ignore_monsters, ignore_glass, edict(), &tr);

		if (!tr.pHit || CBaseEntity::Instance(tr.pHit)->IsBSPModel())
		{
			// Hack it lower a little bit..
			// The eye position is not always within the hitboxes for a standing TF Player
			vecMidEnemy = m_hEnemy->EyePosition() + Vector(0,0,-5);
		}

		vecAimDir = vecMidEnemy - vecSrc;

		float flDistToTarget = vecAimDir.Length();

		vecAimDir = vecAimDir.Normalize();

		pev->effects |= EF_MUZZLEFLASH;

		FireBullets(1, vecSrc, vecAimDir, VECTOR_CONE_5DEGREES, flDistToTarget + 100, BULLET_MONSTER_MP5, 1, SENTRY_GUN_DAMAGE, m_hBuilder != nullptr ? m_hBuilder.Entity<CBasePlayer>()->pev : pev); // TO-DO: add owner

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_GUN_VOLUME, 3.0);

		char wpnsnd2[256];
		sprintf(wpnsnd2, "weapons/hks%d.wav", RANDOM_LONG(1, 3));
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, wpnsnd2, 1.0, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 10));

		m_iAmmo--;
	}
	else
	{
		// could probably use a separate anim for dry fire
		if (m_Activity == ACT_RANGE_ATTACK1)
		{
			SetActivity(ACT_IDLE);
		}
		// Out of ammo, play a click
		EMIT_SOUND(edict(), CHAN_AUTO, SENTRY_SOUND_EMPTY, 1.0, ATTN_NORM);

		m_flNextAttack = gpGlobals->time + 0.2;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Rotate and scan for targets
//-----------------------------------------------------------------------------
void CTFSentry::SentryRotate()
{
	// if we're playing a fire gesture, stop it
	if (m_Activity != ACT_SNIFF)
	{
		SetActivity(ACT_SNIFF);
	}

	// animate
	StudioFrameAdvance();

	// Look for a target
	if (FindTarget())
		return;

	// Rotate
	if (!MoveTurret())
	{
		// Change direction
		EMIT_SOUND(edict(), CHAN_AUTO, SENTRY_SOUND_IDLE, 0.5, ATTN_IDLE);

		// Switch rotation direction
		if (m_bTurningRight)
		{
			m_bTurningRight = false;
			m_vecGoalAngles.y = m_iLeftBound;
		}
		else
		{
			m_bTurningRight = true;
			m_vecGoalAngles.y = m_iRightBound;
		}

		// Randomly look up and down a bit
		if (RANDOM_FLOAT(0, 1) < 0.3)
		{
			m_vecGoalAngles.x = (int)RANDOM_LONG(-10,10);
		}
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CTFSentry::MoveTurret()
{
	bool bMoved = false;

	int iBaseTurnRate = m_iBaseTurnRate;

	// any x movement?
	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += SENTRY_THINK_DELAY * (iBaseTurnRate * 5) * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		} 
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		//SetPoseParameter(m_iPitchPoseParameter, -m_vecCurAngles.x);
		SetBoneController(BONE_PITCH, -m_vecCurAngles.x);
		m_fPitch = -m_vecCurAngles.x;

		bMoved = true;
	}

	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs(m_vecGoalAngles.y - m_vecCurAngles.y);
		bool bReversed = false;

		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
			bReversed = true;
		}

		if (m_hEnemy.Get() == NULL)
		{
			if (flDist > 30)
			{
				if (m_flTurnRate < iBaseTurnRate * 10)
				{
					m_flTurnRate += iBaseTurnRate;
				}
			}
			else
			{
				// Slow down
				if (m_flTurnRate > (iBaseTurnRate * 5))
					m_flTurnRate -= iBaseTurnRate;
			}
		}
		else
		{
			// When tracking enemies, move faster and don't slow
			if (flDist > 30)
			{
				if (m_flTurnRate < iBaseTurnRate * 30)
				{
					m_flTurnRate += iBaseTurnRate * 3;
				}
			}
		}

		m_vecCurAngles.y += SENTRY_THINK_DELAY * m_flTurnRate * flDir;

		// if we passed over the goal, peg right to it now
		if (flDir == -1)
		{
			if ((bReversed == false && m_vecGoalAngles.y > m_vecCurAngles.y) ||
				(bReversed == true && m_vecGoalAngles.y < m_vecCurAngles.y))
			{
				m_vecCurAngles.y = m_vecGoalAngles.y;
			}
		} 
		else
		{
			if ((bReversed == false && m_vecGoalAngles.y < m_vecCurAngles.y) ||
                (bReversed == true && m_vecGoalAngles.y > m_vecCurAngles.y))
			{
				m_vecCurAngles.y = m_vecGoalAngles.y;
			}
		}

		if (m_vecCurAngles.y < 0)
		{
			m_vecCurAngles.y += 360;
		}
		else if (m_vecCurAngles.y >= 360)
		{
			m_vecCurAngles.y -= 360;
		}

		if (flDist < (SENTRY_THINK_DELAY * 0.5 * iBaseTurnRate))
		{
			m_vecCurAngles.y = m_vecGoalAngles.y;
		}

		Vector angles = pev->angles;

		float flYaw = m_vecCurAngles.y - angles.y;

		//SetPoseParameter(m_iYawPoseParameter, -flYaw);
		SetBoneController(BONE_YAW, -flYaw);
		m_fYaw = -flYaw;

		bMoved = true;
	}

	if (!bMoved || m_flTurnRate <= 0)
	{
		m_flTurnRate = iBaseTurnRate * 5;
	}

	return bMoved;
}

void CTFSentry::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 10) < 1))
	{
		UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
		pev->dmgtime = gpGlobals->time;
	}

	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
}

// note last damage time
bool CTFSentry::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (flDamage > 0)
	{
		m_flLastAttackedTime = gpGlobals->time;

		pev->health -= flDamage;
		if (pev->health <= 0)
		{
			pev->takedamage = DAMAGE_NO;

			ClearBits(pev->flags, FL_MONSTER); // why are they set in the first place???

			SetUse(NULL);
			SetThink(NULL);
			ExplodeSentry();

			return false;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called when this object is destroyed
//-----------------------------------------------------------------------------
void CTFSentry::ExplodeSentry()
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BREAKMODEL);
		// position
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z+4);
		// size
		WRITE_COORD(8);
		WRITE_COORD(8);
		WRITE_COORD(8);
		// velocity
		WRITE_COORD(pev->velocity.x);
		WRITE_COORD(pev->velocity.y);
		WRITE_COORD(pev->velocity.z);
		WRITE_BYTE(25); // randomization
		// Model
		WRITE_SHORT(m_idShard); // model id#
		// # of shards
		WRITE_BYTE(5);
		// duration
		WRITE_BYTE(30); // 3.0 seconds
		WRITE_BYTE(BREAK_SMOKE); // flags
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

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, QUIET_GUN_VOLUME, 3.0);

	::RadiusDamage(pev->origin, pev, m_hBuilder != nullptr ? m_hBuilder->pev : pev, 30, 96, CLASS_NONE, DMG_BLAST);

	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSentry::SetModel(const char* pModel)
{
	SET_MODEL(edict(), pModel);

	// Reset this after model change
	UTIL_SetSize(pev, SENTRYGUN_MINS, SENTRYGUN_MAXS);
	pev->solid = SOLID_BBOX;

	// Restore pose parameters
	//m_iPitchPoseParameter = LookupPoseParameter("aim_pitch");
	//m_iYawPoseParameter = LookupPoseParameter("aim_yaw");

	//SetPoseParameter(m_iPitchPoseParameter, flPoseParam0);
	//SetPoseParameter(m_iYawPoseParameter, flPoseParam1);

	SetBoneController(BONE_PITCH,	m_fPitch);
	SetBoneController(BONE_YAW,		m_fYaw);

	ResetSequenceInfo();
}