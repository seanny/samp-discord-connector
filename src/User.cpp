#include "User.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "PawnCallback.hpp"
#include "CLog.hpp"
#include "utils.hpp"


User::User(UserId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	if (!utils::TryGetJsonValue(data, m_Id, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return;
	}

	Update(data);
}

void User::Update(json &data)
{
	_valid =
		utils::TryGetJsonValue(data, m_Username, "username") &&
		utils::TryGetJsonValue(data, m_Discriminator, "discriminator");

	if (!_valid)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't update user: invalid JSON: \"{}\"", data.dump());
		return;
	}

	utils::TryGetJsonValue(data, m_IsBot, "bot");
	utils::TryGetJsonValue(data, m_IsVerified, "verified");
	utils::TryGetJsonValue(data, m_Email, "email");
}


void UserManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
		if (!utils::IsValidJson(data, "user", json::value_t::object))
		{
			// TODO: should be loglevel fatal
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"user\" in \"{}\"", data.dump());
			return;
		}
		AddUser(data["user"]); // that's our bot
		m_Initialized++;
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::USER_UPDATE, [](json &data)
	{
		Snowflake_t user_id;
		if (!utils::TryGetJsonValue(data, user_id, "id"))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		}

		PawnDispatcher::Get()->Dispatch([data, user_id]() mutable
		{
			auto const &user = UserManager::Get()->FindUserById(user_id);
			if (!user)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update user: user id \"{}\" not cached", user_id);
				return;
			}

			user->Update(data);

			// forward DCC_OnUserUpdate(DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnUserUpdate", user->GetPawnId());
		});
	});
}

bool UserManager::WaitForInitialization()
{
	unsigned int const
		SLEEP_TIME_MS = 20,
		TIMEOUT_TIME_MS = 20 * 1000;
	unsigned int waited_time = 0;
	while (m_Initialized != m_InitValue)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MS));
		waited_time += SLEEP_TIME_MS;
		if (waited_time > TIMEOUT_TIME_MS)
			return false;
	}
	return true;
}

UserId_t UserManager::AddUser(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return INVALID_USER_ID;
	}

	User_t const &user = FindUserById(sfid);
	if (user)
		return user->GetPawnId();

	UserId_t id = 1;
	while (m_Users.find(id) != m_Users.end())
		++id;

	if (!m_Users.emplace(id, User_t(new User(id, data))).first->second)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't create user: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	return id;
}

User_t const &UserManager::FindUser(UserId_t id)
{
	static User_t invalid_user;
	auto it = m_Users.find(id);
	if (it == m_Users.end())
		return invalid_user;
	return it->second;
}

User_t const &UserManager::FindUserByName(
	std::string const &name, std::string const &discriminator)
{
	static User_t invalid_user;
	for (auto const &u : m_Users)
	{
		User_t const &user = u.second;
		if (user->GetUsername().compare(name) == 0
			&& user->GetDiscriminator().compare(discriminator) == 0)
		{
			return user;
		}
	}
	return invalid_user;
}

User_t const &UserManager::FindUserById(Snowflake_t const &sfid)
{
	static User_t invalid_user;
	for (auto const &u : m_Users)
	{
		User_t const &user = u.second;
		if (user->GetId().compare(sfid) == 0)
			return user;
	}
	return invalid_user;
}
