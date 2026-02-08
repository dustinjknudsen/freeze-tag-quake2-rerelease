#include "m_player.h"
#include "g_freeze.h"

#define MAX_MOTD_LINES 20
#define	DotProduct(a, b)	a.dot(b)
#define	VectorSubtract(a, b, c)	c = a - b
#define	VectorAdd(a, b, c)	c = a + b
#define	VectorCopy(a, b)	b = a
#define	VectorSet(v, x, y, z)	v = {x, y, z}
#define	VectorClear(v)	v = {}
#define	VectorNormalize(x)	x.normalize()
#define	VectorScale(a, b, c)	c = a * b
#define	VectorLength(a)	a.length()
#define	VectorMA(veca, scale, vecb, vecc)	vecc = veca + (vecb * scale)

static char motd_lines[MAX_MOTD_LINES][64];
int motd_line_count = 0;

void LoadMOTD()
{
	motd_line_count = 0;

	// Try opening from the mod folder (relative to working directory)
	FILE* f = fopen("freezetag/freeze.ini", "r");
	if (!f)
	{
		gi.Com_PrintFmt("Freeze Tag: freeze.ini not found\n");
		return;
	}

	char line[256];
	bool in_motd = false;

	while (fgets(line, sizeof(line), f))
	{
		// Strip newline/carriage return
		int len = (int)strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
			line[--len] = '\0';

		// Skip comments
		if (line[0] == '/' && line[1] == '/')
			continue;

		// Section headers
		if (line[0] == '[')
		{
			in_motd = !strncmp(line, "[motd]", 6);
			continue;
		}

		if (!in_motd)
			continue;

		// End marker
		if (line[0] == '#' && line[1] == '#' && line[2] == '#')
			break;

		if (motd_line_count >= MAX_MOTD_LINES)
			break;

		Q_strlcpy(motd_lines[motd_line_count], line, sizeof(motd_lines[0]));
		motd_line_count++;
	}

	fclose(f);
	gi.Com_PrintFmt("Freeze Tag: Loaded {} MOTD lines\n", motd_line_count);
}

static pmenu_t motd_menu_data[MAX_MOTD_LINES + 2];

static void MOTDDismiss(edict_t* ent, pmenuhnd_t* p)
{
	PMenu_Close(ent);
	CTFOpenJoinMenu(ent);
}

void OpenMOTD(edict_t* ent)
{
	if (motd_line_count == 0)
	{
		CTFOpenJoinMenu(ent);
		return;
	}

	memset(motd_menu_data, 0, sizeof(motd_menu_data));

	int slot = 0;
	for (int i = 0; i < motd_line_count && slot < MAX_MOTD_LINES; i++)
	{
		Q_strlcpy(motd_menu_data[slot].text, motd_lines[i], sizeof(motd_menu_data[slot].text));
		motd_menu_data[slot].align = PMENU_ALIGN_CENTER;
		motd_menu_data[slot].SelectFunc = nullptr;
		slot++;
	}

	// Blank line
	motd_menu_data[slot].text[0] = '\0';
	motd_menu_data[slot].align = PMENU_ALIGN_CENTER;
	motd_menu_data[slot].SelectFunc = nullptr;
	slot++;

	// Dismiss option
	Q_strlcpy(motd_menu_data[slot].text, "Press ENTER to continue", sizeof(motd_menu_data[slot].text));
	motd_menu_data[slot].align = PMENU_ALIGN_CENTER;
	motd_menu_data[slot].SelectFunc = MOTDDismiss;

	int total = slot + 1;
	PMenu_Open(ent, motd_menu_data, total - 1, total, nullptr, nullptr);
}


float VectorNormalize2(const vec3_t& v, vec3_t& out) {
	float length = v.length();

	if (length > 0.0f)
		out = v / length;
	return length;
}

ctfteam_t get_team_enum(int i) {
	if (i == 0) return CTF_TEAM1; // Red
	if (i == 1) return CTF_TEAM2; // Blue
	if (i == 2) return CTF_TEAM3; // Green
	if (i == 3) return CTF_TEAM4; // Yellow
	return CTF_TEAM1;
}

uint32_t get_team_int(ctfteam_t t) {
	if (t == CTF_TEAM1) return 0; // Red
	if (t == CTF_TEAM2) return 1; // Blue
	if (t == CTF_TEAM3) return 2; // Green
	if (t == CTF_TEAM4) return 3; // Yellow
	return 0;
}

#define	hook_on	0x00000001
#define	hook_in	0x00000002
#define	shrink_on	0x00000004
#define	grow_on	0x00000008
#define	motor_off	0
#define	motor_start	1
#define	motor_on	2
#define	chan_hook	CHAN_AUX

#define	_drophook	"medic/medatck5.wav"
#define	_motorstart	"parasite/paratck2.wav"
#define	_motoron	"parasite/paratck3.wav"
#define	_motoroff	"parasite/paratck4.wav"
#define	_hooktouch	"parasite/paratck1.wav"
#define	_touchsolid	"medic/medatck3.wav"
#define	_firehook	"medic/medatck2.wav"

#define	_shotgun	0x00000001 // 1
#define	_supershotgun	0x00000002 // 2
#define	_machinegun	0x00000004 // 4
#define	_chaingun	0x00000008 // 8
#define	_grenadelauncher	0x00000010 // 16
#define	_rocketlauncher	0x00000020 // 32
#define	_hyperblaster	0x00000040 // 64
#define	_railgun	0x00000080 // 128
#define _chainfist	0x00000100 // 256
#define _etfrifle	0x00000200 // 512
#define _proxlauncher	0x00000400 // 1024
#define _ionripper	0x00000800 // 2048
#define _plasmabeam	0x00001000 // 4096
#define _phalanx	0x00002000 // 8192
#define _disruptor	0x00004000 // 16384

#define	ready_help	0x00000001
#define	thaw_help	0x00000002
#define	frozen_help	0x00000004
#define	chase_help	0x00000008

cvar_t* hook_max_len;
cvar_t* hook_rpf;
cvar_t* hook_min_len;
cvar_t* hook_speed;
cvar_t* frozen_time;
cvar_t* start_weapon;
cvar_t* start_armor;
cvar_t* grapple_wall;
static int	gib_queue;
static int	moan[8];
int team_max_count;
//==============================================================
// Third-Person Ghost Entity - for seeing yourself in third person
//==============================================================

void CreateThirdPersonGhost(edict_t* ent)
{
	// Don't create if already exists or not in third-person
	if (ent->client->thirdperson_body || !ent->client->thirdperson)
		return;

	edict_t* ghost = G_Spawn();
	if (!ghost)
		return;

	// Copy visual state from the player
	ghost->s = ent->s;
	ghost->s.number = ghost - g_edicts;

	// Set up as an intangible visual-only entity
	ghost->classname = "thirdperson_ghost";
	ghost->solid = SOLID_NOT;
	ghost->movetype = MOVETYPE_NONE;
	ghost->takedamage = false;
	ghost->owner = ent;

	// Copy position and bounds
	ghost->s.origin = ent->s.origin;
	ghost->mins = ent->mins;
	ghost->maxs = ent->maxs;

	gi.linkentity(ghost);

	ent->client->thirdperson_body = ghost;
}

void UpdateThirdPersonGhost(edict_t* ent)
{
	if (!ent->client->thirdperson_body)
		return;

	edict_t* ghost = ent->client->thirdperson_body;

	// Keep the ghost synced with the player's state
	ghost->s.origin = ent->s.origin;
	ghost->s.angles = ent->s.angles;
	ghost->s.frame = ent->s.frame;
	ghost->s.skinnum = ent->s.skinnum;
	ghost->s.effects = ent->s.effects;
	ghost->s.renderfx = ent->s.renderfx;
	ghost->s.modelindex = ent->s.modelindex;

	gi.linkentity(ghost);
}

void RemoveThirdPersonGhost(edict_t* ent)
{
	if (!ent->client->thirdperson_body)
		return;

	G_FreeEdict(ent->client->thirdperson_body);
	ent->client->thirdperson_body = nullptr;
}

//==============================================================
// Frozen Body Ghost Entity - for chase camera visibility
//==============================================================

void CreateFrozenBodyGhost(edict_t* ent)
{
	// Don't create if already exists or not frozen
	if (ent->client->frozen_body || !ent->client->frozen)
		return;

	// Don't create for bots
	if (ent->svflags & SVF_BOT)
		return;

	edict_t* ghost = G_Spawn();
	if (!ghost)
		return;

	// Copy visual state from the frozen player
	ghost->s = ent->s;
	ghost->s.number = ghost - g_edicts;

	// Set up as a intangible visual-only entity
	ghost->classname = "frozen_body_ghost";
	ghost->svflags = SVF_NONE;
	ghost->solid = SOLID_NOT;
	ghost->movetype = MOVETYPE_NONE;
	ghost->takedamage = false;
	ghost->owner = ent;

	// Copy position and bounds
	ghost->s.origin = ent->s.origin;
	ghost->mins = ent->mins;
	ghost->maxs = ent->maxs;

	gi.linkentity(ghost);

	ent->client->frozen_body = ghost;
}

void UpdateFrozenBodyGhost(edict_t* ent)
{
	if (!ent->client->frozen_body)
		return;

	edict_t* ghost = ent->client->frozen_body;

	// Keep the ghost synced with the player's frozen state
	ghost->s.origin = ent->s.origin;
	ghost->s.angles = ent->s.angles;
	ghost->s.frame = ent->s.frame;
	ghost->s.skinnum = ent->s.skinnum;
	ghost->s.effects = ent->s.effects;
	ghost->s.renderfx = ent->s.renderfx;
	ghost->s.modelindex = ent->s.modelindex;

	gi.linkentity(ghost);
}

void RemoveFrozenBodyGhost(edict_t* ent)
{
	if (!ent->client->frozen_body)
		return;

	G_FreeEdict(ent->client->frozen_body);
	ent->client->frozen_body = nullptr;
}
//==============================================================

//==============================================================
// Bot Grappling Hook for Frozen Body Rescue
//==============================================================



// Check if bot has line of sight to hook the target
bool BotCanHookTarget(edict_t* bot, edict_t* target)
{
	vec3_t start;
	start = bot->s.origin;
	start[2] += bot->viewheight;

	vec3_t end;
	end = target->s.origin;
	end[2] += 16; // Center mass of frozen body

	trace_t trace = gi.trace(start, vec3_origin, vec3_origin, end, bot, MASK_SHOT);

	if (trace.ent == target)
		return true;

	float dist = (trace.endpos - end).length();
	return dist < 32;
}

// Calculate exact aim angles to hit target
void BotCalculateHookAim(edict_t* bot, edict_t* target, vec3_t& out_angles)
{
	vec3_t start;
	start = bot->s.origin;
	start[2] += bot->viewheight - 8; // Match hook fire offset

	vec3_t end;
	end = target->s.origin;
	end[2] += 8; // Lowered from 16 - aim at lower center mass

	vec3_t aim_dir;
	VectorSubtract(end, start, aim_dir);
	VectorNormalize(aim_dir);

	out_angles = vectoangles(aim_dir);
	out_angles[PITCH] += 2.0f; // Aim 2 degrees lower (positive pitch = down)
}

// Check if bot is standing still enough to fire
bool BotIsStationary(edict_t* bot)
{
	float speed = bot->velocity.length();
	return speed < 10.0f;
}

// Stop bot movement completely
void BotStopMovement(edict_t* bot)
{
	bot->velocity[0] = 0;
	bot->velocity[1] = 0;
	// Don't zero Z velocity - let gravity work

	// Clear movement commands
	bot->client->ps.pmove.velocity[0] = 0;
	bot->client->ps.pmove.velocity[1] = 0;
}

void freezeBotHook()
{
	if (level.intermissiontime)
		return;

	edict_t* bot;

	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		bot = g_edicts + 1 + i;

		// Skip non-bots and invalid states
		if (!bot->inuse)
			continue;
		if (!(bot->svflags & SVF_BOT))
			continue;
		if (bot->client->resp.spectator)
			continue;
		if (bot->health <= 0)
			continue;
		if (bot->client->frozen)
			continue;

		// Check if bot has hook attached to a wall (not a player) - auto-detach after 5 seconds
		if (bot->client->hookstate & hook_in)
		{
			// Find the hook entity
			for (int j = 0; j < globals.num_edicts; j++)
			{
				edict_t* hook = &g_edicts[j];
				if (hook->owner == bot && hook->enemy)
				{
					// Check if hooked to a wall (BSP) rather than a player
					if (hook->enemy->solid == SOLID_BSP)
					{
						// Initialize detach timer if not set
						if (bot->client->hook_wall_time == 0_ms)
							bot->client->hook_wall_time = level.time + 5_sec;

						// Auto-detach after 5 seconds
						if (level.time > bot->client->hook_wall_time)
						{
							bot->client->hookstate = 0;
							bot->client->hook_wall_time = 0_ms;
						}
					}
					break;
				}
			}
		}
		else
		{
			// Reset wall timer when not hooked
			bot->client->hook_wall_time = 0_ms;
		}

		// Check if bot has a frozen teammate to rescue
		edict_t* frozen_ally = bot->client->bot_helper;
		if (!frozen_ally || !frozen_ally->client->frozen)
		{
			// No rescue target - reset state but preserve cooldown
			if (bot->client->hook_rescue_state != RESCUE_NONE)
				bot->client->hook_rescue_state = RESCUE_NONE;
			continue;
		}

		// Check if frozen ally is already being thawed
		if (frozen_ally->client->resp.thawer)
		{
			bot->client->hook_rescue_state = RESCUE_NONE;
			continue;
		}

		// Check if frozen ally is already being hooked by another teammate
		bool already_hooked = false;
		for (uint32_t j = 0; j < game.maxclients; j++)
		{
			edict_t* other = g_edicts + 1 + j;
			if (!other->inuse)
				continue;
			if (other == bot)
				continue;
			if (!(other->svflags & SVF_BOT))
				continue;
			if (!other->client->hookstate)
				continue;
			// Check if this bot is hooking our target
			if (other->client->hook_rescue_state >= RESCUE_FIRING &&
				other->client->bot_helper == frozen_ally)
			{
				already_hooked = true;
				break;
			}
		}

		if (already_hooked)
		{
			bot->client->hook_rescue_state = RESCUE_NONE;
			continue;
		}

		// Cooldown check - don't start new rescue attempt if on cooldown
		if (bot->client->hook_rescue_state == RESCUE_NONE &&
			bot->client->hook_rescue_time > level.time)
		{
			continue;
		}

		float dist = (bot->s.origin - frozen_ally->s.origin).length();

		// If close enough, let normal thaw logic handle it
		if (dist <= MELEE_DISTANCE + 16)
		{
			if (bot->client->hookstate & hook_on)
				bot->client->hookstate = 0;
			bot->client->hook_rescue_state = RESCUE_NONE;
			continue;
		}

		// Don't hook if too close (walk instead) or too far
		if (dist < 200 || dist > hook_max_len->value * 0.85f)
		{
			bot->client->hook_rescue_state = RESCUE_NONE;
			continue;
		}

		// Can't see target
		if (!BotCanHookTarget(bot, frozen_ally))
		{
			bot->client->hook_rescue_state = RESCUE_NONE;
			continue;
		}

		// State machine for hook rescue
		switch (bot->client->hook_rescue_state)
		{
		case RESCUE_NONE:
			// Start rescue attempt
			bot->client->hook_rescue_state = RESCUE_STOPPING;
			bot->client->hook_rescue_time = level.time + 500_ms;
			break;

		case RESCUE_STOPPING:
			// Stop moving and wait
			BotStopMovement(bot);

			if (BotIsStationary(bot) || level.time > bot->client->hook_rescue_time)
			{
				bot->client->hook_rescue_state = RESCUE_AIMING;
				bot->client->hook_rescue_time = level.time + 300_ms;
			}
			break;

		case RESCUE_AIMING:
		{
			// Stay stopped
			BotStopMovement(bot);

			// Aim precisely at target
			vec3_t aim_angles;
			BotCalculateHookAim(bot, frozen_ally, aim_angles);
			bot->s.angles[YAW] = aim_angles[YAW];
			bot->s.angles[PITCH] = 0; // Force model upright
			bot->client->v_angle = aim_angles;
			bot->client->ps.viewangles = aim_angles;

			// Wait for aim to stabilize, then fire
			if (level.time > bot->client->hook_rescue_time)
			{
				// Final aim check right before firing
				BotCalculateHookAim(bot, frozen_ally, aim_angles);
				bot->s.angles[YAW] = aim_angles[YAW];
				bot->s.angles[PITCH] = 0; // Force model upright
				bot->client->v_angle = aim_angles;
				bot->client->ps.viewangles = aim_angles;

				// Fire!
				bot->client->hookstate = hook_on;
				firehook(bot);

				bot->client->hook_rescue_state = RESCUE_FIRING;
				bot->client->hook_rescue_time = level.time + 2_sec;
			}
			break;
		}

		case RESCUE_FIRING:
			// Keep model upright while waiting for hook
			bot->s.angles[PITCH] = 0;

			// Wait for hook to connect or timeout
			if (bot->client->hookstate & hook_in)
			{
				// Check if we actually hooked our frozen ally
				bool hooked_ally = false;
				for (int j = 0; j < globals.num_edicts; j++)
				{
					edict_t* hook = &g_edicts[j];
					if (hook->owner == bot && hook->enemy == frozen_ally)
					{
						hooked_ally = true;
						break;
					}
				}

				if (hooked_ally)
				{
					// 50% chance to retreat, 50% chance to reel
					if (rand() % 2 == 0)
					{
						bot->client->hook_rescue_state = RESCUE_RETREATING;
						// Give them slightly longer to drag the body to safety
						bot->client->hook_rescue_time = level.time + 4_sec;
					}
					else
					{
						bot->client->hook_rescue_state = RESCUE_REELING;
						// Standard reel time
						//bot->client->hook_rescue_time = level.time + 5_sec; 
					}
				}
				else
				{
					// Hooked a wall - detach
					bot->client->hookstate = 0;
					bot->client->hook_rescue_state = RESCUE_NONE;
					bot->client->hook_rescue_time = level.time + 2_sec;
				}
			}
			else if (!(bot->client->hookstate & hook_on) || level.time > bot->client->hook_rescue_time)
			{
				bot->client->hook_rescue_state = RESCUE_NONE;
				bot->client->hook_rescue_time = level.time + 3_sec;
			}
			break;

		case RESCUE_REELING:
		{
			// Keep model upright while reeling
			bot->s.angles[PITCH] = 0;

			// Hook attached - reel in
			if (!(bot->client->hookstate & hook_on))
			{
				// Hook dropped - cooldown before retry
				bot->client->hook_rescue_state = RESCUE_NONE;
				bot->client->hook_rescue_time = level.time + 2_sec;
				break;
			}

			// Auto-detach hook after 5 seconds for bots
			if (level.time > bot->client->hook_rescue_time)
			{
				bot->client->hookstate = 0;
				bot->client->hook_rescue_state = RESCUE_NONE;
				bot->client->hook_rescue_time = level.time + 2_sec;
				break;
			}

			bot->client->hookstate |= shrink_on;
			bot->client->hookstate &= ~grow_on;

			// Check if close enough to thaw
			float current_dist = (bot->s.origin - frozen_ally->s.origin).length();
			if (current_dist <= MELEE_DISTANCE + 32)
			{
				bot->client->hookstate = 0;
				bot->client->hook_rescue_state = RESCUE_NONE;
				bot->client->hook_rescue_time = level.time + 1_sec; // Short cooldown after success
			}
			break;
		}

		case RESCUE_RETREATING:
		{
			// Keep model upright while retreating
			bot->s.angles[PITCH] = 0;

			// Hook attached - pull frozen ally while backing up
			if (!(bot->client->hookstate & hook_on))
			{
				// Hook dropped - cooldown before retry
				bot->client->hook_rescue_state = RESCUE_NONE;
				bot->client->hook_rescue_time = level.time + 2_sec;
				break;
			}

			// Auto-detach hook after timeout
			if (level.time > bot->client->hook_rescue_time)
			{
				bot->client->hookstate = 0;
				bot->client->hook_rescue_state = RESCUE_NONE;
				bot->client->hook_rescue_time = level.time + 2_sec;
				break;
			}

			// Reel in the frozen ally
			bot->client->hookstate |= shrink_on;
			bot->client->hookstate &= ~grow_on;

			// Make bot crouch
			bot->client->ps.pmove.pm_flags |= PMF_DUCKED;
			bot->mins[2] = -24;
			bot->maxs[2] = 4;
			bot->viewheight = -2;

			// Calculate retreat direction (away from frozen ally)
			vec3_t retreat_dir;
			VectorSubtract(bot->s.origin, frozen_ally->s.origin, retreat_dir);
			retreat_dir[2] = 0; // Keep horizontal
			VectorNormalize(retreat_dir);

			// Find a point to retreat to (200 units behind bot)
			vec3_t retreat_target;
			VectorMA(bot->s.origin, 200, retreat_dir, retreat_target);

			// Move the bot backward
			gi.Bot_MoveToPoint(bot, retreat_target, 0);

			// Check if frozen ally is close enough to thaw
			float current_dist = (bot->s.origin - frozen_ally->s.origin).length();
			if (current_dist <= MELEE_DISTANCE + 32)
			{
				// Stand back up when done
				bot->client->ps.pmove.pm_flags &= ~PMF_DUCKED;
				bot->mins[2] = -24;
				bot->maxs[2] = 32;
				bot->viewheight = 22;

				bot->client->hookstate = 0;
				bot->client->hook_rescue_state = RESCUE_NONE;
				bot->client->hook_rescue_time = level.time + 1_sec;
			}
			break;
		}

		}
	}
}

void putInventory(const char* s, edict_t* ent)
{
	gitem_t* item = nullptr;
	gitem_t* ammo = nullptr;
	int	index;

	item = FindItem(s);
	if (item)
	{
		index = item->id;
		ent->client->pers.inventory[index] = 1;

		ammo = GetItemByIndex(item->ammo);
		if (ammo)
		{
			index = ammo->id;
			ent->client->pers.inventory[index] = ammo->quantity;
		}
		ent->client->newweapon = item;
	}
}

void playerWeapon(edict_t* ent)
{
	gitem_t* item = FindItem("blaster");

	item = FindItem("blaster");
	ent->client->pers.inventory[item->id] = 1;
	ent->client->newweapon = item;

	if (start_armor->value)
	{
		int	index = FindItem("jacket armor")->id;
		ent->client->pers.inventory[index] = (int)(start_armor->value / 2) * 2;
	}

	if (start_weapon->value)
	{
		if ((int)start_weapon->value & _shotgun)
			putInventory("shotgun", ent);
		if ((int)start_weapon->value & _supershotgun)
			putInventory("super shotgun", ent);
		if ((int)start_weapon->value & _machinegun)
			putInventory("machinegun", ent);
		if ((int)start_weapon->value & _chaingun)
			putInventory("chaingun", ent);
		if ((int)start_weapon->value & _grenadelauncher)
			putInventory("grenade launcher", ent);
		if ((int)start_weapon->value & _rocketlauncher)
			putInventory("rocket launcher", ent);
		if ((int)start_weapon->value & _hyperblaster)
			putInventory("hyperblaster", ent);
		if ((int)start_weapon->value & _railgun)
			putInventory("railgun", ent);
		if ((int)start_weapon->value & _chainfist)
			putInventory("chainfist", ent);
		if ((int)start_weapon->value & _etfrifle)
			putInventory("etf rifle", ent);
		if ((int)start_weapon->value & _proxlauncher)
			putInventory("prox launcher", ent);
		if ((int)start_weapon->value & _ionripper)
			putInventory("ionripper", ent);
		if ((int)start_weapon->value & _plasmabeam)
			putInventory("plasmabeam", ent);
		if ((int)start_weapon->value & _phalanx)
			putInventory("phalanx", ent);
		if ((int)start_weapon->value & _disruptor)
			putInventory("disruptor", ent);
	}
	ChangeWeapon(ent);
}

bool playerDamage(edict_t* targ, edict_t* attacker, int damage, mod_t mod)
{
	if (!targ->client)
		return false;
	if (mod.id == MOD_TELEFRAG)
		return false;
	if (!attacker->client)
		return false;
	if (targ->client->hookstate && frandom() < 0.2)
		targ->client->hookstate = 0;
	if (targ->health > 0)
	{
		if (targ == attacker)
			return false;
		if (targ->client->resp.ctf_team != attacker->client->resp.ctf_team && targ->client->respawn_time + 3_sec < level.time)
			return false;
	}
	else
	{
		if (targ->client->frozen)
		{
			if (frandom() < 0.1)
				ThrowGib(targ, "models/objects/debris2/tris.md2", damage, GIB_NONE, 1);
			return true;
		}
		else
			return false;
	}
	if (g_friendly_fire->integer)
		return true;
	//meansOfDeath |= MOD_FRIENDLY_FIRE;
	return false;
}

bool freezeCheck(edict_t* ent, mod_t mod)
{
	if (ent->deadflag)
		return false;
	if (mod.friendly_fire)
		return false;
	switch (mod.id)
	{
	case MOD_FALLING:
	case MOD_SLIME:
	case MOD_LAVA:
		if (frandom() < 0.08)
			break;
	case MOD_SUICIDE:
	case MOD_CRUSH:
	case MOD_WATER:
	case MOD_EXIT:
	case MOD_TRIGGER_HURT:
	case MOD_BFG_LASER:
	case MOD_BFG_EFFECT:
	case MOD_TELEFRAG:
	case MOD_NUKE:
		return false;
	}
	return true;
}

void freezeAnim(edict_t* ent)
{
	// Clean up third-person ghost if player was in third-person mode
	if (ent->client->thirdperson)
	{
		RemoveThirdPersonGhost(ent);
		ent->client->thirdperson = false;
		ent->client->ps.stats[STAT_CHASE] = 0;
		ent->client->ps.stats[STAT_FT_CHASE] = 0;
		ent->client->ps.stats[STAT_FT_VIEWED] = 0;
	}

	ent->client->anim_priority = ANIM_DEATH;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		if (rand() & 1)
		{
			ent->s.frame = FRAME_crpain1 - 1;
			ent->client->anim_end = FRAME_crpain1 + rand() % 4;
		}
		else
		{
			ent->s.frame = FRAME_crdeath1 - 1;
			ent->client->anim_end = FRAME_crdeath1 + rand() % 5;
		}
	}
	else
	{
		switch (rand() % 8)
		{
		case 0:
			ent->s.frame = FRAME_run1 - 1;
			ent->client->anim_end = FRAME_run1 + rand() % 6;
			break;
		case 1:
			ent->s.frame = FRAME_pain101 - 1;
			ent->client->anim_end = FRAME_pain101 + rand() % 4;
			break;
		case 2:
			ent->s.frame = FRAME_pain201 - 1;
			ent->client->anim_end = FRAME_pain201 + rand() % 4;
			break;
		case 3:
			ent->s.frame = FRAME_pain301 - 1;
			ent->client->anim_end = FRAME_pain301 + rand() % 4;
			break;
		case 4:
			ent->s.frame = FRAME_jump1 - 1;
			ent->client->anim_end = FRAME_jump1 + rand() % 6;
			break;
		case 5:
			ent->s.frame = FRAME_death101 - 1;
			ent->client->anim_end = FRAME_death101 + rand() % 6;
			break;
		case 6:
			ent->s.frame = FRAME_death201 - 1;
			ent->client->anim_end = FRAME_death201 + rand() % 6;
			break;
		case 7:
			ent->s.frame = FRAME_death301 - 1;
			ent->client->anim_end = FRAME_death301 + rand() % 6;
			break;
		}
	}

	if (frandom() < 0.2)
		gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_BODY, gi.soundindex("boss3/d_hit.wav"), 1, ATTN_NORM, 0);
	ent->client->frozen = true;
	//ent->client->resp.deaths++;
	ent->client->frozen_time = level.time + gtime_t::from_sec(frozen_time->value);
	ent->client->resp.thawer = nullptr;
	ent->client->bot_helper = nullptr;
	ent->client->thaw_time = HOLD_FOREVER;
	if (frandom() > 0.3)
		ent->client->hookstate -= ent->client->hookstate & (grow_on | shrink_on);
	ent->deadflag = true;
	gi.linkentity(ent);
}

bool gibCheck()
{
	if (gib_queue > 35)
		return true;
	else
	{
		gib_queue++;
		return false;
	}
}

THINK(gibThink) (edict_t* ent) -> void
{
	gib_queue--;
	G_FreeEdict(ent);
}

void playerView(edict_t* ent)
{
	edict_t* other;
	vec3_t	ent_origin;
	vec3_t	forward;
	vec3_t	other_origin;
	vec3_t	dist;
	trace_t	trace;
	float	dot;
	float	other_dot;
	edict_t* best_other;

	if ((level.time.milliseconds() / 100) % 8)
		return;

	other_dot = 0.3f;
	best_other = nullptr;
	VectorCopy(ent->s.origin, ent_origin);
	ent_origin[2] += ent->viewheight;
	AngleVectors(ent->s.angles, forward, nullptr, nullptr);

	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (other->health <= 0 && !other->client->frozen)
			continue;
		VectorCopy(other->s.origin, other_origin);
		other_origin[2] += other->viewheight;
		VectorSubtract(other_origin, ent_origin, dist);
		if (VectorLength(dist) > 800)
			continue;
		trace = gi.trace(ent_origin, vec3_origin, vec3_origin, other_origin, ent, MASK_OPAQUE);
		if (trace.fraction != 1)
			continue;
		VectorNormalize(dist);
		dot = DotProduct(dist, forward);
		if (dot > other_dot)
		{
			other_dot = dot;
			best_other = other;
		}
	}
	if (best_other)
		ent->client->viewed = best_other;
	else
		ent->client->viewed = nullptr;
}

void playerBotHelper(edict_t* ent) {
	if (ent->client->bot_helper) {
		edict_t* other = ent->client->bot_helper;
		if (other->health <= 0 || other->client->frozen) {
			ent->client->bot_helper = nullptr;
			other->client->bot_helper = nullptr;
		}
		else {
			return;
		}
	}

	edict_t* other;
	float best_distance = std::numeric_limits<float>::max();
	edict_t* best_other = nullptr;
	for (uint32_t i = 0; i < game.maxclients; i++) {
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (other->health <= 0)
			continue;
		if (other->client->resp.ctf_team != ent->client->resp.ctf_team)
			continue;
		if (!(other->svflags & SVF_BOT))
			continue;
		if (other->client->bot_helper)
			continue;
		float dist = (ent->s.origin - other->s.origin).length();
		if (dist < best_distance) {
			best_distance = dist;
			best_other = other;
		}
	}
	if (best_other) {
		ent->client->bot_helper = best_other;
		best_other->client->bot_helper = ent;
	}
}

void CleanupOrphanedGhosts()
{
	for (int i = 0; i < globals.num_edicts; i++)
	{
		edict_t* e = &g_edicts[i];
		if (!e->inuse)
			continue;

		// Check if this is a ghost entity whose owner is gone
		if (e->owner && (!e->owner->inuse || !e->owner->client))
		{
			G_FreeEdict(e);
			continue;
		}
	}
}

// Bot team management
ctfteam_t bot_pending_team = CTF_NOTEAM;
int bot_pending_count = 0;

void Cmd_BotAddTeam_f()
{
	if (gi.argc() < 3) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Usage: sv bot_add <team> <count>\n");
		gi.LocClient_Print(nullptr, PRINT_HIGH, "  team: red, blue, green, yellow\n");
		gi.LocClient_Print(nullptr, PRINT_HIGH, "  count: number of bots to add\n");
		return;
	}

	const char* t = gi.argv(2);
	ctfteam_t team;

	if (Q_strcasecmp(t, "red") == 0)
		team = CTF_TEAM1;
	else if (Q_strcasecmp(t, "blue") == 0)
		team = CTF_TEAM2;
	else if (Q_strcasecmp(t, "green") == 0)
		team = CTF_TEAM3;
	else if (Q_strcasecmp(t, "yellow") == 0)
		team = CTF_TEAM4;
	else {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Unknown team: {}. Use red, blue, green, yellow.\n", t);
		return;
	}

	if (gi.argc() < 4) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Please specify a number of bots to add.\n");
		return;
	}

	int count = atoi(gi.argv(3));
	if (count < 1) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Count must be at least 1.\n");
		return;
	}

	// Check available slots
	int used = 0;
	for (uint32_t i = 1; i <= game.maxclients; i++)
		if (g_edicts[i].inuse)
			used++;

	int available = game.maxclients - used;
	if (count > available) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Only {} slots available (requested {}).\n", available, count);
		count = available;
	}

	if (count <= 0) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "No available client slots.\n");
		return;
	}

	// Set pending team so CTFAssignTeam picks it up
	bot_pending_team = team;
	bot_pending_count = count;

	// Use engine's addbot command for each bot
	for (int i = 0; i < count; i++)
		gi.AddCommandString("addbot\n");

	gi.LocClient_Print(nullptr, PRINT_HIGH, "Adding {} bot(s) to {} team.\n", count, CTFTeamName(team));
}

void freezeBotDrown()
{
	if (level.intermissiontime)
		return;

	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		edict_t* bot = g_edicts + 1 + i;
		if (!bot->inuse)
			continue;
		if (!(bot->svflags & SVF_BOT))
			continue;
		if (bot->client->resp.spectator)
			continue;
		if (bot->health <= 0)
			continue;
		if (bot->client->frozen)
			continue;
		if (bot->waterlevel < WATER_UNDER)
			continue;

		// Bot is fully submerged — check if running low on air
		gtime_t air_left = bot->air_finished - level.time;

		if (air_left > 3_sec)
			continue; // Still have plenty of air

		// Urgent: try to hook out of water
		if (air_left < 2_sec && !(bot->client->hookstate & hook_on))
		{
			// Look straight up and fire hook
			bot->client->v_angle[PITCH] = -80;
			bot->client->ps.viewangles[PITCH] = -80;
			bot->s.angles[PITCH] = 0;

			// Trace upward to find a ceiling/surface to hook
			vec3_t start, end;
			start = bot->s.origin;
			start[2] += bot->viewheight;
			end = start;
			end[2] += hook_max_len->value * 0.7f;

			trace_t trace = gi.traceline(start, end, bot, MASK_SOLID);
			if (trace.fraction < 1.0f)
			{
				// Found something to hook — fire!
				bot->client->hookstate = hook_on;
				firehook(bot);
			}
		}

		// If hooked, reel in
		if (bot->client->hookstate & hook_in)
		{
			bot->client->hookstate |= shrink_on;
			bot->client->hookstate &= ~grow_on;

			// If we surfaced, release hook
			if (bot->waterlevel < WATER_UNDER)
			{
				bot->client->hookstate = 0;
			}
		}

		// Always try to swim upward when low on air
		if (air_left < 3_sec)
		{
			// Find a point above us
			vec3_t escape_point;
			escape_point = bot->s.origin;
			escape_point[2] += 256;

			gi.Bot_MoveToPoint(bot, escape_point, 0);

			// Add upward velocity to help swim
			if (bot->velocity[2] < 200)
				bot->velocity[2] += 50;
		}
	}
}

void Cmd_BotKickTeam_f()
{
	if (gi.argc() < 3) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Usage: sv bot_kick <team> [count]\n");
		gi.LocClient_Print(nullptr, PRINT_HIGH, "  team: red, blue, green, yellow, all\n");
		gi.LocClient_Print(nullptr, PRINT_HIGH, "  count: number to kick (default 1, 0 = all on team)\n");
		gi.LocClient_Print(nullptr, PRINT_HIGH, "  'sv bot_kick all' kicks all bots\n");
		return;
	}
	const char* t = gi.argv(2);
	ctfteam_t team = CTF_NOTEAM;
	bool kick_all_teams = false;
	if (Q_strcasecmp(t, "red") == 0)
		team = CTF_TEAM1;
	else if (Q_strcasecmp(t, "blue") == 0)
		team = CTF_TEAM2;
	else if (Q_strcasecmp(t, "green") == 0)
		team = CTF_TEAM3;
	else if (Q_strcasecmp(t, "yellow") == 0)
		team = CTF_TEAM4;
	else if (Q_strcasecmp(t, "all") == 0)
		kick_all_teams = true;
	else {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Unknown team: {}. Use red, blue, green, yellow, all.\n", t);
		return;
	}

	int count = 1;
	bool kick_all_on_team = false;

	if (gi.argc() >= 4)
	{
		count = atoi(gi.argv(3));
		if (count == 0)
			kick_all_on_team = true;
	}
	else if (kick_all_teams)
	{
		// "sv bot_kick all" with no count = kick all bots
		kick_all_on_team = true;
	}

	// Build list of bot indices to kick (reverse order to avoid index shifting issues)
	int kicked = 0;
	for (int i = game.maxclients; i >= 1; i--)
	{
		edict_t* ent = &g_edicts[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		if (!(ent->svflags & SVF_BOT))
			continue;
		if (!ent->client->pers.connected)
			continue;
		if (!kick_all_teams && ent->client->resp.ctf_team != team)
			continue;
		if (!kick_all_on_team && kicked >= count)
			break;
		// Clean up freeze tag state
		if (ent->client->frozen) {
			RemoveFrozenBodyGhost(ent);
			ent->client->frozen = false;
			freeze[get_team_int(ent->client->resp.ctf_team)].update = true;
		}
		if (ent->client->thirdperson) {
			RemoveThirdPersonGhost(ent);
			ent->client->thirdperson = false;
		}
		if (ent->client->bot_helper) {
			ent->client->bot_helper->client->bot_helper = nullptr;
			ent->client->bot_helper = nullptr;
		}
		// Kick by client number (i - 1 is the client index)
		gi.AddCommandString(G_Fmt("kick {}\n", i - 1).data());
		kicked++;
	}
	if (kicked == 0)
	{
		if (kick_all_teams)
			gi.LocClient_Print(nullptr, PRINT_HIGH, "No bots found.\n");
		else
			gi.LocClient_Print(nullptr, PRINT_HIGH, "No bots found on {} team.\n", CTFTeamName(team));
	}
	else
	{
		if (kick_all_teams)
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Kicked {} bot(s) from all teams. Wait before adding new bots.\n", kicked);
		else
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Kicked {} bot(s) from {} team. Wait before adding new bots.\n", kicked, CTFTeamName(team));
	}
	// Clean up any orphaned ghost entities
	CleanupOrphanedGhosts();
}

void freezeBotHookTaxi()
{
	if (level.intermissiontime)
		return;

	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		edict_t* bot = g_edicts + 1 + i;
		if (!bot->inuse)
			continue;
		if (!(bot->svflags & SVF_BOT))
			continue;
		if (bot->client->resp.spectator)
			continue;
		if (bot->health <= 0)
			continue;
		if (bot->client->frozen)
			continue;

		// Skip if bot is already in a hook rescue sequence
		if (bot->client->hook_rescue_state != RESCUE_NONE)
			continue;

		// Skip if hook is already in use
		if (bot->client->hookstate & hook_on)
			continue;

		// Skip if on hook cooldown
		if (bot->client->hook_rescue_time > level.time)
			continue;

		// Find any frozen teammate nearby (not just assigned helper)
		edict_t* best_frozen = nullptr;
		float best_dist = 999999;

		for (uint32_t j = 0; j < game.maxclients; j++)
		{
			edict_t* other = g_edicts + 1 + j;
			if (!other->inuse)
				continue;
			if (!other->client->frozen)
				continue;
			if (other->client->resp.ctf_team != bot->client->resp.ctf_team)
				continue;

			// Already being thawed by someone
			if (other->client->resp.thawer)
				continue;

			// Already being hooked by someone else
			bool already_hooked = false;
			for (uint32_t k = 0; k < game.maxclients; k++)
			{
				edict_t* hooker = g_edicts + 1 + k;
				if (!hooker->inuse || hooker == bot)
					continue;
				if ((hooker->client->hookstate & hook_on) && hooker->client->bot_helper == other)
				{
					already_hooked = true;
					break;
				}
			}
			if (already_hooked)
				continue;

			float dist = (bot->s.origin - other->s.origin).length();

			// Sweet spot: close enough to hook accurately while running
			// but not so close that walking would be faster
			if (dist < 150 || dist > hook_max_len->value * 0.6f)
				continue;

			if (dist < best_dist)
			{
				best_dist = dist;
				best_frozen = other;
			}
		}

		if (!best_frozen)
			continue;

		// Check line of sight
		if (!BotCanHookTarget(bot, best_frozen))
			continue;

		// Check bot is actually moving (not camping)
		float speed = bot->velocity.length();
		if (speed < 100)
			continue;

		// Fire hook at frozen teammate without stopping!
		vec3_t aim_angles;
		BotCalculateHookAim(bot, best_frozen, aim_angles);
		bot->client->v_angle = aim_angles;
		bot->client->ps.viewangles = aim_angles;
		bot->s.angles[YAW] = aim_angles[YAW];
		bot->s.angles[PITCH] = 0;

		bot->client->hookstate = hook_on;
		firehook(bot);

		// Set a short timer to auto-release
		bot->client->hook_rescue_time = level.time + 3_sec;
		bot->client->hook_taxi_time = level.time + 3_sec;
	}

	// Handle active taxi hooks — reel in while keeping momentum
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		edict_t* bot = g_edicts + 1 + i;
		if (!bot->inuse)
			continue;
		if (!(bot->svflags & SVF_BOT))
			continue;
		if (bot->client->hook_taxi_time == 0_ms)
			continue;

		// Taxi expired or hook dropped
		if (level.time > bot->client->hook_taxi_time || !(bot->client->hookstate & hook_on))
		{
			if (bot->client->hookstate & hook_on)
				bot->client->hookstate = 0;
			bot->client->hook_taxi_time = 0_ms;
			bot->client->hook_rescue_time = level.time + 3_sec; // Cooldown
			continue;
		}

		// Hook connected — reel in the frozen body while bot keeps moving
		if (bot->client->hookstate & hook_in)
		{
			bot->client->hookstate |= shrink_on;
			bot->client->hookstate &= ~grow_on;
		}
	}
}

void freezeBotHelper() {

	if (level.intermissiontime)
		return;

	edict_t* ent;
	for (uint32_t i = 0; i < game.maxclients; i++) {
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->client->resp.spectator)
			continue;
		if (ent->client->frozen)
			continue;
		if (!(ent->svflags & SVF_BOT))
			continue;
		if (!ent->client->bot_helper || !ent->client->bot_helper->client->frozen)
		{
			ent->client->bot_rescue_urgent = false;
			continue;
		}

		// Move toward frozen teammate
		gi.Bot_MoveToPoint(ent, ent->client->bot_helper->s.origin, 0);

		// Urgent rescue: suppress combat behavior
		if (ent->client->bot_rescue_urgent)
		{
			// Clear enemy so bot doesn't stop to fight
			ent->enemy = nullptr;
			ent->oldenemy = nullptr;

			// Don't fire weapons - keep moving
			ent->client->buttons &= ~BUTTON_ATTACK;
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			// Check if rescue complete
			float dist = (ent->s.origin - ent->client->bot_helper->s.origin).length();
			if (dist <= MELEE_DISTANCE + 16)
			{
				ent->client->bot_rescue_urgent = false;
			}
		}
	}
}

void playerThaw(edict_t* ent)
{
	edict_t* other;
	int	j;
	vec3_t	eorg;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (other->health <= 0)
			continue;
		if (other->client->resp.ctf_team != ent->client->resp.ctf_team)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = ent->s.origin[j] - (other->s.origin[j] + (other->mins[j] + other->maxs[j]) * 0.5);
		if (VectorLength(eorg) > MELEE_DISTANCE + 5)
			continue;
		if (!(other->client->resp.help & thaw_help))
		{
			other->client->showscores = false;
			other->client->resp.help |= thaw_help;
			gi.LocCenter_Print(other, "Wait here a second to free them.");
			gi.sound(other, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
		}
		ent->client->resp.thawer = other;
		if (ent->client->thaw_time == HOLD_FOREVER)
		{
			ent->client->thaw_time = level.time + 3_sec;
			gi.sound(ent, CHAN_BODY, gi.soundindex("world/steam3.wav"), 1, ATTN_NORM, 0);
		}
		// Stop bot movement and crouch while thawing
		if (other->svflags & SVF_BOT)
		{
			other->velocity[0] = 0;
			other->velocity[1] = 0;
			other->client->ps.pmove.pm_flags |= PMF_DUCKED;
			other->mins[2] = -24;
			other->maxs[2] = 4;
			other->viewheight = -2;
		}
		return;
	}
	// Uncrouch bot if it was thawing and left range
	if (ent->client->resp.thawer && (ent->client->resp.thawer->svflags & SVF_BOT))
	{
		ent->client->resp.thawer->client->ps.pmove.pm_flags &= ~PMF_DUCKED;
		ent->client->resp.thawer->mins[2] = -24;
		ent->client->resp.thawer->maxs[2] = 32;
		ent->client->resp.thawer->viewheight = 22;
	}
	ent->client->resp.thawer = nullptr;
	ent->client->thaw_time = HOLD_FOREVER;
}

void playerBreak(edict_t* ent, int force)
{
	int	n;
	ent->client->respawn_time = level.time + 1_sec;
	if (ent->waterlevel == 3)
		gi.sound(ent, CHAN_BODY, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
	else
		gi.sound(ent, CHAN_BODY, gi.soundindex("world/brkglas.wav"), 1, ATTN_NORM, 0);
	n = rand() % (gib_queue > 10 ? 5 : 3);
	if (rand() & 1)
	{
		switch (n)
		{
		case 0:
			ThrowGib(ent, "models/objects/gibs/arm/tris.md2", force, GIB_NONE, 1);
			break;
		case 1:
			ThrowGib(ent, "models/objects/gibs/bone/tris.md2", force, GIB_NONE, 1);
			break;
		case 2:
			ThrowGib(ent, "models/objects/gibs/bone2/tris.md2", force, GIB_NONE, 1);
			break;
		case 3:
			ThrowGib(ent, "models/objects/gibs/chest/tris.md2", force, GIB_NONE, 1);
			break;
		case 4:
			ThrowGib(ent, "models/objects/gibs/leg/tris.md2", force, GIB_NONE, 1);
			break;
		}
	}
	while (n--)
		ThrowGib(ent, "models/objects/debris1/tris.md2", force, GIB_NONE, 1);
	ent->takedamage = false;
	ent->movetype = MOVETYPE_TOSS;
	ThrowClientHead(ent, force);
	ent->client->frozen = false;
	freeze[get_team_int(ent->client->resp.ctf_team)].update = true;
	ent->client->ps.stats[STAT_CHASE] = 0;
	ent->client->ps.stats[STAT_FT_CHASE] = 0;
	ent->client->ps.stats[STAT_FT_VIEWED] = 0;
	// Clean up ghost entity and restore visibility
	RemoveFrozenBodyGhost(ent);
	RemoveThirdPersonGhost(ent);
	ent->client->thirdperson = false;
	ent->svflags &= ~SVF_NOCLIENT;
	// Clear ALL bot helpers pointing at this player and uncrouch bots
	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		edict_t* other = &g_edicts[i];
		if (!other->inuse)
			continue;
		if (other->client->bot_helper == ent)
		{
			// Uncrouch bot if it was thawing
			if (other->svflags & SVF_BOT)
			{
				other->client->ps.pmove.pm_flags &= ~PMF_DUCKED;
				other->mins[2] = -24;
				other->maxs[2] = 32;
				other->viewheight = 22;
			}
			other->client->bot_helper = nullptr;
		}
	}
	ent->client->bot_helper = nullptr;
}

void playerUnfreeze(edict_t* ent)
{
	if (level.time > ent->client->frozen_time && level.time > ent->client->respawn_time)
	{
		playerBreak(ent, 50);
		return;
	}
	if (ent->waterlevel == 3 && !((level.time.milliseconds() / 100) % 4))
		ent->client->frozen_time -= 150_ms;
	if (level.time > ent->client->thaw_time)
	{
		if (!ent->client->resp.thawer || !ent->client->resp.thawer->inuse)
		{
			ent->client->resp.thawer = nullptr;
			ent->client->thaw_time = HOLD_FOREVER;
		}
		else
		{
			ent->client->resp.thawer->client->resp.score++;
			ent->client->resp.thawer->client->resp.thawed++;
			freeze[get_team_int(ent->client->resp.ctf_team)].thawed++;
			if (rand() & 1)
				gi.LocBroadcast_Print(PRINT_HIGH, "{} thaws {} like a package of frozen peas.\n", ent->client->resp.thawer->client->pers.netname, ent->client->pers.netname);
			else
				gi.LocBroadcast_Print(PRINT_HIGH, "{} evicts {} from their igloo.\n", ent->client->resp.thawer->client->pers.netname, ent->client->pers.netname);
			playerBreak(ent, 100);
		}
	}
}

void playerMove(edict_t* ent)
{
	edict_t* other;
	vec3_t	forward;
	float	dist;
	int	j;
	vec3_t	eorg;

	if (ent->client->hookstate)
		return;
	AngleVectors(ent->s.angles, forward, nullptr, nullptr);
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other->client->resp.spectator)
			continue;
		if (other == ent)
			continue;
		if (!other->client->frozen)
			continue;
		if (other->client->resp.ctf_team == ent->client->resp.ctf_team)
			continue;
		if (other->client->hookstate)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = ent->s.origin[j] - (other->s.origin[j] + (other->mins[j] + other->maxs[j]) * 0.5);
		dist = VectorLength(eorg);
		if (dist > MELEE_DISTANCE)
			continue;
		VectorScale(forward, 600, other->velocity);
		other->velocity[2] = 200;
		gi.linkentity(other);
	}
}

void freezeMain(edict_t* ent)
{
	if (!ent->inuse)
		return;
	playerView(ent);
	if (ent->client->resp.spectator)
		return;
	if (ent->client->frozen)
	{
		playerBotHelper(ent);
		playerThaw(ent);
		playerUnfreeze(ent);

		// Update chase camera for frozen players
		if (ent->client->chase_target)
			UpdateChaseCam(ent);
	}
	else if (ent->health > 0)
		playerMove(ent);
}

void freezeIntermission(void)
{
	int	i, j, k;
	int	team;
	i = j = k = 0;
	// Check scores (Loop 4 times)
	for (i = 0; i < 4; i++)
		if (get_team_score(i) > j)
			j = get_team_score(i);
	// Find winners (Loop 4 times)
	for (i = 0; i < 4; i++)
		if (get_team_score(i) == j)
		{
			k++;
			team = i;
		}
	if (k > 1)
	{
		i = j = k = 0;
		// Tie breaker on thaws (Loop 4 times)
		for (i = 0; i < 4; i++)
			if (freeze[i].thawed > j)
				j = freeze[i].thawed;
		for (i = 0; i < 4; i++)
			if (freeze[i].thawed == j)
			{
				k++;
				team = i;
			}
	}
	if (k != 1)
	{
		gi.LocBroadcast_Print(PRINT_CENTER, "Stalemate!\n");
		gi.LocBroadcast_Print(PRINT_HIGH, "Stalemate!\n");
		return;
	}
	gi.LocBroadcast_Print(PRINT_CENTER, "{} TEAM IS THE WINNER!\n", freeze_team[team]);
	gi.LocBroadcast_Print(PRINT_HIGH, "{} TEAM IS THE WINNER!\n", freeze_team[team]);
}

void playerHealth(edict_t* ent)
{
	ent->client->pers.inventory.fill(0);

	ent->client->quad_time = 0_ms;
	ent->client->invincible_time = 0_ms;
	ent->flags &= ~FL_POWER_ARMOR;

	ent->health = ent->client->pers.max_health;

	ent->s.sound = 0;
	ent->client->weapon_sound = 0;
}

void breakTeam(int team)
{
	edict_t* ent;
	gtime_t	break_time;

	break_time = level.time;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->client->frozen)
		{
			// Only break frozen players from the losing team when 3+ teams are active
			if (ent->client->resp.ctf_team != get_team_enum(team) && team_max_count >= 3)
				continue;
			ent->client->frozen_time = break_time;
			break_time += 250_ms;
			continue;
		}
		// Only restore health/weapons when less than 3 teams
		if (ent->health > 0 && team_max_count < 3)
		{
			playerHealth(ent);
			playerWeapon(ent);
		}
	}
	freeze[team].break_time = break_time + 1_sec;
	if (rand() & 1)
		gi.LocBroadcast_Print(PRINT_HIGH, "{} team was run circles around by their foe.\n", freeze_team[team]);
	else
		gi.LocBroadcast_Print(PRINT_HIGH, "{} team was less than a match for their foe.\n", freeze_team[team]);
}

void updateTeam(int team)
{
	edict_t* ent;
	int	frozen, alive, total_players;
	int	play_sound = 0;
	frozen = alive = total_players = 0;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->client->resp.spectator)
			continue;
		if (ent->client->resp.ctf_team != get_team_enum(team))
			continue;
		total_players++;
		if (ent->client->frozen)
			frozen++;
		if (ent->health > 0)
			alive++;
	}
	freeze[team].frozen = frozen;
	freeze[team].alive = alive;
	// Team remaining count for all players on this team
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->client->resp.spectator)
			continue;
		if (ent->client->resp.ctf_team != get_team_enum(team))
			continue;
		int cs_index = CONFIG_CTF_PLAYER_NAME + 50 + team;
		gi.configstring(cs_index, G_Fmt("Team Remaining: {}", alive).data());
		ent->client->ps.stats[STAT_LAST_ALIVE] = cs_index;
		// Sound warning when last alive
		if (alive == 1 && frozen > 0 && !ent->client->frozen && ent->health > 0)
		{
			if (ent->client->last_alive_warn_time == 0_ms)
			{
				ent->client->last_alive_warn_time = level.time;
				//gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
			}
		}
		else
		{
			ent->client->last_alive_warn_time = 0_ms;
		}
	}
	if (total_players > 0 && !alive)
	{
		// Loop 4 times to award points to all surviving teams
		for (int i = 0; i < 4; i++)
		{
			if (freeze[i].alive)
			{
				play_sound++;
				G_AdjustTeamScore(get_team_enum(i), 1);
				freeze[i].update = true;
			}
		}
		breakTeam(team);
		if (play_sound <= 1)
			gi.positioned_sound(vec3_origin, world, CHAN_VOICE | CHAN_RELIABLE, gi.soundindex("world/xian1.wav"), 1, ATTN_NONE, 0);
	}
}

bool endCheck()
{
	int	i;

	// Update team_max_count based on player counts
	int total[4] = { 0, 0, 0, 0 };

	for (uint32_t j = 0; j < game.maxclients; j++)
	{
		edict_t* ent = g_edicts + 1 + j;
		if (!ent->inuse)
			continue;
		if (ent->client->resp.spectator)
			continue;

		if (ent->client->resp.ctf_team == CTF_TEAM1)
			total[0]++;
		else if (ent->client->resp.ctf_team == CTF_TEAM2)
			total[1]++;
		else if (ent->client->resp.ctf_team == CTF_TEAM3)
			total[2]++;
		else if (ent->client->resp.ctf_team == CTF_TEAM4)
			total[3]++;
	}

	// Determine how many teams are active
// Determine how many teams are active (have players)
	if (total[3] > 0)
		team_max_count = 4;
	else if (total[2] > 0)
		team_max_count = 3;
	else
		team_max_count = 2;

	// Update teams
	for (i = 0; i < 4; i++)
		if (level.time > freeze[i].last_update)
		{
			updateTeam(i);
			freeze[i].update = false;
			freeze[i].last_update = level.time + 3_sec;
		}

	// Dynamic point limit based on number of teams
	if (capturelimit->value)
	{
		int point_limit;

		if (team_max_count >= 4)
			point_limit = 15;
		else if (team_max_count >= 3)
			point_limit = 12;
		else
			point_limit = (int)capturelimit->value;  // Default 

		for (i = 0; i < 4; i++)
			if (get_team_score(i) >= point_limit)
				return true;
	}

	return false;
}

void cmdMoan(edict_t* ent)
{
	if (!(ent->client->resp.help & frozen_help) && !(ent->svflags & SVF_BOT))
	{
		ent->client->showscores = false;
		ent->client->resp.help |= frozen_help;
		gi.LocCenter_Print(ent, "You have been frozen.\nWait to be saved. \nUse '[' or ']' for Chasecam.");
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
	}
	//else if (!(ent->client->chase_target || ent->client->resp.help & chase_help) && !(ent->svflags & SVF_BOT))
	//{
	//	GetChaseTarget(ent);
	//	ent->client->showscores = false;
	//	ent->client->resp.help |= chase_help;
	//	gi.LocCenter_Print(ent, "Use the chase camera with\nyour inventory keys.");
	//	gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_STATIC, 0);
	//	return;
	//}
	if (ent->client->moan_time > level.time)
		return;
	ent->client->moan_time = level.time + 2_sec;
	if (ent->svflags & SVF_BOT)
		ent->client->moan_time += random_time(5_sec, 30_sec);
	if (ent->waterlevel == 3)
	{
		if (rand() & 1)
			gi.sound(ent, CHAN_AUTO, gi.soundindex("flipper/flpidle1.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_AUTO, gi.soundindex("flipper/flpsrch1.wav"), 1, ATTN_NORM, 0);
	}
	else
		gi.sound(ent, CHAN_AUTO, moan[rand() % 8], 1, ATTN_NORM, 0);
}

void playerShell(edict_t* ent, ctfteam_t team)
{
	ent->s.effects |= EF_COLOR_SHELL;
	if (team == CTF_TEAM1)
		ent->s.renderfx |= RF_SHELL_RED;
	else if (team == CTF_TEAM2)
		ent->s.renderfx |= RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE;
	else if (team == CTF_TEAM3)
		ent->s.renderfx |= RF_SHELL_GREEN;
	else
		ent->s.renderfx |= RF_SHELL_RED | RF_SHELL_GREEN;
}

bool powerupBlinkVisible()
{
	// 100ms on, 100ms off
	return ((level.time.milliseconds() / 100) % 3) == 0;
}

void freezeEffects(edict_t* ent)
{
	if (level.intermissiontime)
		return;
	if (!ent->client->frozen)
		return;
	if (!ent->client->resp.thawer || ((level.time.milliseconds() / 100) % 16) < 8)
		playerShell(ent, ent->client->resp.ctf_team);
}

void FreezeScoreboardMessage(edict_t* ent, edict_t* killer)
{
	if (!ent || !ent->client) return;

	struct ScoreboardPlayer {
		int  client_num;
		int  team;
		int  frags;
		int  thaws;
		int  score;
		int  ping;
		bool is_frozen;
		bool is_spectator;
	};

	std::vector<ScoreboardPlayer> all_players;
	all_players.reserve(game.maxclients);

	for (uint32_t i = 0; i < game.maxclients; i++) {
		edict_t* p_ent = g_edicts + 1 + i;
		if (!p_ent->inuse || !p_ent->client || !p_ent->client->pers.connected)
			continue;

		gclient_t* client = p_ent->client;

		int team_id = 0;
		if (client->resp.ctf_team == CTF_TEAM1) team_id = 1;
		else if (client->resp.ctf_team == CTF_TEAM2) team_id = 2;
		else if (client->resp.ctf_team == CTF_TEAM3) team_id = 3;
		else if (client->resp.ctf_team == CTF_TEAM4) team_id = 4;

		int total_score = client->resp.score;
		int thaws = client->resp.thawed;
		int frags = total_score - thaws;

		all_players.push_back({
			(int)i, team_id, frags, thaws, total_score, client->ping,
			client->frozen ? true : false,
			client->resp.spectator ? true : false
			});
	}

	// Sort players
	std::sort(all_players.begin(), all_players.end(), [](const ScoreboardPlayer& a, const ScoreboardPlayer& b) {
		int teamA = (a.team == 0) ? 99 : a.team;
		int teamB = (b.team == 0) ? 99 : b.team;
		if (teamA != teamB) return teamA < teamB;
		return a.score > b.score;
		});

	// --- Determine Winning Team (Only during Intermission) ---
	int winning_team = 0;
	if (level.intermissiontime)
	{
		int max_score = -999999;
		int count = (team_max_count < 2) ? 2 : team_max_count;

		// Use actual team scores, same as freezeIntermission
		for (int i = 0; i < count; i++) {
			if (get_team_score(i) > max_score) {
				max_score = get_team_score(i);
				winning_team = i + 1; // team_totals is 1-indexed
			}
		}

		// Check for ties
		int tie_count = 0;
		for (int i = 0; i < count; i++) {
			if (get_team_score(i) == max_score) tie_count++;
		}

		// If tied, use thaw tiebreaker (same as freezeIntermission)
		if (tie_count > 1)
		{
			winning_team = 0;
			max_score = -1;
			for (int i = 0; i < count; i++) {
				if (freeze[i].thawed > max_score) {
					max_score = freeze[i].thawed;
					winning_team = i + 1;
				}
			}
			// Check thaw ties too
			tie_count = 0;
			for (int i = 0; i < count; i++) {
				if (freeze[i].thawed == max_score) tie_count++;
			}
			if (tie_count > 1) winning_team = 0;
		}
	}
	// -----------------------------------------------------------

	static std::string string;
	string.clear();

	// Header
	if (capturelimit->integer) {
		int point_limit;

		if (team_max_count >= 4)
			point_limit = 15;
		else if (team_max_count >= 3)
			point_limit = 12;
		else
			point_limit = capturelimit->integer;

		fmt::format_to(std::back_inserter(string), FMT_STRING("xl 3 yv -47 loc_string2 1 $g_score_captures \"{}\" "), point_limit);
	}
	if (timelimit->value) {
		int frames_left = gi.ServerFrame() + ((gtime_t::from_min(timelimit->value) - level.time)).milliseconds() / gi.frame_time_ms;
		fmt::format_to(std::back_inserter(string), FMT_STRING("xl 310 yv -47 time_limit {} "), frames_left);
	}

	/*
	Layout position commands :

		xv / yv = relative to screen center
		xl / yt = relative to left / top edge
		xr / yb = relative to right / bottom edge

	*/

	// "freeze_sb <num_players> <winning_team>"
	fmt::format_to(std::back_inserter(string), FMT_STRING("xl 2 yv -40 freeze_sb {} {} "), all_players.size(), winning_team);

	for (const auto& p : all_players) {
		if (string.size() > 1300) break;
		int team_id = p.is_spectator ? 0 : p.team;
		fmt::format_to(std::back_inserter(string), FMT_STRING("{} {} {} {} {} {} {} "),
			p.client_num, p.score, p.frags, p.thaws, p.ping, p.is_frozen ? 1 : 0, team_id);
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(string.c_str());
}

void p_projectsourcereverse(gclient_t* client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t& result)
{
	vec3_t	_distance;

	VectorCopy(distance, _distance);

	if (client->pers.hand == LEFT_HANDED)
		;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	else
		_distance[1] *= -1;
	result = G_ProjectSource(point, _distance, forward, right);
}

void drophook(edict_t* ent)
{
	ent->owner->client->hookstate = 0;
	ent->owner->client->hooker = 0;
	gi.sound(ent->owner, chan_hook, gi.soundindex(_drophook), 1, ATTN_IDLE, 0);
	G_FreeEdict(ent);
}

void maintainlinks(edict_t* ent)
{
	float	multiplier;
	vec3_t	norm_hookvel, pred_hookpos;
	vec3_t	forward, right, offset;
	vec3_t	start = {}, chainvec;
	vec3_t	norm_chainvec;

	multiplier = VectorLength(ent->velocity) / 22;
	VectorNormalize2(ent->velocity, norm_hookvel);
	VectorMA(ent->s.origin, multiplier, norm_hookvel, pred_hookpos);

	AngleVectors(ent->owner->client->v_angle, forward, right, nullptr);
	VectorSet(offset, 8, 8, (float)ent->owner->viewheight - 8);
	p_projectsourcereverse(ent->owner->client, ent->owner->s.origin, offset, forward, right, start);
	VectorSubtract(pred_hookpos, start, chainvec);
	VectorNormalize2(chainvec, norm_chainvec);
	VectorMA(pred_hookpos, -20, norm_chainvec, pred_hookpos);
	VectorMA(start, 10, norm_chainvec, start);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_MEDIC_CABLE_ATTACK);
	gi.WriteShort(ent - g_edicts);
	gi.WritePosition(pred_hookpos);
	gi.WritePosition(start);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);
}

THINK(hookbehavior) (edict_t* ent) -> void
{
	edict_t* targ;
	bool	chain_moving;
	vec3_t	forward, right;
	vec3_t	offset, start = {};
	vec3_t	end;
	vec3_t	chainvec, velpart;
	float	chainlen;
	float	force;
	int	_hook_rpf = hook_rpf->value;

	if (!_hook_rpf)
		_hook_rpf = 80;

	targ = ent->owner;
	if (!targ->inuse || !(targ->client->hookstate & hook_on) || ent->enemy->solid == SOLID_NOT ||
		(targ->health <= 0 && !targ->client->frozen) || level.intermissiontime || targ->s.event == EV_PLAYER_TELEPORT)
	{
		drophook(ent);
		return;
	}

	VectorCopy(ent->enemy->velocity, ent->velocity);

	chain_moving = false;
	if (targ->client->hookstate & grow_on && ent->angle < hook_max_len->value)
	{
		ent->angle += _hook_rpf;
		if (ent->angle > hook_max_len->value)
			ent->angle = hook_max_len->value;
		chain_moving = true;
	}
	if (targ->client->hookstate & shrink_on && ent->angle > hook_min_len->value)
	{
		ent->angle -= _hook_rpf;
		if (ent->angle < hook_min_len->value)
			ent->angle = hook_min_len->value;
		chain_moving = true;
	}
	if (chain_moving)
	{
		if (ent->sounds == motor_off)
		{
			gi.sound(targ, chan_hook, gi.soundindex(_motorstart), 1, ATTN_IDLE, 0);
			ent->sounds = motor_start;
		}
		else if (ent->sounds == motor_start)
		{
			gi.sound(targ, chan_hook, gi.soundindex(_motoron), 1, ATTN_IDLE, 0);
			ent->sounds = motor_on;
		}
	}
	else if (ent->sounds != motor_off)
	{
		gi.sound(targ, chan_hook, gi.soundindex(_motoroff), 1, ATTN_IDLE, 0);
		ent->sounds = motor_off;
	}

	AngleVectors(ent->owner->client->v_angle, forward, right, nullptr);
	VectorSet(offset, 8, 8, (float)ent->owner->viewheight - 8);
	p_projectsourcereverse(ent->owner->client, ent->owner->s.origin, offset, forward, right, start);

	targ = nullptr;
	if (ent->enemy->client)
	{
		targ = ent->enemy;
		if (!targ->inuse || (targ->health <= 0 && !targ->client->frozen) ||
			(targ->client->buttons & 4 && frandom() < 0.3) || targ->s.event == EV_PLAYER_TELEPORT)
		{
			drophook(ent);
			return;
		}
		VectorCopy(ent->s.origin, end);
		VectorCopy(start, ent->s.origin);
		VectorCopy(end, start);
		targ = ent->owner;
		ent->owner = ent->enemy;
		ent->enemy = targ;
	}

	VectorSubtract(ent->s.origin, start, chainvec);
	chainlen = VectorLength(chainvec);
	if (chainlen > ent->angle)
	{
		VectorScale(chainvec, DotProduct(ent->owner->velocity, chainvec) / DotProduct(chainvec, chainvec), velpart);
		force = (chainlen - ent->angle) * 5;
		if (DotProduct(ent->owner->velocity, chainvec) < 0)
		{
			if (chainlen > ent->angle + 25)
				VectorSubtract(ent->owner->velocity, velpart, ent->owner->velocity);
		}
		else
		{
			if (VectorLength(velpart) < force)
				force -= VectorLength(velpart);
			else
				force = 0;
		}
	}
	else
		force = 0;

	VectorNormalize(chainvec);
	VectorMA(ent->owner->velocity, force, chainvec, ent->owner->velocity);
	SV_CheckVelocity(ent->owner);

	if (targ)
	{
		targ = ent->enemy;
		ent->enemy = ent->owner;
		ent->owner = targ;
		VectorCopy(ent->enemy->s.origin, ent->s.origin);
	}
	else if (!ent->owner->client->resp.old_hook &&
		ent->owner->client->hookstate & shrink_on && chain_moving)
		ent->owner->velocity -= ent->owner->gravityVector * (ent->owner->gravity * level.gravity * gi.frame_time_s);
	maintainlinks(ent);
	ent->nextthink = level.time + FRAME_TIME_S;
}

TOUCH(hooktouch) (edict_t* ent, edict_t* other, const trace_t& tr, bool other_touching_self) -> void
{
	vec3_t	forward, right;
	vec3_t	offset, start = {};
	vec3_t	chainvec;

	AngleVectors(ent->owner->client->v_angle, forward, right, nullptr);
	VectorSet(offset, 8, 8, (float)ent->owner->viewheight - 8);
	p_projectsourcereverse(ent->owner->client, ent->owner->s.origin, offset, forward, right, start);
	VectorSubtract(ent->s.origin, start, chainvec);
	ent->angle = VectorLength(chainvec);
	if (tr.surface && tr.surface->flags & SURF_SKY)
	{
		drophook(ent);
		return;
	}
	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, tr.plane.normal, ent->dmg, 100, DAMAGE_NONE, MOD_HIT);
	if (other->solid == SOLID_BBOX)
	{
		if (other->client)
		{
			// Don't hook living teammates (but allow hooking frozen teammates for rescue)
			if (other->client->resp.ctf_team == ent->owner->client->resp.ctf_team && !other->client->frozen)
			{
				drophook(ent);
				return;
			}

			if (ent->owner->client->hooker < 2)
			{
				ent->owner->client->hooker++;
				other->s.origin[2] += 9;
				gi.sound(ent, CHAN_VOICE, gi.soundindex(_hooktouch), 1, ATTN_IDLE, 0);
			}
			else
			{
				drophook(ent);
				return;
			}
		}
		else
		{
			drophook(ent);
			return;
		}
	}
	else if (other->solid == SOLID_BSP && grapple_wall->value)
	{
		if (!ent->owner->client->resp.old_hook)
			VectorClear(ent->owner->velocity);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_SHOTGUN);
		gi.WritePosition(ent->s.origin);
		if (!tr.plane.normal)
			gi.WriteDir(vec3_origin);
		else
			gi.WriteDir(tr.plane.normal);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);
		gi.sound(ent, CHAN_VOICE, gi.soundindex(_touchsolid), 1, ATTN_IDLE, 0);
	}
	else
	{
		drophook(ent);
		return;
	}
	VectorCopy(other->velocity, ent->velocity);
	ent->owner->client->hookstate |= hook_in;
	ent->enemy = other;
	ent->touch = nullptr;
	ent->think = hookbehavior;
	ent->nextthink = level.time + FRAME_TIME_S;
}

THINK(hookairborne) (edict_t* ent) -> void
{
	vec3_t	chainvec;
	float	chainlen;

	VectorSubtract(ent->s.origin, ent->owner->s.origin, chainvec);
	chainlen = VectorLength(chainvec);
	if (!(ent->owner->client->hookstate & hook_on) || chainlen > hook_max_len->value)
	{
		drophook(ent);
		return;
	}
	maintainlinks(ent);
	ent->nextthink = level.time + FRAME_TIME_S;
}

void firehook(edict_t* ent)
{
	int	damage;
	vec3_t	forward, right;
	vec3_t	offset, start = {};
	edict_t* newhook;

	damage = 10;

	AngleVectors(ent->client->v_angle, forward, right, nullptr);
	VectorSet(offset, 8, 8, (float)ent->viewheight - 8);
	p_projectsourcereverse(ent->client, ent->s.origin, offset, forward, right, start);

	newhook = G_Spawn();
	newhook->svflags = SVF_DEADMONSTER;
	VectorCopy(start, newhook->s.origin);
	VectorCopy(start, newhook->s.old_origin);
	newhook->s.angles = vectoangles(forward);
	VectorScale(forward, hook_speed->value, newhook->velocity);
	VectorCopy(forward, newhook->movedir);
	newhook->movetype = MOVETYPE_FLYMISSILE;
	newhook->clipmask = MASK_SHOT;
	newhook->solid = SOLID_BBOX;
	VectorClear(newhook->mins);
	VectorClear(newhook->maxs);
	newhook->owner = ent;
	newhook->dmg = damage;
	newhook->sounds = 0;
	newhook->touch = hooktouch;
	newhook->think = hookairborne;
	newhook->nextthink = level.time + FRAME_TIME_S;
	gi.linkentity(newhook);
	gi.sound(ent, chan_hook, gi.soundindex(_firehook), 1, ATTN_IDLE, 0);
}

void cmdHook(edict_t* ent)
{
	const char* s;
	int* hookstate;

	s = gi.argv(1);
	if (!*s)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "hook <value> [action / stop / grow / shrink] : control hook\n");
		if (ent->client->resp.old_hook)
			ent->client->resp.old_hook = false;
		else
			ent->client->resp.old_hook = true;
		return;
	}
	if (ent->health <= 0 || ent->client->resp.spectator)
		return;
	hookstate = &ent->client->hookstate;
	if (!(*hookstate & hook_on) && Q_strcasecmp(s, "action") == 0)
	{
		*hookstate = hook_on;
		firehook(ent);
		return;
	}
	if (*hookstate & hook_on)
	{
		if (Q_strcasecmp(s, "action") == 0)
		{
			*hookstate = 0;
			return;
		}
		if (Q_strcasecmp(s, "stop") == 0)
		{
			if (ent->client->resp.old_hook || ent->client->hooker)
				*hookstate -= *hookstate & (grow_on | shrink_on);
			else
				*hookstate = 0;
			return;
		}
		if (Q_strcasecmp(s, "grow") == 0)
		{
			*hookstate |= grow_on;
			*hookstate -= *hookstate & shrink_on;
			return;
		}
		if (Q_strcasecmp(s, "shrink") == 0)
		{
			*hookstate |= shrink_on;
			*hookstate -= *hookstate & grow_on;
			return;
		}
	}
}

void freezeSpawn()
{
	int	i;

	memset(freeze, 0, sizeof(freeze));
	for (i = 0; i < 4; i++) {
		freeze[i].update = true;
		freeze[i].last_update = level.time;
	}
	gib_queue = 0;
	team_max_count = 2;

	moan[0] = gi.soundindex("insane/insane1.wav");
	moan[1] = gi.soundindex("insane/insane2.wav");
	moan[2] = gi.soundindex("insane/insane3.wav");
	moan[3] = gi.soundindex("insane/insane4.wav");
	moan[4] = gi.soundindex("insane/insane6.wav");
	moan[5] = gi.soundindex("insane/insane8.wav");
	moan[6] = gi.soundindex("insane/insane9.wav");
	moan[7] = gi.soundindex("insane/insane10.wav");

	gi.configstring(CS_GENERAL + 5, ">");
}

void cvarFreeze()
{
	hook_max_len = gi.cvar("hook_max_len", "1000", CVAR_NOSET);
	hook_rpf = gi.cvar("hook_rpf", "19", CVAR_NOSET);
	hook_min_len = gi.cvar("hook_min_len", "40", CVAR_NOSET);
	hook_speed = gi.cvar("hook_speed", "970", CVAR_NOSET);
	frozen_time = gi.cvar("frozen_time", "180", CVAR_NOSET);
	start_weapon = gi.cvar("start_weapon", "0", CVAR_NOSET);
	start_armor = gi.cvar("start_armor", "0", CVAR_NOSET);
	grapple_wall = gi.cvar("grapple_wall", "1", CVAR_NOSET);
	LoadMOTD();
}

bool humanPlaying(edict_t* ent) {
	edict_t* other;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		other = g_edicts + 1 + i;
		if (!other->inuse)
			continue;
		if (other == ent)
			continue;
		if (other->svflags & SVF_BOT)
			continue;
		return true;
	}
	return false;
}
