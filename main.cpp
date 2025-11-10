#include <iostream>

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

    /*
    //Check if there are no double dots or dot at the end
    //Check if current i pos has '.', if yes then reject
    //move the i to the next dot, after i++ will move it to next. If we see another dot then reject
    for (int i = 0; i < hostname.size(); i++) {
        if (hostname[i] == '.') {
            std::cerr << "Invalid hostname: empty label (consecutive or leading dot)." << std::endl;
            return EXIT_FAILURE;
        }
        if (hostname.find('.',i) != std::string::npos) {
            i = hostname.find('.',i);
        }
    }
    */

    //Create a view which will be used to determine the correctness of the hostname
    std::string_view view = hostname;
    //In case if view ends with '.' then remove it. It should still work properly.
    if (view.rfind('.') == view.size()-1) {
        if (view.size() == 0) {
            std::cerr << "Invalid hostname: empty label (consecutive or leading dot)." << std::endl;
            return EXIT_FAILURE;
        }
        view.remove_suffix(1);
    }
    //Check if there are no double dots or dot at the end
    //Check if current i pos has '.', if yes then reject
    //move the i to the next dot, after i++ will move it to snext. If we see another dot then reject
    //example.com.s

    std::string_view test = "example.com.s";

    std::cout << test.substr(8,11-8) << std::endl;

    std::cout << view.find('.',12) << std::endl;

    size_t labelStart = 0;
    size_t labelEnd = 0;
    do {
        labelEnd = view.find('.',labelStart);
        std::string_view labelView = view.substr(labelStart,labelEnd-labelStart);
        std::cout << "labelView: " << labelView << std::endl;

        if (labelEnd == std::string_view::npos) {break;}
        labelStart = labelEnd + 1;
    } while (true);
    /*
    for (size_t i = 0; i < view.size(); i++) {
        size_t labelStart = i;
        std::cout << "view: " << view << std::endl;
        std::cout << "i " << i << std::endl;
        size_t labelEnd = view.find('.',i);
        std::cout << "labelEnd: " << labelEnd << std::endl;
        std::string_view labelView = view.substr(labelStart, labelEnd-labelStart);
        std::cout << labelView << std::endl;
        std::cout << labelEnd << std::endl;
        std::cout << i << std::endl;

        i = labelEnd;
    }*/

    std::cout << "Host: " << argv[1] << std::endl;
    return 0;
}