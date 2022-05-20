/*
 * Everything for the command freakmodel_manage.
 */

Action ManageCommand(int client, int args)
{

	if (args < 1)
	{
		ManageCommandHelp(client);
		return Plugin_Handled;
	}

	char operation[65];
	GetCmdArg(1, operation, sizeof(operation));

	if(StrEqual(operation, "refreshconfig", false))
	{
		RefreshConfigFromFile();
		ReplyToCommand(client, "Configuration refreshed from file.");
		ReplyToCommand(client, "Map will need to be changed to precache any new models.");
	}
	else if (StrEqual(operation, "help", false))
	{
		ManageCommandHelp(client);
	}
	else
	{
		ReplyToCommand(client, "Unknown operation.");
	}

	return Plugin_Handled;
}

void ManageCommandHelp(int client)
{
	ReplyToCommand(client, "Printing FreakModels manage command options.");
	ReplyToCommand(client, "- refreshconfig: Refreshes the model configuration from file. Map will need to be changed to precache & use any new models.");
}
