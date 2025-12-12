// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "g_freeze.h"

void UpdateChaseCam(edict_t* ent)
{
	vec3_t       o, ownerv, goal;
	edict_t* targ;
	vec3_t       forward, right;
	trace_t      trace;
	vec3_t       oldgoal;
	vec3_t       angles;

	// is our chase target gone?
	if (!ent->client->chase_target->inuse || ent->client->chase_target->client->resp.spectator ||
		(ent->client->frozen && (ent->client->frozen_time < level.time + 2_sec || ent->client->thaw_time < level.time + 1_sec)) ||
		(ent->health < 0 && !ent->client->frozen))
	{
		ent->client->chase_target = nullptr;
		ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);

		// Remove ghost when exiting chase mode
		if (ent->client->frozen)
			RemoveFrozenBodyGhost(ent);

		return;
	}

	targ = ent->client->chase_target;

	ownerv = targ->s.origin;
	oldgoal = ent->s.origin;

	ownerv[2] += targ->viewheight;

	angles = targ->client->v_angle;
	if (angles[PITCH] > 56)
		angles[PITCH] = 56;

	AngleVectors(angles, forward, right, nullptr);
	forward.normalize();
	o = ownerv + (forward * -30);

	if (o[2] < targ->s.origin[2] + 20)
		o[2] = targ->s.origin[2] + 20;

	// jump animation lifts
	if (!targ->groundentity)
		o[2] += 16;

	// Trace from target's viewpoint to desired camera position
	trace = gi.traceline(ownerv, o, targ, MASK_SOLID);

	goal = trace.endpos;
	goal += (forward * 2); // Back off slightly

	// pad for floors and ceilings
	// Check ceiling clip
	o = goal;
	o[2] += 6;
	trace = gi.traceline(goal, o, targ, MASK_SOLID);
	if (trace.fraction < 1)
	{
		goal = trace.endpos;
		goal[2] -= 6;
	}

	// Check floor clip
	o = goal;
	o[2] -= 6;
	trace = gi.traceline(goal, o, targ, MASK_SOLID);
	if (trace.fraction < 1)
	{
		goal = trace.endpos;
		goal[2] += 6;
	}

	//// Show chase target name when switching
	//if (ent->client->update_chase) {
	//	gi.Center_Print(ent, "Chasing: {}", targ->client->pers.netname);
	//	ent->client->update_chase = false;
	//}

	if (ent->client->frozen) {
		// Create ghost if it doesn't exist
		CreateFrozenBodyGhost(ent);

		// Update ghost visual state
		UpdateFrozenBodyGhost(ent);

		// Hide the real player entity from this client's view
		ent->svflags |= SVF_NOCLIENT;

		// Move camera to chase position
		ent->client->ps.pmove.origin = goal;
		ent->client->ps.pmove.pm_type = PM_SPECTATOR;
		ent->client->ps.viewoffset = {};

	}
	else {
		if (targ->deadflag)
			ent->client->ps.pmove.pm_type = PM_DEAD;
		else
			ent->client->ps.pmove.pm_type = PM_FREEZE;

		ent->s.origin = goal;
	}

	ent->client->ps.pmove.delta_angles = targ->client->v_angle - ent->client->resp.cmd_angles;

	if (targ->deadflag)
	{
		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = targ->client->killer_yaw;
	}
	else
	{
		ent->client->ps.viewangles = targ->client->v_angle;
		ent->client->ps.viewangles[ROLL] = 0;  // Keep camera level

		ent->client->v_angle = targ->client->v_angle;
		AngleVectors(ent->client->v_angle, ent->client->v_forward, nullptr, nullptr);
	}

	ent->viewheight = 0;

	// Always disable prediction to prevent jitter
	ent->client->ps.pmove.pm_flags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;

	gi.linkentity(ent);
}

void ChaseNext(edict_t *ent)
{
	ptrdiff_t i;
	edict_t	*e;

	if (!ent->client->chase_target)
		/* freeze
		return;
		freeze */
		ent->client->chase_target = ent;
		/* freeze */

	i = ent->client->chase_target - g_edicts;
	do
	{
		i++;
		if (i > game.maxclients)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		/* freeze */
		if (!ent->client->resp.spectator && e->client->resp.ctf_team != ent->client->resp.ctf_team)
			continue;
		if (e->client->frozen && e != ent)
			continue;
		/* freeze */
		if (!e->client->resp.spectator)
			break;
	} while (e != ent->client->chase_target);

	/* freeze */
	if (e == ent) {
		ent->client->chase_target = nullptr;
		ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
	}
	else
	/* freeze */
	ent->client->chase_target = e;
	ent->client->update_chase = true;
}

void ChasePrev(edict_t *ent)
{
	int		 i;
	edict_t *e;

	if (!ent->client->chase_target)
		/* freeze
		return;
		freeze */
		ent->client->chase_target = ent;
		/* freeze */

	i = ent->client->chase_target - g_edicts;
	do
	{
		i--;
		if (i < 1)
			i = game.maxclients;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		/* freeze */
		if (!ent->client->resp.spectator && e->client->resp.ctf_team != ent->client->resp.ctf_team)
			continue;
		if (e->client->frozen && e != ent)
			continue;
		/* freeze */
		if (!e->client->resp.spectator)
			break;
	} while (e != ent->client->chase_target);

	/* freeze */
	if (e == ent) {
		ent->client->chase_target = nullptr;
		ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
	}
	else
	/* freeze */
	ent->client->chase_target = e;
	ent->client->update_chase = true;
}

void GetChaseTarget(edict_t *ent)
{
	/* freeze
	uint32_t i;
	edict_t *other;

	for (i = 1; i <= game.maxclients; i++)
	{
		other = g_edicts + i;
		if (other->inuse && !other->client->resp.spectator)
		{
			ent->client->chase_target = other;
			ent->client->update_chase = true;
			UpdateChaseCam(ent);
			return;
		}
	}
	freeze */
	ChaseNext(ent);

	if (ent->client->chase_target)
		UpdateChaseCam(ent);
	else
	/* freeze */

	if (ent->client->chase_msg_time <= level.time)
	{
		gi.LocCenter_Print(ent, "$g_no_players_chase");
		ent->client->chase_msg_time = level.time + 5_sec;
	}
}
