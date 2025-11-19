#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

int main(const int argc, char* argv[] ) {
    //Must be only one argument provided
    if (argc != 2) {
        std::cerr << "Usage: <program> <hostname>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string_view hostname = argv[1];

    //Reject blank argument
    if (hostname.empty()) return EXIT_FAILURE;
    //Reject too large argument
    if (hostname.size() > 253) return EXIT_FAILURE;
    //Reject special hostname symbol
    if (hostname.find("://") != std::string_view::npos) {
        std::cerr << "Invalid input: looks like a URL (contains \"://\"). Please pass a hostname like \"example.com\"." << std::endl;
        return EXIT_FAILURE;
    }
    //Reject path or query chars
    if (
        hostname.find('/') != std::string_view::npos ||
        hostname.find('\\') != std::string_view::npos ||
        hostname.find('?') != std::string_view::npos ||
        hostname.find('&') != std::string_view::npos ||
        hostname.find('#') != std::string_view::npos
    ) {
        std::cerr << "Invalid input: contains path or query characters ('/', '?', '#')" << std::endl;
        return EXIT_FAILURE;
    }

    if (
        hostname.find(' ') < hostname.size() ||
        hostname.find('\t') < hostname.size() ||
        hostname.find('\n') < hostname.size()
    ) {
        std::cerr << "No whitespace allowed" << std::endl;
        return EXIT_FAILURE;
    }

    //Create a view which will be used to determine the correctness of the hostname
    std::string_view view = hostname;
    //In case if view ends with '.' then remove it for next checkup stages
    if (view.ends_with('.')) {
        //If the size of the view is 1, then the resulting string is empty and should be rejected
        if (view.size() == 1) {
            std::cerr << "Invalid hostname: empty label (consecutive or leading dot)." << std::endl;
            return EXIT_FAILURE;
        }
        view.remove_suffix(1);
    }

    //Initialize iterators used in splitting strings into labels
    //First label start is start of the string
    size_t labelStart = 0;
    size_t labelEnd = 0;
    do {
        //Find where the label ends (it's either next dot or end of string if no dots remain)
        labelEnd = view.find('.',labelStart);
        //Save label in separate string_view
        std::string_view labelView = view.substr(labelStart,labelEnd-labelStart);
        //Reject incorrect labels
        //Empty label
        if (labelView.empty()) {
            std::cerr << "Invalid hostname: empty label (consecutive or leading dot)." << std::endl;
            return EXIT_FAILURE;
        }
        //Label size 1-63
        if (labelView.size() > 63) {
            std::cerr << "Invalid hostname: Label length: 1–63." << std::endl;
            return EXIT_FAILURE;
        }
        //Label should not start or end with '-'
        if (labelView.ends_with('-') || labelView.starts_with('-')) {
            std::cerr << "Invalid hostname: No leading or trailing - in label." << std::endl;
            return EXIT_FAILURE;
        }

        if (!std::ranges::all_of(labelView, [](unsigned char c){return std::isalnum(c) || c == '-';})) {
            std::cerr << "Invalid hostname: Allowed chars per label: [A–Z a–z 0–9 -]." << std::endl;
            return EXIT_FAILURE;
        }

        //In case if it's the last label
        if (labelEnd == std::string_view::npos) {break;}
        //When looking for next label then it starts with the next char after the end of last one
        labelStart = labelEnd + 1;
    } while (true);

    std::cout << "Host: " << argv[1] << std::endl;


    struct addrinfo hints; //Create a addrinfo structure for hints
    struct addrinfo *result, *rp; //Create addrinfo pointers for furure results and result poninter

    //Initialize all hints as 0
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_protocol = 0;          // Any protocol

    int rc; //Get result code of the getaddrinfo
    rc = getaddrinfo(argv[1], "80", &hints, &result);
    if (rc != 0) {
        std::cerr << gai_strerror(rc) << std::endl;
        return EXIT_FAILURE;
    }

    int v6_addr_count = 0;
    int v4_addr_count = 0;
    int total_addr_count = 0;

    //Create pointers which will be used to point to objects of types sockaddr_in and sockaddr_in6
    struct sockaddr_in *sockaddr_ipv4;
    struct sockaddr_in6 *sockaddr_ipv6;
    //Create ipbuffer with max possible size = ipv6
    char ipbuffer[INET6_ADDRSTRLEN];
    std::string ipv = "";
    int sockfd, connfd, selectfd;
    bool connected = false;
    int errno;

    fd_set writefds; // Used to perform select() in the socket
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 5000;

    int optval;
    socklen_t optlen;

    std::cout << "Resolved addresses for " << hostname << ":"<<std::endl;

    //Iterate trough linked list of all found addresses
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        //If ai_family is AF_INET then this ipv4 address
        if (rp->ai_family == AF_INET) {
            v4_addr_count++;
            sockaddr_ipv4 = (struct sockaddr_in *)rp->ai_addr;
            //Use inet_ntop to transform IP address from HEX to ipv4
            auto results = inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipbuffer, INET_ADDRSTRLEN);
            if (results == nullptr) {
                std::cout << "IPv4: <invalid>" << std::endl;
            } else {
                std::cout <<"IPv4: "<< ipbuffer << std::endl;
            }
            ipv = "IPv4";
        } else if (rp->ai_family == AF_INET6) { // AF_INET6 is for ipv6
            v6_addr_count++;
            sockaddr_ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
            //Use inet_ntop to transform IP address from HEX to ipv6
            auto results = inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr,ipbuffer, INET6_ADDRSTRLEN);
            if (results == nullptr) {
                std::cout << "IPv6: <invalid>" << std::endl;
            } else {
                std::cout <<"IPv6: "<< ipbuffer << std::endl;
            }
            ipv = "IPv6";
        }
        total_addr_count++;
        //If socket creation is not successful proceed with next rp
        if ((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
            continue;
        }
        fcntl(sockfd, F_SETFL, O_NONBLOCK); //Set the successful socket to non-blocking

        if((connfd = connect(sockfd, rp->ai_addr, rp->ai_addrlen)) == 0) {
            connected = true;
            std::cout << "Connected to straight"<< ipbuffer <<":80 " << ipv << std::endl;
            break;
        } else if(connfd == -1 && errno == EINPROGRESS) { // If unable to connect straight away, wait a bit to get result again
            FD_ZERO(&writefds);//initialize file descriptor set values to contain no file descriptors
            FD_SET(sockfd, &writefds); //set the current used socket to t
            selectfd = select(sockfd+1,NULL,&writefds,NULL,&tv); //get the answer from socket in timespan selected in tv timeval struct
            if (selectfd > 0) { // If we have at least one answer from the socket then proceed
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen)== 0) { // if connection succeeded stop looking
                    connected = true;
                    std::cout << "Connected to "<< ipbuffer <<":80 " << ipv << std::endl;
                    FD_CLR(sockfd, &writefds); //remove socket from file descriptor set
                    fcntl(sockfd, F_SETFL, ~O_NONBLOCK);
                    break;
                }
                std::cout << "errno getsockopt: " << errno << std::endl;
            }
            FD_CLR(sockfd, &writefds); //remove socket from file descriptor set
        }
        close(sockfd); // Close socket if connection unsuccessful
    }
    if (v4_addr_count + v6_addr_count == 0) {
        std::cout << "no printable addresses for AF_INET/AF_INET6" << std::endl;
    }
    if (!connected) {
        std::cerr << "Failed to connect to any resolved address (timed out or refused)" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout<< "Resolved " << total_addr_count << " addresses (" << v4_addr_count << " IPv4, " << v6_addr_count << " IPv6)" << std::endl;

    freeaddrinfo(result);

    return 0;
}