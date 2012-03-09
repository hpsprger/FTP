#include <client_ftp.h>

static size_t size_packet = sizeof(struct packet);

static const char commandlist[NCOMMANDS][10] =
	{
		"get",
		"put",

		"mget",
		"mput",
		
		"cd",
		"lcd",
		
		"mgetwild",
		"mputwild",
		
		"dir",
		"ldir",

		"ls",
		"lls",
		
		"rget",
		"rput",
		
		"pwd",
		"lpwd",
		
		"exit"
	};			// any change made here should also be \
				replicated accordingly in the COMMANDS \
				enum in commons.h

static void append_path(struct command* c, char* s)
{
	char** temppaths = (char**) malloc(c->npaths * sizeof(char*));
	if(c->npaths > 1)
		memcpy(temppaths, c->paths, (c->npaths - 1) * sizeof(char*));

	char* temps = (char*) malloc((strlen(s) + 1) * sizeof(char));
	strcpy(temps, s);

	*(temppaths + c->npaths - 1) = temps;

	c->paths = temppaths;
}


struct command* userinputtocommand(char s[LENUSERINPUT])
{
	struct command* cmd = (struct command*) malloc(sizeof(struct command));
	cmd->id = -1;
	cmd->npaths = 0;
	cmd->paths = NULL;
	char* savestate;
	char* token;
	int i, j;
	for(i = 0; ; i++, s = NULL)
	{
		token = strtok_r(s, " \t\n", &savestate);
		if(token == NULL)
			break;
		if(cmd->id == -1)
			for(j = 0; j < NCOMMANDS; j++)
			{	
				if(!strcmp(token, commandlist[j]))
				{
					cmd->id = j;
					break;
				}
			}// ommitting braces for the "for loop" here is \
			 disastrous because the else below gets \
			 associated with the "if inside the for loop". \
			 #BUGFIX
		else
		{
			cmd->npaths++;
			append_path(cmd, token);
		}
	}
	if(cmd->id != -1)
		return cmd;
	else
	{
		fprintf(stderr, "\tError parsing command\n");
		return NULL;
	}
}

void printcommand(struct command* c)
{
	if(!DEBUG)
		return;
	
	printf("\t\tPrinting contents of the above command...\n");
	printf("\t\tid = %d\n", c->id);
	printf("\t\tnpaths = %d\n", c->npaths);
	printf("\t\tpaths =\n");
	int i;
	for(i = 0; i < c->npaths; i++)
		printf("\t\t\t%s\n", c->paths[i]);
	printf("\n");
}

void command_pwd(struct packet* chp, struct packet* data, int sfd_client)
{
	int x;
	set0(chp);
	chp->type = REQU;
	chp->conid = -1;
	chp->comid = PWD;
	data = htonp(chp);
	if((x = send(sfd_client, data, size_packet, 0)) != size_packet)
		er("send()", x);
	if((x = recv(sfd_client, data, size_packet, 0)) <= 0)
		er("recv()", x);
	chp = ntohp(data);
	if(chp->type == DATA && chp->comid == PWD && strlen(chp->buffer) > 0)
		printf("\t%s\n", chp->buffer);
	else
		fprintf(stderr, "\tError retrieving information.\n");
}

void command_cd(struct packet* chp, struct packet* data, int sfd_client, char* path)
{
	int x;
	set0(chp);
	chp->type = REQU;
	chp->conid = -1;
	chp->comid = CD;
	strcpy(chp->buffer, path);
	data = htonp(chp);
	if((x = send(sfd_client, data, size_packet, 0)) != size_packet)
		er("send()", x);
	if((x = recv(sfd_client, data, size_packet, 0)) <= 0)
		er("recv()", x);
	chp = ntohp(data);
	if(chp->type == INFO && chp->comid == CD && !strcmp(chp->buffer, "success"))
		;
	else
		fprintf(stderr, "\tError executing command on the server.\n");
}

void command_lls(char* lpwd)
{
	DIR* d = opendir(lpwd);
	if(!d)
		er("opendir()", (int) d);
	struct dirent* e;
	while(e = readdir(d))
		printf("\t%s\t%s\n", e->d_type == 4 ? "DIR:" : "FILE:", e->d_name);
	closedir(d);
}

void command_ls(struct packet* chp, struct packet* data, int sfd_client)
{
	int x;
	set0(chp);
	chp->type = REQU;
	chp->conid = -1;
	chp->comid = LS;
	data = htonp(chp);
	if((x = send(sfd_client, data, size_packet, 0)) != size_packet)
		er("send()", x);
	while(chp->type != EOT)
	{
		if(chp->type == DATA && chp->comid == LS && strlen(chp->buffer))
			printf("\t%s\n", chp->buffer);
		/*
		else
			fprintf(stderr, "\tError executing command on the server.\n");
		*/
		if((x = recv(sfd_client, data, size_packet, 0)) <= 0)
			er("recv()", x);
		chp = ntohp(data);
	}
}

void command_get(struct packet* chp, struct packet* data, int sfd_client, char* filename)
{
	FILE* f = fopen(filename, "wb");
	if(!f)
		er("fopen()", (int) f);
	int x;
	set0(chp);
	chp->type = REQU;
	chp->conid = -1;
	chp->comid = GET;
	strcpy(chp->buffer, filename);
	data = htonp(chp);
	if((x = send(sfd_client, data, size_packet, 0)) != size_packet)
		er("send()", x);
	receive_file(chp, data, sfd_client, f);
	fclose(f);
}

