//
// Created by benoit on 18/09/29.
//

#ifndef PISTACHE_TEST_UTILS_H
#define PISTACHE_TEST_UTILS_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>

namespace Pistache {
    namespace Test {
        class SocketWrapper {

        public:
            explicit SocketWrapper(int fd): fd_(fd) {}
            ~SocketWrapper() { close(fd_);}

            uint16_t port() {
                sockaddr_in sin;
                socklen_t len = sizeof(sin);

                uint16_t port = 0;
                if (getsockname(fd_, (struct sockaddr *)&sin, &len) == -1) {
                    perror("getsockname");
                } else {
                    port = ntohs(sin.sin_port);
                }
                return port;
            }
        private:
            int fd_;
        };


/*
 * Will try to get a free port by binding port 0.
 */
        Pistache::Test::SocketWrapper bind_free_port() {
            int sockfd;  // listen on sock_fd, new connection on new_fd
            addrinfo hints, *servinfo, *p;

            int yes=1;
            int rv;

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE; // use my IP

            if ((rv = getaddrinfo(nullptr, "0", &hints, &servinfo)) != 0) {
                std::cerr << "getaddrinfo: " << gai_strerror(rv) << "\n";
                exit(1);
            }

            // loop through all the results and bind to the first we can
            for(p = servinfo; p != nullptr; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                     p->ai_protocol)) == -1) {
                    perror("server: socket");
                    continue;
                }

                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                               sizeof(int)) == -1) {
                    perror("setsockopt");
                    exit(1);
                }

                if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    close(sockfd);
                    perror("server: bind");
                    continue;
                }

                break;
            }

            freeaddrinfo(servinfo); // all done with this structure

            if (p == nullptr)  {
                fprintf(stderr, "server: failed to bind\n");
                exit(1);
            }
            return Pistache::Test::SocketWrapper(sockfd);
        }

        uint16_t find_port() {
            uint16_t port_nb = 0;

            // This is just done to get the value of a free port. The socket will be closed
            // after the closing curly bracket and the port will be free again (SO_REUSEADDR option).
            // In theory, it is possible that some application grab this port before we bind it again...
            {
                Pistache::Test::SocketWrapper s = bind_free_port();
                port_nb = s.port();
            }

            return port_nb;
        }

    }

}
#endif //PISTACHE_TEST_UTILS_H
