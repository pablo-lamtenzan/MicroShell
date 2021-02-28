
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
# include <sys/wait.h>

# define PIPE 		1
# define SEMICOLON	2

int* operators;
int fd[3];

size_t ft_strlen(const char* s)
{
	char* it = (char*)s;
	while (*it)
		it++;
	return (it - s);
}

void* ft_error(const char* msg)
{
	write(STDERR_FILENO, msg, ft_strlen(msg));
	return (NULL);
}

void* ft_exit()
{
	ft_error("SOME MSG\n");
	exit(EXIT_FAILURE);
	return (NULL);
}

void* free_exit()
{
	free(operators);
	return (ft_exit());	
}

void parse_operators(int ac, char** av)
{
	// Malloc a greather/valid size (simplicity > optimisation in exams)
	if (!(operators = malloc(sizeof(int) * ac)))
		ft_exit();

	// Store the operators and replace in av by NULL
	size_t z = 0;
	for (size_t i = 0 ; i < ac ; i++)
	{
		operators[i] = 0;
		if (!strncmp(av[i], "|", 2))
		{
			av[i] = NULL;
			operators[z++] = PIPE;
		}
		else if (!strncmp(av[i], ";", 2))
		{
			av[i] = NULL;
			operators[z++] = SEMICOLON;
		}
	}
}

int execute(char*** av, size_t index, char** env)
{
	int ret = EXIT_FAILURE;

	// Execute cd builtin
	if (!strncmp((*av)[0], "cd", 3))
		return (builtin_cd(*av));
	
	// Execute child proccesses
	else
	{
		int fildes[2];

		if (pipe(fildes))
			free_exit();

		// Set pipes redirections
		if (index && operators[index - 1] == PIPE)
		{
			fd[0] = fd[2];
			fd[1] = 1;
		}
		if (operators[index] == PIPE)
		{
			fd[1] = fildes[1];
			fd[2] = fildes[0];
		}

		// TO DO: Sem overwrite the 2nd pipe
		if (operators[index] == SEMICOLON)
		{
			fd[0] = 0;
			fd[1] = 1;
			fd[2] = 2;
		}
	
		// Execute in the targeted fds
		int pid = fork();
		if (pid == 0)
		{
			// Use pipe redirections
			for (size_t i = 0 ; i < 2 ; i++)
			{
				if (fd[i] != i && (dup2(fd[i], i) < 0 || close(fd[i]) < 0))
					free_exit();
			}
			execve((*av)[0], *av, env);
			// Some error msg
			exit(EXIT_FAILURE);
		}
		else if (pid < 0)
			free_exit();

		// Wait for the process
		int wstatus;
		while (waitpid(pid, &wstatus, 0) >= 0);
		if (WIFEXITED(wstatus))
			ret = WEXITSTATUS(wstatus);

		// Close fds
		for (size_t i = 0 ; i < 2 ; i++)
		{
			if (fd[i] != i && close(fd[i]) < 0)
				free_exit();
		}
	}

	// Go to next cmd
	size_t distance = 0;
	while ((*av)[distance])
		distance++;
	*av += distance + 1;
	return (ret);
}

int main(int ac, char** av, char** env)
{
	fd[0] = 0;
	fd[1] = 1;
	fd[2] = 2;

	parse_operators(ac, av);

	int ret;
	// TO DO: "ls | cat" does not work, "ls | cat ;" works
	// TO DO: Perhabs use 2 defines for the pipes is easier
	for (size_t i = 0 ; operators[i] ; i++)
		ret = execute(&av, i, env);
	return (ret);
}