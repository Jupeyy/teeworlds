/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/client/components/motd.h>
#include <game/client/components/skins.h>
#include <game/client/teecomp.h>
#include <game/localization.h>
#include "teecomp_stats.h"
#include "scoreboard.h"


CScoreboard::CScoreboard()
{
	OnReset();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	((CScoreboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0;
}

void CScoreboard::OnReset()
{
	m_Active = false;
}

void CScoreboard::OnRelease()
{
	m_Active = false;
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
}

void CScoreboard::RenderGoals(float x, float y, float w)
{
	float h = 50.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// render goals
	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit);
			TextRender()->Text(0, x, y, 20.0f, aBuf, -1);
		}
		if(m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit);
			TextRender()->Text(0, x+220.0f, y, 20.0f, aBuf, -1);
		}
		if(m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum && m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Round"), m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent, m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum);
			float tw = TextRender()->TextWidth(0, 20.0f, aBuf, -1);
			TextRender()->Text(0, x+w-tw-20.0f, y, 20.0f, aBuf, -1);
		}
	}
}

void CScoreboard::RenderSpectators(float x, float y, float w)
{
	char aBuffer[1024*4];
	int Count = 0;
	float h = 120.0f;
	
	str_format(aBuffer, sizeof(aBuffer), "%s: ", Localize("Spectators"));

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y-10.f, w, h, 10.0f);
	Graphics()->QuadsEnd();
	
	for(int i = 0; i < Client()->SnapNumItems(IClient::SNAP_CURRENT); i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
		{
			const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;
			if(pInfo->m_Team == TEAM_SPECTATORS)
			{
				if(Count)
					str_append(aBuffer, ", ", sizeof(aBuffer));
				if(g_Config.m_ClScoreboardClientID)
				{
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "%d | %s", pInfo->m_ClientID, m_pClient->m_aClients[pInfo->m_ClientID].m_aName);
					str_append(aBuffer, aBuf, sizeof(aBuffer));
				}
				else
					str_append(aBuffer, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, sizeof(aBuffer));
				Count++;
			}
		}
	}
	
	TextRender()->Text(0, x+10, y, 32, aBuffer, (int)w-20);
}

void CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle)
{
	if(Team == TEAM_SPECTATORS)
		return;
	
	//float ystart = y;
	float h = 740.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x-10.f, y, w, h, 17.0f);
	Graphics()->QuadsEnd();

	// render title
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else
			pTitle = Localize("Score board");
	}
		
	float Offset = 0;
	if(m_pClient->m_IsRace)
		Offset = 110.0f;
	
	float tw = TextRender()->TextWidth(0, 48, pTitle, -1);
	TextRender()->Text(0, x+10, y, 48, pTitle, -1);
	
	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		if(m_pClient->m_Snap.m_pGameDataObj)
		{
			char aBuf[128];
			int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue;
			if(!m_pClient->m_IsRace)
			{
				str_format(aBuf, sizeof(aBuf), "%d", Score);
				tw = TextRender()->TextWidth(0, 48, aBuf, -1);
				TextRender()->Text(0, x+w-tw-30, y, 48, aBuf, -1);
			}
		}
	}
	else
	{
		char aBuf[128];
		int Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
		if(!m_pClient->m_IsRace)
		{
			str_format(aBuf, sizeof(aBuf), "%d", Score);
			tw = TextRender()->TextWidth(0, 48, aBuf, -1);
			TextRender()->Text(0, x+w-tw-30, y, 48, aBuf, -1);
		}
	}

	y += 54.0f;

	// render headlines
	TextRender()->Text(0, x+10, y, 24.0f, Localize("Score"), -1);
	TextRender()->Text(0, x+125+Offset, y, 24.0f, Localize("Name"), -1);
	TextRender()->Text(0, x+w-75, y, 24.0f, Localize("Ping"), -1);
	y += 29.0f;

	float FontSize = 35.0f;
	float LineHeight = 50.0f;
	float TeeSizeMod = 1.0f;
	float TeeOffset = 0.0f;
	
	if(m_pClient->m_Snap.m_aTeamSize[Team] > 13)
	{
		FontSize = 30.0f;
		LineHeight = 40.0f;
		TeeSizeMod = 0.8f;
		TeeOffset = -5.0f;
	}
	
	// render player scores
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paInfoByScore[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;
		
		// make sure that we render the correct team

		char aBuf[128];
		if(pInfo->m_Local)
		{
			// background so it's easy to find the local player
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1,1,1,0.25f);
			RenderTools()->DrawRoundRect(x, y, w-20, LineHeight*0.95f, 17.0f);
			Graphics()->QuadsEnd();
		}

		float FontSizeResize = FontSize;
		float Width;
		const float ScoreWidth = 60.0f+Offset;
		const float PingWidth = 60.0f;
		if(m_pClient->m_IsRace)
		{
			// reset time
			if(pInfo->m_Score == -9999)
				m_pClient->m_aClients[pInfo->m_ClientID].m_Score = 0;
				
			float Time = m_pClient->m_aClients[pInfo->m_ClientID].m_Score;
			if(Time > 0)
			{	
				str_format(aBuf, sizeof(aBuf), "%02d:%06.3f", (int)Time/60, fmod(Time, 60));
				while((Width = TextRender()->TextWidth(0, FontSizeResize, aBuf, -1)) > ScoreWidth)
					--FontSizeResize;
				TextRender()->Text(0, x+ScoreWidth-Width, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);
			}
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -9999, 9999));
			while((Width = TextRender()->TextWidth(0, FontSizeResize, aBuf, -1)) > ScoreWidth)
				--FontSizeResize;
			TextRender()->Text(0, x+ScoreWidth-Width, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);
		}
		
		FontSizeResize = FontSize;
		if(g_Config.m_ClScoreboardClientID)
		{
			str_format(aBuf, sizeof(aBuf), "%d | %s", pInfo->m_ClientID, m_pClient->m_aClients[pInfo->m_ClientID].m_aName);
			if(m_pClient->m_IsRace)
			{
				while(TextRender()->TextWidth(0, FontSizeResize, aBuf, -1) > w-163.0f-Offset-PingWidth)
					--FontSizeResize;
				TextRender()->Text(0, x+128.0f+Offset, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);
			}
			else
			{
				while(TextRender()->TextWidth(0, FontSizeResize, aBuf, -1) > w-163.0f-PingWidth)
					--FontSizeResize;
				TextRender()->Text(0, x+128.0f, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);
			}
		}
		else
		{
			if(m_pClient->m_IsRace)
			{
				while(TextRender()->TextWidth(0, FontSizeResize, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1) > w-163.0f-Offset-PingWidth)
					--FontSizeResize;
				TextRender()->Text(0, x+128.0f+Offset, y+(FontSize-FontSizeResize)/2, FontSizeResize, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);
			}
			else
			{
				while(TextRender()->TextWidth(0, FontSizeResize, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1) > w-163.0f-PingWidth)
					--FontSizeResize;
				TextRender()->Text(0, x+128.0f, y+(FontSize-FontSizeResize)/2, FontSizeResize, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);
			}
		}

		FontSizeResize = FontSize;
		str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, -9999, 9999));
		while((Width = TextRender()->TextWidth(0, FontSizeResize, aBuf, -1)) > PingWidth)
			--FontSizeResize;
		TextRender()->Text(0, x+w-35.0f-Width, y+(FontSize-FontSizeResize)/2, FontSizeResize, aBuf, -1);

		// render flag
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS &&
			m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientID ||
			m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientID))
		{
			Graphics()->BlendNormal();
			if(g_Config.m_TcColoredFlags)
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME_GRAY].m_Id);
			else
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(pInfo->m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
			
			if(g_Config.m_TcColoredFlags)
			{
				vec3 Col = CTeecompUtils::GetTeamColor(1-pInfo->m_Team, m_pClient->m_Snap.m_pLocalInfo->m_Team, 
					g_Config.m_TcColoredTeesTeam1, g_Config.m_TcColoredTeesTeam2, g_Config.m_TcColoredTeesMethod);
				Graphics()->SetColor(Col.r, Col.g, Col.b, 1.0f);
			}

			float size = 64.0f;
			IGraphics::CQuadItem QuadItem(x+55+Offset, y-15, size/2, size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
		
		// render avatar
		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
		
		// anti rainbow
		if(g_Config.m_ClAntiRainbow && (m_pClient->m_aClients[pInfo->m_ClientID].m_ColorChangeCount > g_Config.m_ClAntiRainbowCount))
		{
			if(g_Config.m_TcForceSkinTeam1)
				TeeInfo.m_Texture = m_pClient->m_pSkins->Get(max(0, m_pClient->m_pSkins->Find(g_Config.m_TcForcedSkin1)))->m_OrgTexture;
			else
				TeeInfo.m_Texture = m_pClient->m_pSkins->Get(m_pClient->m_aClients[pInfo->m_ClientID].m_SkinID)->m_OrgTexture;
			TeeInfo.m_ColorBody = vec4(1,1,1,1);
			TeeInfo.m_ColorFeet = vec4(1,1,1,1);
		}

		TeeInfo.m_Size *= TeeSizeMod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1,0), vec2(x+90+Offset, y+28+TeeOffset));

		
		y += LineHeight;
	}
}

void CScoreboard::RenderRecordingNotification(float x)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.4f);
	RenderTools()->DrawRoundRectExt(x, 0.0f, 120.0f, 50.0f, 15.0f, CUI::CORNER_B);
	Graphics()->QuadsEnd();

	//draw the red dot
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	RenderTools()->DrawRoundRect(x+20, 15.0f, 20.0f, 20.0f, 10.0f);
	Graphics()->QuadsEnd();

	//draw the text
	TextRender()->Text(0, x+50.0f, 8.0f, 24.0f, Localize("REC"), -1);
}

void CScoreboard::OnRender()
{
	if(!m_pClient->m_Snap.m_pLocalInfo)
		return;

	if(!Active())
		return;
		
	// if the score board is active, then we should clear the motd message aswell
	if(m_pClient->m_pMotd->IsActive())
		m_pClient->m_pMotd->Clear();
	

	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	
	Graphics()->MapScreen(0, 0, Width, Height);

	float w = 650.0f;
	
	// resize scoreboard for race
	if(m_pClient->m_IsRace)
		w = 750.0f;

	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		if(!(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS))
			RenderScoreboard(Width/2-w/2, 150.0f, w, 0, 0);
		else
		{
		
			if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER && m_pClient->m_Snap.m_pGameDataObj)
			{
				const char *pText = Localize("Draw!");
				if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue)
					pText = Localize("Red team wins!");
				else if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed)
					pText = Localize("Blue team wins!");
			
				float w = TextRender()->TextWidth(0, 86.0f, pText, -1);
				TextRender()->Text(0, Width/2-w/2, 39, 86.0f, pText, -1);
			}
		
			RenderScoreboard(Width/2-w-20, 150.0f, w, TEAM_RED, Localize("Red team"));
			RenderScoreboard(Width/2 + 20, 150.0f, w, TEAM_BLUE, Localize("Blue team"));
		}
	}

	RenderGoals(Width/2-w/2, 150+750+25, w);
	RenderSpectators(Width/2-w/2, 150+750+25+50+25, w);
	RenderRecordingNotification((Width/7)*4);
}

bool CScoreboard::Active()
{
	// if statboard active dont show scoreboard
	if(m_pClient->m_pTeecompStats->IsActive())
		return false;

	// if we activly wanna look on the scoreboard
	if(m_Active)
		return true;

	if(g_Config.m_ClRenderScoreboard && !g_Config.m_ClClearAll && m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
	{
		// we are not a spectator, check if we are dead
		if(!m_pClient->m_Snap.m_pLocalCharacter)
			return true;
	}

	// if the game is over
	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return true;

	return false;
}