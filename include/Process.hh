#pragma once

#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <iostream>

extern char **environ;

class Process {
public:
    inline Process(std::vector<char *> args)
        : _args(std::move(args))
    { this->run(); }

    template<class T>
    using Alias = T;

    template <class... Args>
    inline Process(Args&&... args)
    {
        using ptr = char*;
        Alias<char[]>{(
                _args.push_back(ptr(args))
                ,'0'
                )...,'0'};
        this->run();
    }

private:
    inline void run() {
        pid_t pid = fork();

        if (pid == -1) {
            throw 42;
        } else if (pid == 0) {
            _args.push_back(nullptr);
            execve(_args[0], _args.data(), environ);
            perror("srsly?? ");
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (status != 0) {
                 throw 42;
            }
        }
    }

private:
    std::vector<char *> _args;
};
