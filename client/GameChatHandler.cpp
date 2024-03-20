/*
 * GameChatHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameChatHandler.h"
#include "CServerHandler.h"
#include "CPlayerInterface.h"
#include "PlayerLocalState.h"
#include "globalLobby/GlobalLobbyClient.h"
#include "lobby/CLobbyScreen.h"

#include "adventureMap/CInGameConsole.h"

#include "../CCallback.h"

#include "../lib/networkPacks/PacksForLobby.h"
#include "../lib/TextOperations.h"
#include "../lib/mapObjects/CArmedInstance.h"
#include "../lib/CConfigHandler.h"
#include "../lib/MetaString.h"

static std::string getCurrentTimeFormatted(int timeOffsetSeconds = 0)
{
	// FIXME: better/unified way to format date
	auto timeNowChrono = std::chrono::system_clock::now();
	timeNowChrono += std::chrono::seconds(timeOffsetSeconds);

	return TextOperations::getFormattedTimeLocal(std::chrono::system_clock::to_time_t(timeNowChrono));
}

const std::vector<GameChatMessage> & GameChatHandler::getChatHistory() const
{
	return chatHistory;
}

void GameChatHandler::resetMatchState()
{
	chatHistory.clear();
}

void GameChatHandler::sendMessageGameplay(const std::string & messageText)
{
	LOCPLINT->cb->sendMessage(messageText, LOCPLINT->localState->getCurrentArmy());
	CSH->getGlobalLobby().sendMatchChatMessage(messageText);
}

void GameChatHandler::sendMessageLobby(const std::string & senderName, const std::string & messageText)
{
	LobbyChatMessage lcm;
	lcm.message = messageText;
	lcm.playerName = senderName;
	CSH->sendLobbyPack(lcm);
	CSH->getGlobalLobby().sendMatchChatMessage(messageText);
}

void GameChatHandler::onNewLobbyMessageReceived(const std::string & senderName, const std::string & messageText)
{
	if (!SEL)
	{
		logGlobal->debug("Received chat message for lobby but lobby not yet exists!");
		return;
	}

	auto * lobby = dynamic_cast<CLobbyScreen*>(SEL);

	// FIXME: when can this happen?
	assert(lobby);
	assert(lobby->card);

	if(lobby && lobby->card)
	{
		MetaString formatted = MetaString::createFromRawString("[%s] %s: %s");
		formatted.replaceRawString(getCurrentTimeFormatted());
		formatted.replaceRawString(senderName);
		formatted.replaceRawString(messageText);

		lobby->card->chat->addNewMessage(formatted.toString());
		if (!lobby->card->showChat)
				lobby->toggleChat();
	}

	chatHistory.push_back({senderName, messageText, getCurrentTimeFormatted()});
}

void GameChatHandler::onNewGameMessageReceived(PlayerColor sender, const std::string & messageText)
{
	std::string timeFormatted = getCurrentTimeFormatted();
	std::string playerName = sender.isSpectator() ? "Spectator" : sender.toString(); //FIXME: should actually be player nickname, at least in MP

	chatHistory.push_back({playerName, messageText, timeFormatted});

	LOCPLINT->cingconsole->addMessage(timeFormatted, playerName, messageText);
}

void GameChatHandler::onNewSystemMessageReceived(const std::string & messageText)
{
	chatHistory.push_back({"System", messageText, getCurrentTimeFormatted()});

	if(LOCPLINT && !settings["session"]["hideSystemMessages"].Bool())
		LOCPLINT->cingconsole->addMessage(getCurrentTimeFormatted(), "System", messageText);
}

