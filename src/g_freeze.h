#pragma once

#include "g_local.h" 

//================================================================================================
// SECTION 1: CORE DATA STRUCTURES AND CONSTANTS
//================================================================================================

// Freeze Tag Team Status (Limited to 2 teams, but index 0 and 1 are used for Red/Blue)
struct {
	int	thawed;
	bool	update;
	gtime_t	last_update;
	int	frozen;
	int	alive;
	gtime_t	break_time;
} freeze[2];

// Team Names Array 
static const char* freeze_team[] = { "Red", "Blue", "Green", "Yellow", "None" };

//================================================================================================
// SECTION 2: ENUM/TYPE CONVERSION FUNCTIONS
// (Mapping between enumeration types and integer indices)
//================================================================================================

// Note: Assumes ctfteam_t is the enumeration type used for teams (e.g., Red, Blue)
ctfteam_t get_team_enum(int i);
uint32_t get_team_int(ctfteam_t t);

// Hook rescue states
enum hook_rescue_state_t {
	RESCUE_NONE,        // Not attempting rescue
	RESCUE_STOPPING,    // Coming to a stop
	RESCUE_AIMING,      // Stopped, lining up shot
	RESCUE_FIRING,      // Firing hook
	RESCUE_REELING      // Hook attached, pulling
};

// Item hook states
enum item_hook_state_t {
	ITEM_HOOK_NONE,
	ITEM_HOOK_STOPPING,
	ITEM_HOOK_AIMING,
	ITEM_HOOK_FIRING,
	ITEM_HOOK_SWINGING
};

//================================================================================================
// SECTION 3: PLAYER PHYSICS AND DAMAGE HOOKS
//================================================================================================

bool playerDamage(edict_t* targ, edict_t* attacker, int damage, mod_t mod);
bool freezeCheck(edict_t* ent, mod_t mod); // Checks if damage should result in freezing

//================================================================================================
// SECTION 4: EXTERNAL CONFIGURATION VARIABLES (CVARS)
//================================================================================================

extern cvar_t* hook_max_len;
extern cvar_t* hook_rpf;
extern cvar_t* hook_min_len;
extern cvar_t* hook_speed;
extern cvar_t* frozen_time;
extern cvar_t* start_weapon;
extern cvar_t* start_armor;
extern cvar_t* grapple_wall;

//================================================================================================
// SECTION 5: CORE GAME FLOW AND STATE MANAGEMENT
//================================================================================================

void freezeMain(edict_t* ent);
void freezeIntermission(void);
bool endCheck();      // Checks for win conditions and triggers intermission
void freezeSpawn();   // Handles player spawning/respawning
void cvarFreeze();    // Initializes or refreshes CVars
bool humanPlaying(edict_t* ent); // Checks if the entity is a human player (not a bot/spectator)

//================================================================================================
// SECTION 6: PLAYER ACTIONS, ANIMATIONS, AND VISUALS
//================================================================================================

void playerWeapon(edict_t* ent); // Gives player their starting weapons
void freezeAnim(edict_t* ent);   // Sets player animation upon freezing
void cmdMoan(edict_t* ent);      // Triggers the "Help me!" sound/message for frozen players
void playerShell(edict_t* ent, ctfteam_t team); // Applies the team-colored ice shell effect
void freezeEffects(edict_t* ent); // Updates effects like thaw pulse or solid shell
void FreezeScoreboardMessage(edict_t* ent, edict_t* killer);

//================================================================================================
// SECTION 7: GIB/SHATTER LOGIC
//================================================================================================

bool gibCheck();    // Logic to prevent too many gibs from existing at once
void gibThink(edict_t* ent); // Frees the memory for a gib entity

//================================================================================================
// SECTION 8: BOT AND UTILITY FUNCTIONS
//================================================================================================

void freezeBotHelper(); // Function related to bot logic/management
void freezeBotHook();
//void freezeBotItemHook();
// 
//================================================================================================
// SECTION 9: GRAPPLE HOOK DYNAMICS
//================================================================================================

void cmdHook(edict_t* ent); // Player command to fire or retract the hook
void firehook(edict_t* ent); // Spawns and initializes the hook entity
//================================================================================================
// SECTION 10: FROZEN BODY GHOST ENTITY (CHASE CAMERA)
//================================================================================================

void CreateFrozenBodyGhost(edict_t* ent); // Spawns the spectator/chase target
void UpdateFrozenBodyGhost(edict_t* ent); // Updates ghost position/state
void RemoveFrozenBodyGhost(edict_t* ent); // Cleans up the ghost entity on thaw/death