// pointsHTF.cpp
//

#include "bzfsAPI.h"

const char* teamToFlagType(bz_eTeamType team) {
    if (team == eRedTeam) {
        return "R*";
    } else if (team == eGreenTeam) {
        return "G*";
    } else if (team == eBlueTeam) {
        return "B*";
    } else if (team == ePurpleTeam) {
        return "P*";
    } else {
        return "US";
    }
}

bz_eTeamType flagToTeamValue(const char* flagType) {
    if (strcmp("R*", flagType) == 0) {
        return eRedTeam;
    } else if (strcmp("G*", flagType) == 0) {
        return eGreenTeam;
    } else if (strcmp("B*", flagType) == 0) {
        return eBlueTeam;
    } else if (strcmp("P*", flagType) == 0) {
        return ePurpleTeam;
    }  else {
        return eNoTeam;
    }
}


class pointsHTF : public bz_Plugin
{
public:
    virtual const char* Name () {return "pointsHTF";}
    virtual void Init(const char* config);
    virtual void Event(bz_EventData *eventData);
    // Some variables for keeping track of score and related.
    int playerCapValue[200] = { 0 };
    int playerSelfCapValue[200] = { 0 };
    int cappingPlayer = -1;
    int alive = -1;
    bz_eTeamType cappingTeam = eNoTeam;
    bz_eTeamType basePoint;
    // Actually the above could be a single two dimensional array, but for now, this makes it easier to read.

int isInRange(int player)
{
    if ((player >= 0) && (player <= 199))
        return 1;
    else 
        return 0;
}

void resetPlayerCaps(int playerID) 
{
    if (isInRange(playerID) == 1) {
        playerCapValue[playerID]     = 0;
        playerSelfCapValue[playerID] = 0;
    }
}

void resetAllPlayerCaps(void)
{
    for (int i = 0; i < 200; i++)
    {
        resetPlayerCaps(i);
    }
}

void setPlayerScores(int playerID) 
{
    if (isInRange(playerID) == 1) {
        bz_setPlayerWins(playerID, playerCapValue[playerID]);
        bz_setPlayerLosses(playerID, playerSelfCapValue[playerID]);
    }
}

void resetAllPlayersScore(void) 
{
    bz_APIIntList *player_list = bz_newIntList();
    bz_getPlayerIndexList(player_list);
    
    for (unsigned int i = 0; i < player_list->size(); i++) 
    {
        if (bz_getPlayerTeam(player_list->get(i)) != eNoTeam) {// If not equal to eNoTeam, assume valid player. (Inefficient to some extent, since we have observers "reset", but it works.)
            bz_setPlayerTKs(i, 0);
            bz_setPlayerLosses(i, 0);
            bz_setPlayerWins(i, 0);
        }
    }
    bz_deleteIntList(player_list);

}

};

BZ_PLUGIN(pointsHTF)

void pointsHTF::Init( const char* /*commandLine*/ )
{
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPartEvent);
    Register(bz_ePlayerDieEvent);
    Register(bz_eCaptureEvent);
    Register(bz_eGameStartEvent);
    Register(bz_eGameEndEvent);
    Register(bz_ePlayerScoreChanged);
    Register(bz_eAllowCTFCaptureEvent);
    Register(bz_eTickEvent);
    Register(bz_ePlayerSpawnEvent);
    MaxWaitTime = 0.2f;
}

void pointsHTF::Event ( bz_EventData *eventData )
{
    switch (eventData->eventType) 
    {
        case bz_ePlayerJoinEvent: {
        bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;
        
        resetPlayerCaps(joinData->playerID);

    }break;
        
        case bz_ePlayerPartEvent: {
        bz_PlayerJoinPartEventData_V1* partData = (bz_PlayerJoinPartEventData_V1*)eventData;
        resetPlayerCaps(partData->playerID);
        if (partData->playerID == cappingPlayer) {
            alive = -1;
            cappingPlayer = -1;
            cappingTeam = eNoTeam;
            basePoint = eNoTeam;    
        }

    }break;
    
    case bz_ePlayerDieEvent: {
        bz_PlayerDieEventData_V2* deathData = (bz_PlayerDieEventData_V2*)eventData;
        
        int player = deathData->playerID;
        int killer = deathData->killerID;
        
        if (isInRange(player) == 1) {
            if ((player == cappingPlayer) && (cappingPlayer != -1)) {
                alive = -1;
            }
            if (bz_getPlayerWins(player) != playerCapValue[player]) {
                bz_setPlayerWins(player, playerCapValue[player]);
            }
            
            if (bz_getPlayerLosses(player) != playerSelfCapValue[player]) {
                bz_setPlayerLosses(player, playerSelfCapValue[player]);
            }
        }
        
        if (isInRange(killer) == 1)  {
            if (bz_getPlayerWins(killer) != playerCapValue[killer]) {
                bz_setPlayerWins(killer, playerCapValue[killer]);
            }
            
            if (bz_getPlayerLosses(killer) != playerSelfCapValue[killer]) {
                bz_setPlayerLosses(killer, playerSelfCapValue[killer]);
            }
        }

    }break;
    
    
    case bz_eCaptureEvent: {
        bz_CTFCaptureEventData_V1* capData = (bz_CTFCaptureEventData_V1*)eventData;
        
        bz_eTeamType capTeam = capData->teamCapping;
        bz_eTeamType cappedTeam = capData->teamCapped;
        
        int capPlayer = capData->playerCapping;
        bz_eTeamType playerTeam = bz_getPlayerTeam(capPlayer);
 
        if (isInRange(capPlayer) == 1) { // Because we can only update valid slots.        
            if (capData->playerCapping == cappingPlayer) {
              if (capData->teamCapping == cappingTeam) {
                  bz_killPlayer(capData->playerCapping, true, BZ_SERVER);
                  cappingPlayer = -1;
                  alive = -1;
                  cappingTeam = eNoTeam;
                  basePoint = eNoTeam;
              }
            } 
            // Update arrays.
            if ((playerTeam != eNoTeam) && (playerTeam != cappedTeam)) {
                playerCapValue[capPlayer]++;
                bz_setPlayerWins(capPlayer, playerCapValue[capPlayer]);
            } else {
                playerSelfCapValue[capPlayer]++;
                bz_setPlayerLosses(capPlayer, playerSelfCapValue[capPlayer]);
            }
            // Update player Scores.
        }

    }break;
    
    case bz_eGameStartEvent: {
        bz_GameStartEndEventData_V2* gameStartData = (bz_GameStartEndEventData_V2*)eventData;
        // We reset all player caps.
        resetAllPlayerCaps();
        resetAllPlayersScore();

    }break;
    
    case bz_eGameEndEvent: {
        bz_GameStartEndEventData_V2* gameEndData = (bz_GameStartEndEventData_V2*)eventData;
        
        resetAllPlayerCaps();
        
        //resetAllPlayersScore(); //
        // For now, no need to reset scores, players may wish to view scores and see how many caps made by each.
        // Unsure if a public message announcing captures made by players makes sense...

    }break;
    
    case bz_ePlayerScoreChanged: {
        bz_PlayerScoreChangeEventData_V1* scoreChangeData = (bz_PlayerScoreChangeEventData_V1*)eventData;
        
        // We use wins for captures, losses for self-captures, if there is team kills, we simply set to zero. 
        bz_eScoreElement scoreElement = scoreChangeData->element;
        int playerScoreID = scoreChangeData->playerID;
        int scoreValue = scoreChangeData->thisValue;
        
        
        if (isInRange(playerScoreID) == 1) {
            if (scoreElement == bz_eWins) {
                
                if (scoreValue != playerCapValue[playerScoreID]) {
                    bz_setPlayerWins(playerScoreID, playerCapValue[playerScoreID]);
                }
                
            } else if (scoreElement == bz_eLosses) {
                
                if (scoreValue != playerSelfCapValue[playerScoreID]) {
                    bz_setPlayerLosses(playerScoreID, playerSelfCapValue[playerScoreID]);
                }
                
            } else { // bz_eTKs
                
                if (scoreValue != 0) { // Only possibility is team kill value.
                    bz_setPlayerTKs(playerScoreID, 0); // Erase team kills as we've changed the scoring system.
                }
                
            }
        }

    }break;
 
   case bz_eAllowCTFCaptureEvent: {
      bz_AllowCTFCaptureEventData_V1* allowCapData = (bz_AllowCTFCaptureEventData_V1*)eventData;
      if (bz_getPlayerTeam(allowCapData->playerCapping) == allowCapData->teamCapped) {
        allowCapData->allow = false;
        basePoint = bz_checkBaseAtPoint(allowCapData->pos);
        if (basePoint != eNoTeam) {
            bz_resetFlag(bz_getPlayerFlagID(allowCapData->playerCapping));
            cappingPlayer = allowCapData->playerCapping;
            cappingTeam = bz_getPlayerTeam(allowCapData->playerCapping);
            alive = 1;
            //
            bz_APIIntList *playerList = bz_newIntList();
            if (playerList) {
                bz_getPlayerIndexList(playerList);
                for ( unsigned int i = 0; i < playerList->size(); i++) {
                    int targetID = (*playerList)[i];
                    bz_BasePlayerRecord *playRec = bz_getPlayerByIndex ( targetID );
                    if (playRec != NULL) {
                        if ((playRec->spawned) && (playRec->team == cappingTeam)) {
                            if (targetID != cappingPlayer) {
                                bz_killPlayer(targetID, true, BZ_SERVER);
                            }
                        }
                    }
                    bz_freePlayerRecord(playRec);
                }
                bz_deleteIntList(playerList);
            }
            //
            bz_givePlayerFlag(cappingPlayer, teamToFlagType(basePoint), true);
            
        }

        // This shouldn't be so tedious to do.
       
        }
 
    }break;

    case bz_eTickEvent: {
      if ((cappingPlayer != -1) && (alive == 1)) {
        int flagID = bz_getPlayerFlagID(cappingPlayer);
        if (flagID != -1) {
            bz_eTeamType flagTeam = flagToTeamValue(bz_getFlagName(flagID).c_str());
            if (flagTeam != basePoint) {
                bz_resetFlag(flagID);
                bz_givePlayerFlag(cappingPlayer, teamToFlagType(basePoint), true);
            }
        } else {
            bz_givePlayerFlag(cappingPlayer, teamToFlagType(basePoint), true);
        }
        bz_triggerFlagCapture(cappingPlayer, cappingTeam, basePoint);
      }
    }break;

    case bz_ePlayerSpawnEvent: {
      bz_PlayerSpawnEventData_V1* spawnData = (bz_PlayerSpawnEventData_V1*)eventData;
      if ((spawnData->playerID == cappingPlayer) && (cappingPlayer != -1)) {
        alive = 1;
        bz_givePlayerFlag(cappingPlayer, teamToFlagType(basePoint), true);
      } 
    }break;

    default: {
    }break;

    }
}
