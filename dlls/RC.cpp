#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "RC.h"

// controllable RC car
LINK_ENTITY_TO_CLASS(pl_rc, CRC);
void CRC::RC_Create(unsigned int RCDamage, Vector VecSpawnPos, Vector vecDir, int RCType, CBasePlayer* owner)
{
    // Create a new entity with CRC private data
    CRC* pRC = GetClassPtr((CRC*)NULL);
    pRC->pev->classname = MAKE_STRING("pl_rc");
    pRC->m_BulletDamage = BLLTDamage;
    pRC->m_SpawnPos = VecSpawnPos;
    pRC->m_direction = vecDir;
    pRC->m_Flare = FlareType; // tracer type
    pRC->Owner = shooter;
    pRC->pev->owner = NULL;

    pRC->Spawn();

    return pRC;
}