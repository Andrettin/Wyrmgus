#ifndef __WYRMGUS_H__
#define __WYRMGUS_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <core/ustring.h>
#include <scene/main/node.h>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCampaign;
class CHairColor;
class CLiteraryText;
class CPlayer;
class CPlayerColor;
class CSkinColor;
class oamlGodotModule;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class Wyrmgus : public Node
{
	GDCLASS(Wyrmgus, Node)
	
public:
	static Wyrmgus *GetInstance();

private:
	static Wyrmgus *Instance;

public:
	void Run();
	String GetVersion() const;
	void LuaCommand(const String command);
	
	void SetOamlModule(Node *oaml_module);
	
	oamlGodotModule *GetOamlModule() const
	{
		return this->OamlModule;
	}
	
	const String &GetUserDirectory() const
	{
		return this->UserDirectory;
	}
	
	String NumberToRomanNumeral(const unsigned number) const;

private:
	oamlGodotModule *OamlModule = nullptr;
	String UserDirectory;
	
protected:
	static void _bind_methods();
};

#endif
