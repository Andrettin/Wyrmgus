#ifndef __WYRMGUS_MODULE_H__
#define __WYRMGUS_MODULE_H__

#include <scene/main/node.h>

class CCampaign;

class WyrmgusModule : public Node
{
	GDCLASS(WyrmgusModule, Node)

public:
	int Run();
	String GetVersion();
	void LuaCommand(String command);
	CCampaign *GetCampaign(String ident);
	Array GetCampaigns();
	void SetCurrentCampaign(String campaign_ident);
	CCampaign *GetCurrentCampaign();

protected:
	static void _bind_methods();
};

#endif
