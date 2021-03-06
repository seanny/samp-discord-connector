#include "sdk.hpp"
#include "natives.hpp"
#include "PawnDispatcher.hpp"
#include "PawnCallback.hpp"
#include "Network.hpp"
#include "Guild.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "SampConfigReader.hpp"
#include "CLog.hpp"
#include "version.hpp"

#include <samplog/DebugInfo.h>


extern void	*pAMXFunctions;
logprintf_t logprintf;


PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
	
	samplog::Init();
	CLog::Get()->SetLogLevel(LogLevel::DEBUG | LogLevel::INFO | LogLevel::WARNING | LogLevel::ERROR);

	bool ret_val = true;
	std::string bot_token;
	if (SampConfigReader::Get()->GetVar("discord_bot_token", bot_token))
	{
		GuildManager::Get()->Initialize();
		UserManager::Get()->Initialize();
		ChannelManager::Get()->Initialize();

		Network::Get()->Initialize(bot_token);

		if (GuildManager::Get()->WaitForInitialization()
			&& UserManager::Get()->WaitForInitialization()
			&& ChannelManager::Get()->WaitForInitialization())
		{
			logprintf(" >> plugin.dc-connector: " PLUGIN_VERSION " successfully loaded.");
		}
		else
		{
			logprintf(" >> plugin.dc-connector: timeout while initializing data.");
			ret_val = false;
		}
	}
	else
	{
		logprintf(" >> plugin.dc-connector: bot token not specified in server config.");
		ret_val = false;
	}
	return ret_val;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	logprintf("plugin.dc-connector: Unloading plugin...");
	
	ChannelManager::CSingleton::Destroy();
	UserManager::CSingleton::Destroy();
	GuildManager::CSingleton::Destroy();
	Network::CSingleton::Destroy();
	PawnCallbackManager::CSingleton::Destroy();
	CLog::CSingleton::Destroy();
	
	samplog::Exit();
	
	logprintf("plugin.dc-connector: Plugin unloaded.");
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	PawnDispatcher::Get()->Process();
}


extern "C" const AMX_NATIVE_INFO native_list[] =
{
	//AMX_DEFINE_NATIVE(native_name)
	AMX_DEFINE_NATIVE(DCC_FindChannelByName)
	AMX_DEFINE_NATIVE(DCC_FindChannelById)
	AMX_DEFINE_NATIVE(DCC_GetChannelId)
	AMX_DEFINE_NATIVE(DCC_GetChannelType)
	AMX_DEFINE_NATIVE(DCC_GetChannelGuild)
	AMX_DEFINE_NATIVE(DCC_GetChannelName)
	AMX_DEFINE_NATIVE(DCC_GetChannelTopic)
	AMX_DEFINE_NATIVE(DCC_SendChannelMessage)

	AMX_DEFINE_NATIVE(DCC_FindUserByName)
	AMX_DEFINE_NATIVE(DCC_FindUserById)
	AMX_DEFINE_NATIVE(DCC_GetUserId)
	AMX_DEFINE_NATIVE(DCC_GetUserName)
	AMX_DEFINE_NATIVE(DCC_GetUserDiscriminator)
	AMX_DEFINE_NATIVE(DCC_GetUserEmail)
	AMX_DEFINE_NATIVE(DCC_IsUserBot)
	AMX_DEFINE_NATIVE(DCC_IsUserVerified)

	AMX_DEFINE_NATIVE(DCC_FindRoleById)
	AMX_DEFINE_NATIVE(DCC_GetRoleId)
	AMX_DEFINE_NATIVE(DCC_GetRoleName)
	AMX_DEFINE_NATIVE(DCC_GetRoleColor)
	AMX_DEFINE_NATIVE(DCC_GetRolePermissions)
	AMX_DEFINE_NATIVE(DCC_IsRoleHoist)
	AMX_DEFINE_NATIVE(DCC_IsRoleMentionable)

	AMX_DEFINE_NATIVE(DCC_FindGuildByName)
	AMX_DEFINE_NATIVE(DCC_FindGuildById)
	AMX_DEFINE_NATIVE(DCC_GetGuildId)
	AMX_DEFINE_NATIVE(DCC_GetGuildName)
	AMX_DEFINE_NATIVE(DCC_GetGuildOwnerId)
	AMX_DEFINE_NATIVE(DCC_GetGuildRole)
	AMX_DEFINE_NATIVE(DCC_GetGuildRoleCount)
	AMX_DEFINE_NATIVE(DCC_GetGuildMember)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberCount)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberNickname)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberRole)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberRoleCount)
	AMX_DEFINE_NATIVE(DCC_HasGuildMemberRole)
	AMX_DEFINE_NATIVE(DCC_GetGuildMemberStatus)
	AMX_DEFINE_NATIVE(DCC_GetGuildChannel)
	AMX_DEFINE_NATIVE(DCC_GetGuildChannelCount)
	
	{ NULL, NULL }
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	samplog::RegisterAmx(amx);
	PawnCallbackManager::Get()->AddAmx(amx);
	return amx_Register(amx, native_list, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	samplog::EraseAmx(amx);
	PawnCallbackManager::Get()->RemoveAmx(amx);
	return AMX_ERR_NONE;
}
