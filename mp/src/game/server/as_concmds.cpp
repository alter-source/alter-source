#include "cbase.h"
#include "props.h"
#include "datacache\imdlcache.h"
#include "tier0/memdbgon.h"

// taken from spherical.cpp
const char* s_flFactorials[] = {
	"1",
	"1",
	"2",
	"6",
	"24",
	"120",
	"720",
	"5040",
	"40320",
	"362880",
	"3628800",
	"39916800",
	"479001600",
	"6227020800",
	"87178291200",
	"1307674368000",
	"20922789888000",
	"355687428096000",
	"6402373705728000",
	"121645100408832000",
	"2432902008176640000",
	"51090942171709440000",
	"1124000727777607680000",
	"25852016738884976640000",
	"620448401733239439360000",
	"15511210043330985984000000",
	"403291461126605635584000000",
	"10888869450418352160768000000",
	"304888344611713860501504000000",
	"8841761993739701954543616000000",
	"265252859812191058636308480000000",
	"8222838654177922817725562880000000",
	"263130836933693530167218012160000000",
	"8683317618811886495518194401280000000"
};

void CC_GiveCurrentHealth()
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if (!pPlayer && !sv_cheats->GetBool())
		return;

	else
	{
		pPlayer->SetHealth(100);
	}
}

void CC_Factorial(const CCommand &args)
{
	const char* input = args.Arg(1);
	int i = atoi(input);

	if (i == 0)
	{
		Msg("%s\n", s_flFactorials[0]);
		return;
	}

	if (i == NULL || !i || i < 0)
	{
		Warning("invalid input\n");
		return;
	}

	if (i > 33)
	{
		Warning("going too high\n");
		return;
	}

	Msg("%s\n", s_flFactorials[i]);
}

void CC_CreateExplosiveBarrel(void)
{
	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache(true);

	CBaseEntity* entity = dynamic_cast<CBaseEntity*>(CreateEntityByName("prop_physics"));
	if (entity)
	{
		entity->PrecacheModel("models/props_c17/oildrum001_explosive.mdl");
		entity->SetModel("models/props_c17/oildrum001_explosive.mdl");
		entity->SetName(MAKE_STRING("barrel"));
		entity->AddSpawnFlags(SF_PHYSPROP_ENABLE_PICKUP_OUTPUT);
		entity->Precache();
		DispatchSpawn(entity);

		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors(&forward);
		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		if (tr.fraction != 1.0)
		{
			tr.endpos.z += 12;
			entity->Teleport(&tr.endpos, NULL, NULL);
			UTIL_DropToFloor(entity, MASK_SOLID);
		}
	}
	CBaseEntity::SetAllowPrecache(allowPrecache);
}

static ConCommand factorial("factorial", CC_Factorial, "factorial function", FCVAR_NONE);
static ConCommand ent_create_explosive_barrel("ent_create_explosive_barrel", CC_CreateExplosiveBarrel, "creates a explosive barrel", FCVAR_NONE);
static ConCommand giveCurrentHealth("givecurrenthealth", CC_GiveCurrentHealth, "sets the player health to 100", FCVAR_CHEAT);