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
    vec3_t       angles;

    // Check for third-person self-chase
    bool selfChase = (ent->client->thirdperson && ent->client->chase_target == ent);

    if (selfChase)
    {
        // Exit third-person if dead (and not frozen)
        if (ent->health <= 0 && !ent->client->frozen)
        {
            ent->client->thirdperson = false;
            ent->client->chase_target = nullptr;
            ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
            return;
        }
        targ = ent;
    }
    else
    {
        // Original chase target validation for spectating others
        if (!ent->client->chase_target->inuse || ent->client->chase_target->client->resp.spectator ||
            (ent->client->frozen && (ent->client->frozen_time < level.time + 2_sec || ent->client->thaw_time < level.time + 1_sec)) ||
            (ent->health < 0 && !ent->client->frozen))
        {
            ent->client->chase_target = nullptr;
            ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);

            if (ent->client->frozen)
                RemoveFrozenBodyGhost(ent);

            return;
        }
        targ = ent->client->chase_target;
    }

    ownerv = targ->s.origin;

    // On first chase frame, use target's origin as starting point
    // (skip this check for self-chase since we're always at our own origin)
    if (!selfChase)
    {
        vec3_t oldgoal = ent->s.origin;
        if (oldgoal == vec3_origin || ent->client->update_chase)
            oldgoal = targ->s.origin;
    }

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
    o = goal;
    o[2] += 6;
    trace = gi.traceline(goal, o, targ, MASK_SOLID);
    if (trace.fraction < 1)
    {
        goal = trace.endpos;
        goal[2] -= 6;
    }

    o = goal;
    o[2] -= 6;
    trace = gi.traceline(goal, o, targ, MASK_SOLID);
    if (trace.fraction < 1)
    {
        goal = trace.endpos;
        goal[2] += 6;
    }

    // === THIRD-PERSON SELF-CHASE MODE ===
    if (selfChase)
    {
        // Custom camera positioning for third-person
        vec3_t cam_offset;

        angles = targ->client->v_angle;
        if (angles[PITCH] > 56)
            angles[PITCH] = 56;

        AngleVectors(angles, forward, right, nullptr);

        // Start at player origin + eye height
        ownerv = targ->s.origin;
        ownerv[2] += targ->viewheight - 8;  // Slightly lower than eye level

        // Position camera behind player
        o = ownerv + (forward * -50);  // 50 units behind (adjust to taste)

        // Minimum height
        if (o[2] < targ->s.origin[2] + 10)
            o[2] = targ->s.origin[2] + 10;

        // Trace to avoid going through walls
        trace = gi.traceline(ownerv, o, targ, MASK_SOLID);
        goal = trace.endpos;
        goal += (forward * 2);

        // Create ghost if it doesn't exist
        CreateThirdPersonGhost(ent);

        // Update ghost visual state
        UpdateThirdPersonGhost(ent);

        // Calculate view offset from player origin to camera position
        ent->client->ps.viewoffset[0] = goal[0] - ent->s.origin[0];
        ent->client->ps.viewoffset[1] = goal[1] - ent->s.origin[1];
        ent->client->ps.viewoffset[2] = goal[2] - ent->s.origin[2];

        // Disable prediction to prevent jitter
        ent->client->ps.pmove.pm_flags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;

        gi.linkentity(ent);
        return;
    }
    // === END THIRD-PERSON MODE ===

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
        ent->client->ps.pmove.origin = goal;
        ent->client->ps.viewoffset = {};
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
        ent->client->ps.viewangles[ROLL] = 0;

        if ((targ->svflags & SVF_BOT) && targ->client->hook_rescue_state >= RESCUE_AIMING)
            ent->client->ps.viewangles[PITCH] = 0;

        ent->client->v_angle = ent->client->ps.viewangles;
        AngleVectors(ent->client->v_angle, ent->client->v_forward, nullptr, nullptr);
    }

    ent->viewheight = 0;

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

void GetChaseTarget(edict_t* ent)
{
	//gi.Com_PrintFmt("GetChaseTarget called\n");

	ChaseNext(ent);

	//gi.Com_PrintFmt("After ChaseNext, chase_target={}\n",
	//	ent->client->chase_target ? "valid" : "null");

	if (ent->client->chase_target)
	{
		//gi.Com_PrintFmt("BEFORE: viewheight={}, pmove.viewheight={}, viewoffset[2]={}\n",
		//	ent->viewheight,
		//	ent->client->ps.pmove.viewheight,
		//	ent->client->ps.viewoffset[2]);

		// Clear stale view data before first chase
		ent->viewheight = 0;
		ent->client->ps.pmove.viewheight = 0;
		ent->client->ps.viewoffset = {};
		ent->client->ps.pmove.origin = ent->client->chase_target->s.origin;
		ent->s.origin = ent->client->chase_target->s.origin;

		UpdateChaseCam(ent);

		//gi.Com_PrintFmt("AFTER: viewheight={}, pmove.viewheight={}, viewoffset[2]={}\n",
		//	ent->viewheight,
		//	ent->client->ps.pmove.viewheight,
		//	ent->client->ps.viewoffset[2]);
	}
	else

		if (ent->client->chase_msg_time <= level.time)
		{
			gi.LocCenter_Print(ent, "$g_no_players_chase");
			ent->client->chase_msg_time = level.time + 5_sec;
		}
}
