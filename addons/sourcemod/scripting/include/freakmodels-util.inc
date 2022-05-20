/*
 HELPER FUNCTIONS
 ============================================
 */

bool GetTargetString(int client, int target, char[] buffer, int buffersize)
{
	if (client == target) strcopy(buffer, buffersize, "your");
	else
	{
		char name[64];
		GetClientName(target, name, sizeof(name));
		StrCat(name, sizeof(name), "'s");
		strcopy(buffer, buffersize, name);
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