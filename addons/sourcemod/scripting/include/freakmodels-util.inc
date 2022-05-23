/*
 HELPER FUNCTIONS
 ============================================
 */

bool GetTargetString(int client, int target, char[] buffer, int buffersize)
{
	if (client == target) strcopy(buffer, buffersize, "your");
	else
	{
		GetClientName(target, buffer, buffersize);
		StrCat(buffer, buffersize, "'s");
	}
	return client == target;
}

/**
 * Gets a value arg, performing user input checks along the way
 * @param args how many args are in the full command
 * @param i arg to get value of
 * @param path buffer to store value string into
 * @param pathsize size of path
 * @return one of GETPATHARG_ALREADYFILLED, GETPATHARG_NOVAL, or GOOD
 */
FuncOutput GetPathArg(int args, int i, char[] path, int pathsize)
{
	if (path[0]) return GETPATHARG_ALREADYFILLED;
	if (i > args || !checkArgIsVal(i)) return GETPATHARG_NOVAL;

	GetCmdArg(i, path, pathsize);
	return GOOD;
}

bool checkArgIsVal(int i)
{
	char arg[128];

	GetCmdArg(i, arg, sizeof(arg));
	return (arg[0] != '-');
}

void ToLowerCase(char[] str, int strsize)
{
	for (int i = 0; i < strsize; i++)
	{
		if (str[i])
			str[i] = CharToLower(str[i]);
	}
}

/**
 * Returns if a client ID is valid.
 * Note a client index of 0 - the server - is not considered valid.
 * @return true if client is a valid client index, false if not.
 */
public bool IsValidClient(int client) 
{ 
	return client > 0 && client <= MaxClients && IsClientInGame(client); 
}

/**
 * Checks if a client index is valid, and if not, throws an error.
 * Note a client index of 0 - the server - is not considered valid and will throw an error.
 * @error if client is not a valid client index.
 */
public void CheckClientValid(int client) 
{ 
	if (!IsValidClient(client)) ThrowError("Client index %i is not a valid client.", client); 
}