#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "auth.h"

static const char* passwords[] = {
    "skibidisigma", "amogus", "nagibator228", "СКУФИДРОН", "1005001337228"
};

static const char script[] =
    "set -eux\n"
    "password=\"$1\"\n"
    "salt=\"$2\"\n"
    "token=\"$3\"\n"
    "expected_token=\"$(printf '%s%s' \"$password\" \"$salt\" | md5sum -z | cut -f1 -d' ')\"\n"
    "[ \"$token\" = \"$expected_token\" ]"
;

int main(void) {
    for (unsigned int i = 0; i < sizeof(passwords) / sizeof(passwords[0]); i++) {
        const char *password = passwords[i];

        const struct auth_data *auth_data = get_auth_data(password);

        pid_t pid = fork();
        assert(pid >= 0);
        if (pid == 0) /* child */ {
            execl("/bin/sh", "sh", "-c", script,
                  "sh", password, auth_data->salt, auth_data->token, NULL);
            fprintf(stderr, "execl failed: %m");
            assert(0);
        }

        int status;
        assert(waitpid(pid, &status, 0) == pid);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            return WTERMSIG(status);
        }
    }
}

