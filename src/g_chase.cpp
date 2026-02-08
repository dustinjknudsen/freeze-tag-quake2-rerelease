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
            RemoveThirdPersonGhost(ent);
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

    // === CAMERA POSITION CALCULATION (same for all chase modes) ===
    ownerv = targ->s.origin;
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
        // Create/update ghost so player can see themselves
        CreateThirdPersonGhost(ent);
        UpdateThirdPersonGhost(ent);

        // Calculate view offset from player origin to camera position
        ent->client->ps.viewoffset[0] = goal[0] - ent->s.origin[0];
        ent->client->ps.viewoffset[1] = goal[1] - ent->s.origin[1];
        ent->client->ps.viewoffset[2] = goal[2] - ent->s.origin[2] - ent->viewheight;

        // Don't change pm_type - keep normal movement
        // Don't change viewangles - let player control their own view

        ent->client->ps.pmove.pm_flags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;

        gi.linkentity(ent);
        return;
    }
    // === END THIRD-PERSON MODE ===

    if (ent->client->frozen) {
        // Reset any stale third-person viewoffset
        ent->client->ps.viewoffset = {};
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

void ChaseNext(edict_t* ent)
{
    ent->client->auto_chase = false;
    ptrdiff_t i;
    edict_t* e;
    // If alive and not frozen, don't allow chasing others
    if (!ent->client->resp.spectator && !ent->client->frozen && ent->health > 0)
        return;
    if (!ent->client->chase_target)
    {
        // Clean up third-person if entering chase from frozen state
        if (ent->client->frozen && ent->client->thirdperson)
        {
            RemoveThirdPersonGhost(ent);
            ent->client->thirdperson = false;
            ent->client->ps.viewoffset = {};
            ent->client->ps.stats[STAT_FT_CHASE] = 0;
            ent->client->ps.stats[STAT_FT_VIEWED] = 0;
        }
        ent->client->chase_target = ent;
    }
    i = ent->client->chase_target - g_edicts;
    do
    {
        i++;
        if (i > game.maxclients)
            i = 1;
        e = g_edicts + i;
        if (!e->inuse)
            continue;
        // Spectators can chase anyone who isn't a spectator
        if (ent->client->resp.spectator)
        {
            if (!e->client->resp.spectator)
                break;
            continue;
        }
        // Frozen players can only chase living teammates
        if (ent->client->frozen)
        {
            if (e->client->resp.ctf_team != ent->client->resp.ctf_team)
                continue;
            if (e->client->frozen)
                continue;
            if (e->health > 0 && !e->client->resp.spectator)
                break;
        }
    } while (e != ent->client->chase_target);
    // If we looped back to ourselves, return to own frozen view
    if (e == ent->client->chase_target && e == ent)
    {
        ent->client->chase_target = nullptr;
        ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
        ent->client->ps.viewoffset = {};

        if (ent->client->frozen)
        {
            RemoveFrozenBodyGhost(ent);
            ent->svflags &= ~SVF_NOCLIENT;
            ent->client->ps.pmove.pm_type = PM_DEAD;
            ent->client->ps.pmove.origin = ent->s.origin;
            ent->client->ps.viewangles[ROLL] = 40;
            ent->client->ps.viewangles[PITCH] = -15;
            ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
            gi.linkentity(ent);
        }
    }
    else
    {
        ent->client->chase_target = e;
    }
    ent->client->update_chase = true;
}

void ChasePrev(edict_t* ent)
{
    ent->client->auto_chase = false;
    int i;
    edict_t* e;
    // If alive and not frozen, don't allow chasing others
    if (!ent->client->resp.spectator && !ent->client->frozen && ent->health > 0)
        return;
    if (!ent->client->chase_target)
    {
        // Clean up third-person if entering chase from frozen state
        if (ent->client->frozen && ent->client->thirdperson)
        {
            RemoveThirdPersonGhost(ent);
            ent->client->thirdperson = false;
            ent->client->ps.viewoffset = {};
            ent->client->ps.stats[STAT_FT_CHASE] = 0;
            ent->client->ps.stats[STAT_FT_VIEWED] = 0;
        }
        ent->client->chase_target = ent;
    }
    i = ent->client->chase_target - g_edicts;
    do
    {
        i--;
        if (i < 1)
            i = game.maxclients;
        e = g_edicts + i;
        if (!e->inuse)
            continue;
        // Spectators can chase anyone who isn't a spectator
        if (ent->client->resp.spectator)
        {
            if (!e->client->resp.spectator)
                break;
            continue;
        }
        // Frozen players can only chase living teammates
        if (ent->client->frozen)
        {
            if (e->client->resp.ctf_team != ent->client->resp.ctf_team)
                continue;
            if (e->client->frozen)
                continue;
            if (e->health > 0 && !e->client->resp.spectator)
                break;
        }
    } while (e != ent->client->chase_target);
    // If we looped back to ourselves, return to own frozen view
    if (e == ent->client->chase_target && e == ent)
    {
        ent->client->chase_target = nullptr;
        ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
        ent->client->ps.viewoffset = {};

        if (ent->client->frozen)
        {
            RemoveFrozenBodyGhost(ent);
            ent->svflags &= ~SVF_NOCLIENT;
            ent->client->ps.pmove.pm_type = PM_DEAD;
            ent->client->ps.pmove.origin = ent->s.origin;
            ent->client->ps.viewangles[ROLL] = 40;
            ent->client->ps.viewangles[PITCH] = -15;
            ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
            gi.linkentity(ent);
        }
    }
    else
    {
        ent->client->chase_target = e;
    }
    ent->client->update_chase = true;
}

void Cmd_AutoChase_f(edict_t* ent)
{
    if (!ent->client->resp.spectator)
    {
        gi.LocClient_Print(ent, PRINT_HIGH, "Must be a spectator to use auto-chase.\n");
        return;
    }

    ent->client->auto_chase = !ent->client->auto_chase;

    if (ent->client->auto_chase)
    {
        gi.LocClient_Print(ent, PRINT_HIGH, "Auto-chase enabled. Following top player.\n");
    }
    else
    {
        gi.LocClient_Print(ent, PRINT_HIGH, "Auto-chase disabled.\n");
        ent->client->chase_target = nullptr;
        ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
    }
}

void UpdateAutoChase(edict_t* ent)
{
    if (!ent->client->auto_chase)
        return;

    // Ensure we are a spectator
    if (!ent->client->resp.spectator)
    {
        ent->client->auto_chase = false;
        return;
    }

    edict_t* best = nullptr;
    float best_heuristic = -1;

    // --- TUNING WEIGHTS ---
    // FPM_WEIGHT: How much we care about speed/activity (Frags Per Minute)
    // KD_WEIGHT:  How much we care about survival/skill (Kill/Death Ratio)
    // currently set to favor fast active players (2.0) over campers (1.0)
    const float FPM_WEIGHT = 2.0f;
    const float KD_WEIGHT = 1.0f;

    for (uint32_t i = 1; i <= game.maxclients; i++)
    {
        edict_t* other = &g_edicts[i];

        if (!other->inuse) continue;
        if (other->client->resp.spectator) continue;
        if (other->client->resp.ctf_team == CTF_NOTEAM) continue;

        // Skip players who are dead or frozen (optional: remove this if you want to watch dead bodies)
        if (other->health <= 0 || other->client->frozen)
            continue;

        // 1. Calculate Score (Kills + Thaws)
        // If you have a separate 'thaws' counter, add it: 
        // float total_score = (float)other->client->resp.score + (float)other->client->resp.thaws;
        float total_score = (float)other->client->resp.score;

        // 2. Calculate K/D Ratio
        float deaths = (float)other->client->resp.deaths;
        float ratio_kd;

        if (deaths <= 0)
            ratio_kd = total_score; // Perfect game so far
        else
            ratio_kd = total_score / deaths;

        // 3. Calculate FPM (Frags Per Minute)
                // Subtract the gtime_t objects directly to get the duration
        auto duration = level.time - other->client->resp.entertime;

        // Divide the duration by 1_sec to convert it to a plain number (seconds)
        float duration_seconds = (float)duration.milliseconds() / 1000.0f;

        // Clamp to 60 seconds minimum to prevent massive math spikes when a player 
        // joins and gets a kill in their first 5 seconds.
        if (duration_seconds < 60.0f)
            duration_seconds = 60.0f;

        float duration_minutes = duration_seconds / 60.0f;
        float ratio_fpm = total_score / duration_minutes;

        // 4. Calculate Final Weighted Score
        float heuristic = (ratio_fpm * FPM_WEIGHT) + (ratio_kd * KD_WEIGHT);

        if (heuristic > best_heuristic)
        {
            best_heuristic = heuristic;
            best = other;
        }
    }

    // 5. Update Target
    if (best && best != ent->client->chase_target)
    {
        ent->client->chase_target = best;
        ent->client->update_chase = true;
    }

    // 6. Fallback: If 'best' is null (everyone is dead/frozen), pick anyone valid
    if (!best)
    {
        for (uint32_t i = 1; i <= game.maxclients; i++)
        {
            edict_t* other = &g_edicts[i];
            if (!other->inuse || other->client->resp.spectator) continue;

            // Just find someone alive/unfrozen
            if (other->health > 0 && !other->client->frozen)
            {
                if (other != ent->client->chase_target)
                {
                    ent->client->chase_target = other;
                    ent->client->update_chase = true;
                }
                return;
            }
        }
    }
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
