#include "Authentification.h"
crow::response Autentificare::Register(std::string username, std::string password)
{
    return crow::response();
}
crow::response Autentificare::Login(std::string username, std::string password)
{    auto player = m_dataBase.get_all<Player>(sql::where(sql::is_equal(&Player::playerName, username)));
     auto playerPassword = m_dataBase.get_all<Player>(sql::where(sql::is_equal(&Player::password, password)));
     if (player.size() == 0 || playerPassword.size() == 0)
     {
		 return crow::response(404,"login failed");
	 }
     else
     {
         return crow::response(200,"login succesful");
     }
     
     return crow::response();
}
Autentificare::Autentificare(Storage& dataBase): m_dataBase(dataBase)
{
	/*EMPTY*/
}
Autentificare::~Autentificare()
{ 
   /*EMPTY*/
}
